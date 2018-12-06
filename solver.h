#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>
#include "board.h"
#include <numeric>
#include <unordered_map>

#define TILE_P 15
#define BEFORE 0
#define AFTER 1

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
        for (int p = 0; p < 6; p++) {
			for (int t = 1; t <= 3; t++) {
				std::cout << "Initialization for position = " << p << ", tile = " << t << " ..." << std::endl;

				board start;
                for (int i = 0; i < 6; i++)
                    start(i) = 0;
                start.check_bag();

				start(p) = t;
                start.remove_tile(t);

                for (unsigned int h = 0; h < start.bag.size(); h++) {
                    start.info(start.bag[h]);
				    goto_before_state(start);
                }
			}
		}
        std::cout << "Initialization has been completed." << std::endl;
	}

    // player round
    answer goto_before_state(board b) {
        std::unordered_map<unsigned long, answer>::iterator mi;
        mi = before_board_table.find(calculate_key(b, BEFORE));
        if (mi != before_board_table.end())
            return mi->second;

        answer best_value;
        answer value;
        bool flag = true;
        best_value.avg = -1.0;
        b.last_op = -1;

        for (const int& op : {0, 1, 2, 3}) {
            board player_board = board(b);
            board::reward reward = player_board.slide(op);
            if (reward == -1) continue;
            flag = false;
            player_board.last_op = op;
            mi = after_board_table.find(calculate_key(player_board, AFTER));
            if (mi != after_board_table.end())
                value = mi->second;
            else
                value = goto_after_state(player_board);

            if (value.avg > best_value.avg)
                best_value = value;
        }

        if(flag) {
            int v = calculate_board_value(b);
            best_value.max = v;
            best_value.avg = v;
            best_value.min = v;
        }

        before_board_table[calculate_key(b, BEFORE)] = best_value;
        return best_value;
    }

    // put tile round
    answer goto_after_state(board b) {
        return {};
    }

    int calculate_board_value(const board& b) {
        int value = 0;
        int v = 0;

        for(int p = 0; p < 6; p++) {
            v = index_to_value[b(p)];
            if(v > 3)
                v = std::pow(3, std::log2(b(p) / 3) + 1);
            value += v;
        }

        return value;
    }

    unsigned long calculate_key(const board& b, const int& type) {
        unsigned long key = 0;

        for(int i = 0; i < 6; i++)
            key += b(i) * coef[i];
        key += b.last_op * coef[6];
        key += b.info() * coef[7];
        key += type * coef[8];

        return key;
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

private:
	// TODO: place your transposition table here
    std::unordered_map<unsigned long, answer> before_board_table;
    std::unordered_map<unsigned long, answer> after_board_table;
    const std::array<int, 9> coef = {{ (int)std::pow(TILE_P, 0), (int)std::pow(TILE_P, 1), (int)std::pow(TILE_P, 2), (int)std::pow(TILE_P, 3), (int)std::pow(TILE_P, 4),
                                       (int)std::pow(TILE_P, 5), (int)std::pow(TILE_P, 6), (int)std::pow(TILE_P, 7), (int)std::pow(TILE_P, 8) }};
    const int index_to_value[15] = {0, 1, 2, 3, 6, 12, 24, 48, 96, 192, 384, 768, 1536, 3072, 6144};
};
