#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include "board.h"
#include "action.h"
#include "weight.h"

#define BIG_FLOAT 99999999.0;
#define SMALL_FLOAT -99999999.0;
#define MAX_TILE_INDEX 15
#define TUPLE_LEN 6
#define TUPLE_NUM 32
#define EXPECT_SEARCH_LEVEL 1

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
	const std::array<int, 4> all_op = {{0 ,1, 2, 3}};
	const std::array<std::array<int, 4>, 4> side_space = {{ {{12, 13, 14, 15}},
															{{0, 4, 8, 12}},
															{{0, 1, 2, 3}},
															{{3, 7, 11, 15}} }};
};

/*
class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};
*/

/**
 * base agent for agents with weight tables
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), learning_rate(0.1 / TUPLE_NUM) {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
		if (meta.find("learning_rate") != meta.end())
			learning_rate = float(meta["learning_rate"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
        int possibility = (int)std::pow(MAX_TILE_INDEX, TUPLE_LEN);
        for (int i = 0; i < TUPLE_NUM; i++)
    		net.emplace_back(possibility);
	}

	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}

	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}

	virtual float get_after_state(const board& after, const int& level) {
		if (level >= EXPECT_SEARCH_LEVEL)
			return get_board_value(after);

		float expect_value = 0.0;
		int expect_counter = 0;

		for (const int& pos : side_space[after.get_last_op()]) {
			board b = board(after);
			board::reward reward = b.place(pos, b.info());
			if (reward == -1) continue;
			expect_value += reward + get_before_state(b, level);
			expect_counter++;
		}

		return expect_value / expect_counter;
	}

	virtual float get_before_state(const board& before, const int& level) {
		float best_expect = SMALL_FLOAT;
		bool move_flag = false;

		for (const int& op : all_op) {
			board b = board(before);
			board::reward reward = b.slide(op);
			if (reward == -1) continue;
			float value = reward + get_after_state(b, level + 1);
			if (value > best_expect) {
				best_expect = value;
				move_flag = true;
			}
		}

		if (move_flag)
			return best_expect;
		return 0.0;
	}

	virtual float get_board_value(const board& b) {
		float weight_sum = net[0][get_feature_key(b, 0)];
		for (int i = 1; i < TUPLE_NUM; i++)
			weight_sum += net[i][get_feature_key(b, i)];
		return weight_sum;
	}

	virtual int get_feature_key(const board& b, const int& row) {
		int key_sum = b(tuple_index[row][0]) * coefficient[0];
		for(int i = 1; i < TUPLE_LEN; i++)
			key_sum += b(tuple_index[row][i]) * coefficient[i];
		return key_sum;
	}

protected:
	float learning_rate;
	std::vector<weight> net;
    const std::array<int, TUPLE_LEN> coefficient = {{ (int)std::pow(MAX_TILE_INDEX, 0), (int)std::pow(MAX_TILE_INDEX, 1),
                                                      (int)std::pow(MAX_TILE_INDEX, 2), (int)std::pow(MAX_TILE_INDEX, 3),
                                                      (int)std::pow(MAX_TILE_INDEX, 4), (int)std::pow(MAX_TILE_INDEX, 5) }};
    const std::array<std::array<int, TUPLE_LEN>, TUPLE_NUM> tuple_index = {{ {{0, 4, 8, 9, 12, 13}},
                                                                             {{1, 5, 9, 10, 13, 14}},
                                                                             {{1, 2, 5, 6, 9, 10}},
                                                                             {{2, 3, 6, 7, 10, 11}},
                                                                             
                                                                             {{3, 2, 1, 5, 0, 4}},
                                                                             {{7, 6, 5, 9, 4, 8}},
                                                                             {{7, 11, 6, 10, 5, 9}},
                                                                             {{11, 15, 10, 14, 9, 13}},
                                                                             
                                                                             {{15, 11, 7, 6, 3, 2}},
                                                                             {{14, 10, 6, 5, 2, 1}},
                                                                             {{14, 13, 10, 9, 6, 5}},
                                                                             {{13, 12, 9, 8, 5, 4}},
                                                                             
                                                                             {{12, 13, 14, 10, 15, 11}},
                                                                             {{8, 9, 10, 6, 11, 7}},
                                                                             {{8, 4, 9, 5, 10, 6}},
                                                                             {{4, 0, 5, 1, 6, 2}},
                                                                             
                                                                             {{3, 7, 11, 10, 15, 14}},
                                                                             {{2, 6, 10, 9, 14, 13}},
                                                                             {{2, 1, 6, 5, 10, 9}},
                                                                             {{1, 0, 5, 4, 9, 8}},
                                                                             
                                                                             {{0, 1, 2, 6, 3, 7}},
                                                                             {{4, 5, 6, 10, 7, 11}},
                                                                             {{4, 8, 5, 9, 6, 10}},
                                                                             {{8, 12, 9, 13, 10, 14}},
                                                                             
                                                                             {{12, 8, 4, 5, 0, 1}},
                                                                             {{13, 9, 5, 6, 1, 2}},
                                                                             {{13, 14, 9, 10, 5, 6}},
                                                                             {{14, 15, 10, 11, 6, 7}},
                                                                             
                                                                             {{15, 14, 13, 9, 12, 8}},
                                                                             {{11, 10, 9, 5, 8, 4}},
                                                                             {{11, 7, 10, 6, 9, 5}},
                                                                             {{7, 3, 6, 2, 5, 1}} }};
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public agent {
public:
	player(const std::string& args = "") : agent("name=dummy role=player " + args) {}

	virtual action take_action(const board& before) {
		int best_op = -1;
		board::reward best_reward = -1;

		for (const int& op : all_op) {
			board::reward reward = board(before).slide(op);
			if (reward > best_reward) {
				best_op = op;
				best_reward = reward;
			}
		}

		if (best_op != -1)
			return action::slide(best_op);
		return action();
	}
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public weight_agent {
public:
	rndenv(const std::string& args = "") : weight_agent("name=random role=environment " + args) {}

	virtual action take_action(const board& after) {
		int op = after.get_last_op();
		if (op >= 0 && op <= 3) {
			std::array<int, 4> space = side_space[op];
			std::shuffle(space.begin(), space.end(), engine);
			for (int pos : space) {
				if (after(pos) != 0) continue;
				return action::place(pos, after.info());
			}
		}
		else if (op == -1) {
			std::array<int, 16> board_space = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
			std::shuffle(board_space.begin(), board_space.end(), engine);
			for (int& pos : board_space) {
				if (after(pos) != 0) continue;
				return action::place(pos, after.info());
			}
		}
		return action();
	}

private:
	std::default_random_engine engine;
};

/**
 * the player trained by TDL
 */
