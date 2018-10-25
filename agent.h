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

// there are 15 possibilities for tiles
#define TILE_P 15
#define TUPLE_L 4
#define TUPLE_N 8

typedef struct board_state {
	board b;
	board::reward r;
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
	virtual action take_action(const board& b, int& last_op) { return action(); }
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

/**
 * base agent for agents with weight tables
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), learning_rate(0.1f), opcode({ 0, 1, 2, 3 }) {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			learning_rate = float(meta["alpha"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

	virtual action take_action(const board& before, int& next_op) {
		int best_op = -1;
		//float best_weights = std::numeric_limits<float>::min();
		float best_weights = -999999999;
		BS bs;

		for (int& op : opcode) {
			board b = board(before);
			board::reward reward = b.slide(op);
			if(reward == -1) continue;
			float weights = reward + get_board_value(b);
			if (weights > best_weights) {
				best_op = op;
				best_weights = weights;
				bs.r = reward;
				bs.b = b;
			}
		}
		if(best_op != -1) {
			next_op = best_op;
			after_states.emplace_back(bs);
			return action::slide(next_op);
		}

		return action();
	}

	virtual void train_TDL() {
		// remove initial board
		after_states.erase(after_states.begin());

		// initialize after_states at the end of the training
		after_states.clear();
	}

protected:
	virtual void init_weights(const std::string& info) {
		/*
		 * 8 x 4-tuple
		 * each tuple may be 0, 1, 2, 3, 6, 12, ... or 6144 (index is from 0 to 14)
		 * there are 15^4 possibilities for tuples
		 */
		int possibilities = pow(TILE_P, TUPLE_L);
		for(int i = 0; i < TUPLE_N; i++)
			net.emplace_back(possibilities);
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

	virtual float get_board_value(const board& b) {
		float sum = net[0][get_feature_key(b, 0)];
		for(int i = 1; i < TUPLE_N; i++)
			sum += net[i][get_feature_key(b, i)];

		return sum;
	}

	virtual int get_feature_key(const board& b, const int& row) {
		int key_sum = b(weight_index[row][0]) * coef[0];
		for(int i = 1; i < TUPLE_L; i++)
			key_sum += b(weight_index[row][i]) * coef[i];

		return key_sum;
	}

protected:
	float learning_rate;
	std::array<int, 4> opcode;
	std::vector<weight> net;
	std::vector<BS> after_states;
	const std::array<int, TUPLE_L> coef = { (int)std::pow(TILE_P, 0), (int)std::pow(TILE_P, 1),
									  		(int)std::pow(TILE_P, 2), (int)std::pow(TILE_P, 3) };
	const std::array<std::array<int, TUPLE_L>, TUPLE_N> weight_index = {{ {{0, 1, 2, 3}},
															 			  {{4, 5, 6, 7}},
															  			  {{8, 9, 10, 11}},
															  			  {{12, 13, 14, 15}},
															  			  {{0, 4, 8, 12}},
															  			  {{1, 5, 9, 13}},
															  			  {{2, 6, 10, 14}},
															  			  {{3, 7, 11, 15}} }};
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
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		counter(0), popup(0, 2) {}

	virtual action take_action(const board& after, int& last_op) {
		if (last_op == -2) {
			// initialize bag at the beginning of a new game
			if(counter == 0)
				bag.clear();
			counter = (counter + 1) % 9;

			std::array<int, 16> space = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
			return generate_tile(after, space);
		}

		std::array<int, 4> space;
		switch (last_op) {
			case 0: space = {12, 13, 14, 15};	break;	// up
			case 1: space = {0, 4, 8, 12};	break;	// right
			case 2: space = {0, 1, 2, 3};	break;	// down
			case 3: space = {3, 7, 11, 15};	break;	// left
			default: return action();
		}
		return generate_tile(after, space);
	}

	template <class T>
	action generate_tile(const board& after, T& space){
		if (bag.empty()) {
			for(int i = 1; i <= 3; i++)
				bag.push_back(i);
		}
		popup.param(std::uniform_int_distribution<>::param_type {0, (int)(bag.size() - 1)});

		std::shuffle(space.begin(), space.end(), engine);
		for (int pos : space) {
			if (after(pos) != 0) continue;
			int random_num = popup(engine);
			board::cell tile = bag[random_num];
			bag.erase(bag.begin() + random_num);
			return action::place(pos, tile);
		}
		return action();
	}

private:
	int counter;
	std::vector<int> bag;
	std::array<int, 4> space;
	std::uniform_int_distribution<int> popup;
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before, int& next_op) {
		int best_op = -1;
		board::reward best_reward = -1;

		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int& op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward > best_reward) {
				best_reward = reward;
				best_op = op;
			}
		}
		if(best_op != -1) {
			next_op = best_op;
			return action::slide(best_op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};