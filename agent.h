#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <vector>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"

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
		//std::cout << "last_op = " << last_op << std::endl;

		if (last_op == -2) {
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
		//std::cout << "\tbag's size = " << bag.size() << std::endl;
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
			//std::cout << "pos = " << pos << ", random_num = " << random_num << ", tile = " << tile << std::endl;
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

	virtual action take_action(const board& before, int& last_op) {
		int best_op = -1;
		board::reward best_reward = -1;

		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward > best_reward) {
				best_reward = reward;
				best_op = op;
			}
		}
		if(best_op != -1) {
			//std::cout << "direction = " << best_op << std::endl;
			last_op = best_op;
			return action::slide(best_op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};