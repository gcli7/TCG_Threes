#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>
#include "board.h"
#include <numeric>
#include <map>

#define TILE_P 15

class state_type {
public:
	enum type : char {
		before  = 'b',
		after   = 'a',
		illegal = 'i'
	};

public:
	state_type() : t(illegal) {}
	state_type(const state_type& st) = default;
	state_type(state_type::type code) : t(code) {}

	friend std::istream& operator >>(std::istream& in, state_type& type) {
		std::string s;
		if (in >> s) type.t = static_cast<state_type::type>((s + " ").front());
		return in;
	}

	friend std::ostream& operator <<(std::ostream& out, const state_type& type) {
		return out << char(type.t);
	}

	bool is_before()  const { return t == before; }
	bool is_after()   const { return t == after; }
	bool is_illegal() const { return t == illegal; }

private:
	type t;
};

class state_hint {
public:
	state_hint(const board& state) : state(const_cast<board&>(state)) {}

	char type() const { return state.info() ? state.info() + '0' : 'x'; }
	operator board::cell() const { return state.info(); }

public:
	friend std::istream& operator >>(std::istream& in, state_hint& hint) {
		while (in.peek() != '+' && in.good()) in.ignore(1);
		char v; in.ignore(1) >> v;
		hint.state.info(v != 'x' ? v - '0' : 0);
		return in;
	}
	friend std::ostream& operator <<(std::ostream& out, const state_hint& hint) {
		return out << "+" << hint.type();
	}

private:
	board& state;
};


class solver {
public:
	typedef float value_t;

public:
	class answer {
	public:
		answer(value_t min = 0.0/0.0, value_t avg = 0.0/0.0, value_t max = 0.0/0.0) : min(min), avg(avg), max(max) {}
	    friend std::ostream& operator <<(std::ostream& out, const answer& ans) {
	    	return !std::isnan(ans.avg) ? (out << ans.min << " " << ans.avg << " " << ans.max) : (out << "-1") << std::endl;
		}
	public:
		value_t min, avg, max;
	};

public:
	solver(const std::string& args) {
        // TODO: explore the tree and save the result
		for(int p = 0; p < 6; p++) {
			for(int t = 1; t <= 3; t++) {
				std::cout << "Initialization for position = " << p << ", tile = " << t << " ..." << std::endl;
				board start;

				start(p) = t;
                start.remove_bag_tile(t);

                for(unsigned int h = 0; h < start.bag.size(); h++) {
                    start.hint = start.bag[h];
				    goto_before_state(start);
                }
			}
		}
        std::cout << "Initialization has been completed..." << std::endl;
	}

    // player move
	answer goto_before_state(const board& before) {
        std::map<board, answer>::iterator mi = board_value.find(before);
		if(mi != board_value.end())
			return mi->second.avg;

        answer value;
        answer result_value;
        bool flag = true;

        for (const int& op : opcode) {
			board b = board(before);
			board::reward reward = b.slide(op);
			if(reward == -1) continue;
            flag = false;
            b.last_op = op;
            value = goto_after_state(b);
            if (value.avg > result_value.avg)
                result_value = value;
		}

        if(flag) {
            value_t v = 0;

            for(int p = 0; p < 6; p++)
                v += scores[before(p)];

            result_value.max = v;
            result_value.avg = v;
            result_value.min = v;
        }
        board_value[before] = result_value;

        return result_value;
	}

    // put tile
	answer goto_after_state(const board& after) {
        std::map<board, answer>::iterator mi = board_value.find(after);
		if(mi != board_value.end())
			return mi->second.avg;

        std::vector<int> boundary;
        answer value;
        answer result_value;
        int counter = 0;

        result_value.max = 0;
        result_value.avg = 0;
        result_value.min = 0;

        switch(after.last_op) {
            case 0: boundary = {3, 4, 5};  break;
            case 1: boundary = {0, 3};  break;
            case 2: boundary = {0, 1, 2};  break;
            case 3: boundary = {2, 5};  break;
        }

        for(unsigned int i = 0; i < boundary.size(); i++) {
            if(after(boundary[i]) != 0) continue;

            board b = board(after);
            b(boundary[i]) = b.hint;
            b.remove_bag_tile(b.hint);

            for(unsigned int j = 0; j < b.bag.size(); j++) {
                b.hint = b.bag[j];
                value = goto_before_state(b);

                counter++;
                result_value.avg += value.avg;
                if(result_value.max < value.max)
                    result_value.max = value.max;
                if(result_value.min > value.min)
                    result_value.min = value.min;
            }
        }
        board_value[after] = result_value;

        return result_value;
	}

	answer solve(const board& state, state_type type = state_type::before) {
		// TODO: find the answer in the lookup table and return it
		//       do NOT recalculate the tree at here

		// to fetch the hint (if type == state_type::after, hint will be 0)
//		board::cell hint = state_hint(state);

		// for a legal state, return its three values.
//		return { min, avg, max };
		// for an illegal state, simply return {}
		return {};
	}

    int calculate_key(const board &b) {
		int key = b.last_op * coef[6] + b.hint * coef[7];

		for(int i = 0; i < 6; i++)
			key += b(i) * coef[i];

		return key;
	}

	void show_board(const board& b, std::string s) {
		for(int i = 0; i < 5; i++)
			std::cout << "=";
		std::cout << " " << s << " ";
		for(int i = 0; i < 5; i++)
			std::cout << "=";
		std::cout << std::endl;
		for(int i = 0; i < 3; i++)
			std::cout << b(i) << "  ";
		std::cout << std::endl;
		for(int i = 3; i < 6; i++)
			std::cout << b(i) << "  ";
		std::cout << std::endl;
	}

private:
	// TODO: place your transposition table here
	const std::array<int, 4> opcode = {{ 0, 1, 2, 3 }};
	const std::array<int, 8> coef = {{ (int)std::pow(TILE_P, 0), (int)std::pow(TILE_P, 1), (int)std::pow(TILE_P, 2), (int)std::pow(TILE_P, 3),
                                       (int)std::pow(TILE_P, 4), (int)std::pow(TILE_P, 5), (int)std::pow(TILE_P, 6), (int)std::pow(TILE_P, 7) }};
	const int table_size = std::pow(TILE_P, 7) * 4;
    const std::array<int, 15> scores = {{ 0, 0, 0, 3, 9, 27, 81, 243, 729, 2187, 6561, 19683, 59049, 177147, 531441 }};
	std::map<board, answer> board_value;
};
