#pragma once
#include <string>
#include <random>
#include <sstream>
#include <limits>
#include <map>
#include <vector>
#include <type_traits>
#include <cmath>
#include <algorithm>
#include "board.h"
#include "action.h"
#include "weight.h"
#include <fstream>

// there are 15 probabilities for tiles
#define TILE_P 15
#define TUPLE_LEN 6
#define TUPLE_NUM 32
#define EXPECT_LEVEL 1

typedef struct board_state {
	board b;
	board::reward r;

	board_state(board bb, board::reward rr) : b(bb), r(rr) { }
} BS;

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

protected:
	virtual void init_weights(const std::string& info) {
		int possibilities = pow(TILE_P, TUPLE_LEN);
		for(int i = 0; i < TUPLE_NUM; i++)
			net.push_back(possibilities);
	}

	virtual float get_after_state(const board& b, const int& level) {
		if(level == EXPECT_LEVEL)
			return get_board_value(b);

		float expect_value = 0;
		int expect_counter = 0;

        for(int i = 0; i < 4; i++) {
			board before = board(b);
			board::reward reward = before.place(side_space[b.get_last_op()][i], b.info());
            if(reward == -1)	continue;
			expect_value += reward + get_before_state(before, level);
            expect_counter++;
        }

		return expect_value / expect_counter;
	}

	virtual float get_before_state(const board& b, const int& level) {
		float best_expect = -999999999;
		float expect = 0;
		bool move_flag = false;

		for(const int& op : opcode) {
			board after = board(b);
			board::reward reward = after.slide(op);
			if(reward == -1) continue;
			expect = reward + get_after_state(b, level + 1);
			if(expect > best_expect) {
				move_flag = true;
				best_expect = expect;
			}
		}

		if(move_flag)
			return best_expect;
		else
			return 0;
	}

	virtual float get_board_value(const board& b) {
		float sum = net[0][get_feature_key(b, 0)];
		for(int i = 1; i < TUPLE_NUM; i++)
			sum += net[i][get_feature_key(b, i)];

		return sum;
	}

	virtual int get_feature_key(const board& b, const int& row) {
		int key_sum = b(weight_index[row][0]) * coef[0];
		for(int i = 1; i < TUPLE_LEN; i++)
			key_sum += b(weight_index[row][i]) * coef[i];

		return key_sum;
	}

	std::vector<weight> net;
	const std::array<int, 4> opcode = {{ 0, 1, 2, 3 }};
	const std::array<std::array<int, 4>, 4> side_space = {{ {{12, 13, 14, 15}},
															{{0, 4, 8, 12}},
															{{0, 1, 2, 3}},
															{{3, 7, 11, 15}} }};
	const std::array<int, 6> coef = {{ (int)std::pow(TILE_P, 0), (int)std::pow(TILE_P, 1), (int)std::pow(TILE_P, 2),
									   (int)std::pow(TILE_P, 3), (int)std::pow(TILE_P, 4), (int)std::pow(TILE_P, 5) }};
	// this is for 32 x 6-tuple
	const std::array<std::array<int, TUPLE_LEN>, TUPLE_NUM> weight_index = {{ {{0, 4, 8, 9, 12, 13}},
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
	/*
	// this is for 8 x 4-tuple
	const std::array<std::array<int, TUPLE_LEN>, TUPLE_NUM> weight_index = {{ {{0, 1, 2, 3}},
															 			  {{4, 5, 6, 7}},
															  			  {{8, 9, 10, 11}},
															  			  {{12, 13, 14, 15}},
															  			  {{0, 4, 8, 12}},
															  			  {{1, 5, 9, 13}},
															  			  {{2, 6, 10, 14}},
															  			  {{3, 7, 11, 15}} }};
	*/
};

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

class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		counter(0) {}

	virtual action take_action(const board& after) {
		if(after.get_last_op() >= 0 && after.get_last_op() <= 3) {
			std::array<int, 4> space;
			space = agent::side_space[after.get_last_op()];
			return generate_tile(after, space);
		}
        else if (after.get_last_op() == NO_OP) {
			std::array<int, 16> space = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
			return generate_tile(after, space);
		}
		return action();
	}

	template <class T>
	action generate_tile(const board& after, T& space){
        std::shuffle(space.begin(), space.end(), engine);
		for (int pos : space) {
			if (after(pos) != 0) continue;
			return action::place(pos, after.info());
		}
		return action();
	}

private:
	int counter;
	int max_tile;
	int bonus_counter;
	std::array<int, 4> space;
};

/**
 * base agent for agents with weight tables
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), learning_rate(0.0015625f) {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			learning_rate = float(meta["alpha"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights("weights.bin");
	}

	virtual action take_action(const board& before) {
        //std::cout << "player action!" << std::endl;
		int best_op = NO_OP;
		float best_weights = -999999999;
		board::reward best_reward = -1;
		board best_board;

		for (const int& op : opcode) {
			board b = board(before);
			board::reward reward = b.slide(op);
			if(reward == -1) continue;
			//float weights = reward + get_board_value(b);
			float weights = reward + get_after_state(b, 0);
			if (weights > best_weights) {
				best_op = op;
				best_weights = weights;
				best_board = b;
				best_reward = reward;
			}
		}
		if(best_op != NO_OP) {
			after_states.emplace_back(best_board, best_reward);
			return action::slide(best_op);
		}

		return action();
	}

	virtual void train_TDL() {
		// remove initial board
		after_states.erase(after_states.begin());

		// first, train final board state
		train_weights(after_states[after_states.size()-1].b);
		for(int i = after_states.size() - 2; i >= 0; i--)
			train_weights(after_states[i].b, after_states[i+1].b, after_states[i].r);

		// initialize after_states at the end of the training
		after_states.clear();
	}

protected:
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

	virtual void train_weights(const board& b, const board& next_b, board::reward& reward) {
		float err = learning_rate * (reward + get_board_value(next_b) - get_board_value(b));
		for(int i = 0; i < TUPLE_NUM; i++)
			net[i][get_feature_key(b, i)] += err;
	}

	virtual void train_weights(const board& b) {
		float err = learning_rate * (0 - get_board_value(b));
		for(int i = 0; i < TUPLE_NUM; i++)
			net[i][get_feature_key(b, i)] += err;
	}

protected:
	float learning_rate;
	std::vector<BS> after_states;
};

/**
 * base agent for agents with a learning rate
 *
class learning_agent : public agent {
public:
	learning_agent(const std::string& args = "") : agent(args), alpha(0.1f) {
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~learning_agent() {}

protected:
	float alpha;
};
*/

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */

/**
 * dummy player
 * select a legal action randomly
 */
/*
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before, int& next_op) {
		int best_op = NO_OP;
		board::reward best_reward = -1;

		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int& op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward > best_reward) {
				best_reward = reward;
				best_op = op;
			}
		}
		if(best_op != NO_OP) {
			next_op = best_op;
			return action::slide(best_op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};
*/