class TDL_player : public weight_agent {
public:
	TDL_player(const std::string& args = "") : weight_agent("name=dummy role=player " + args) {}

	virtual action take_action(const board& before) {
		int best_op = -1;
		float best_weight = SMALL_FLOAT;
		board best_board;
		board::reward best_reward = -1;

		for (const int& op : all_op) {
			board b = board(before);
			board::reward reward = b.slide(op);
			if(reward == -1) continue;
			//float weight = reward + get_board_value(b);
			float weight = reward + get_after_state(b, 0);
			if (weight > best_weight) {
				best_op = op;
				best_weight = weight;
				best_board = board(b);
				best_reward = reward;
			}
		}

		if (best_op != -1) {
			after_states.emplace_back(std::make_pair(best_board, best_reward));
			return action::slide(best_op);
		}
		return action();
	}

	virtual void training() {
		train_weight(after_states[after_states.size()-1].first);
		for (int i = after_states.size() - 1; i > 0; i--)
			train_weight(after_states[i-1].first, after_states[i].first, after_states[i].second);
		after_states.clear();
	}

private:
	virtual void train_weight(const board& b) {
		float err = learning_rate * (0 - get_board_value(b));
		for (int i = 0; i < TUPLE_NUM; i++)
			net[i][get_feature_key(b, i)] += err;
	}

	virtual void train_weight(const board& last_b, const board& b, const board::reward& reward) {
		float err = learning_rate * (get_board_value(b) + reward - get_board_value(last_b));
		for (int i = 0; i < TUPLE_NUM; i++)
			net[i][get_feature_key(last_b, i)] += err;
	}

private:
	std::vector<std::pair<board, board::reward>> after_states;
};