#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>
#include "board.h"
#include <numeric>
#include <unordered_map>
#include <vector>

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
        answer() : min(0.0), avg(0.0), max(0.0) {}
	    friend std::ostream& operator <<(std::ostream& out, const answer& ans) {
	    	return (ans.min + ans.avg + ans.max != 0) ? (out << ans.min << " " << ans.avg << " " << ans.max) : (out << "-1");
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
                start.check_bag();

				start(p) = t;
                start.remove_tile(t);

                for (std::vector<int>::iterator vi = start.bag.begin(); vi != start.bag.end(); vi++) {
                    start.info(*vi);
				    goto_before_state(start);
                }
			}
		}
        std::cout << "Initialization has been completed." << std::endl;
        std::cout << "The size of before_board_table is " << before_board_table.size() << std::endl;
        std::cout << "The size of after_board_table is " << after_board_table.size() << std::endl;
	}

    // player round
    answer goto_before_state(board b) {
        std::unordered_map<unsigned long, answer>::iterator mi;
        b.last_op = 4;
        mi = before_board_table.find(calculate_key(b));
        if (mi != before_board_table.end())
            return mi->second;

        answer best_value;
        answer value;
        bool flag = true;

        for (const int& op : {0, 1, 2, 3}) {
            board player_board = board(b);
            board::reward reward = player_board.slide(op);
            if (reward == -1) continue;
            flag = false;
            player_board.last_op = op;
            value = goto_after_state(player_board);

            if (value.avg > best_value.avg) {
                best_value.max = value.max;
                best_value.avg = value.avg;
                best_value.min = value.min;
            }
        }

        if(flag) {
            int v = calculate_board_value(b);
            best_value.max = v;
            best_value.avg = v;
            best_value.min = v;
        }

        before_board_table[calculate_key(b)] = best_value;

        return best_value;
    }

    // put tile round
    answer goto_after_state(board b) {
        std::unordered_map<unsigned long, answer>::iterator mi;
        mi = after_board_table.find(calculate_key(b));
        if (mi != after_board_table.end())
            return mi->second;

        answer ret_value;
        answer value;
        int counter = 0;
        std::vector<int> side_line;

        switch (b.last_op) {
            case 0: side_line = {3, 4, 5}; break;
            case 1: side_line = {0, 3}; break;
            case 2: side_line = {0, 1, 2}; break;
            case 3: side_line = {2, 5}; break;
        }

        for (std::vector<int>::iterator vi = side_line.begin(); vi != side_line.end(); vi++) {
            board put_board = board(b);
            board::reward reward = put_board.place(*vi, b.info());
            if (reward == -1) continue;
            put_board.remove_tile(b.info());
            put_board.check_bag();
            for (std::vector<int>::iterator vj = put_board.bag.begin(); vj != put_board.bag.end(); vj++) {
                put_board.info(*vj);
                value = goto_before_state(put_board);

                if (value.max > ret_value.max)
                    ret_value.max = value.max;
                ret_value.avg += value.avg;
                if (value.min < ret_value.min || ret_value.min == 0)
                    ret_value.min = value.min;
                counter++;
            }
        }

        ret_value.avg /= counter;
        after_board_table[calculate_key(b)] = ret_value;

        return ret_value;
    }

    int calculate_board_value(const board& b) {
        int value = 0;
        int v = 0;

        for(int p = 0; p < 6; p++) {
            v = index_to_value[b(p)];
            if(v >= 3) {
                v = std::pow(3, std::log2(v / 3) + 1);
                value += v;
            }
        }

        return value;
    }

    unsigned long calculate_key(const board& b) {
        unsigned long key = 0;

        for(int i = 0; i < 6; i++)
            key += b(i) * coef[i];
        key += b.last_op * coef[6];
        key += b.info() * coef[7];

        return key;
    }

	answer solve(const board& state, state_type type = state_type::before) {
		// TODO: find the answer in the lookup table and return it
		//       do NOT recalculate the tree at here

		// to fetch the hint (if type == state_type::after, hint will be 0)
        // board::cell hint = state_hint(state);

		// for a legal state, return its three values.
        // return { min, avg, max };
		// for an illegal state, simply return {}
        board b = board(state);
        std::unordered_map<unsigned long, answer>::iterator mi;

        if (type.is_before()) {
            b.last_op = 4;
            mi = before_board_table.find(calculate_key(b));
            if (mi != before_board_table.end())
                return mi->second;
        }
        else {
            for (const int& op : {0, 1, 2, 3}) {
                b.last_op = op;
                mi = after_board_table.find(calculate_key(b));
                if (mi != after_board_table.end())
                    return mi->second;
            }
        }

		return {};
	}

    void show_board(const board& b, const int& type) {
        switch (type) {
            case BEFORE: std::cout << "==before===";    break;
            case AFTER : std::cout << "===after===";    break;
        }
        std::cout << "\tlast_op = " << b.last_op << std::endl;
        std::cout << "| " << b(0) << "  " << b(1) << "  " << b(2) << " |\thint = " << b.info() << std::endl;
        std::cout << "|=========|\tbag = ";
        for(std::vector<int>::const_iterator vi = b.bag.begin(); vi != b.bag.end(); vi++)
            std::cout << *vi << " ";
        std::cout << std::endl;
        std::cout << "| " << b(3) << "  " << b(4) << "  " << b(5) << " |\tkey = " << calculate_key(b) << std::endl;
        std::cout << "===========" << std::endl << std::endl;
    }

private:
	// TODO: place your transposition table here
    std::unordered_map<unsigned long, answer> before_board_table;
    std::unordered_map<unsigned long, answer> after_board_table;
    const std::array<int, 8> coef = {{ (int)std::pow(TILE_P, 0), (int)std::pow(TILE_P, 1), (int)std::pow(TILE_P, 2), (int)std::pow(TILE_P, 3),
                                       (int)std::pow(TILE_P, 4), (int)std::pow(TILE_P, 5), (int)std::pow(TILE_P, 6), (int)std::pow(TILE_P, 7) }};
    const int index_to_value[16] = {0, 1, 2, 3, 6, 12, 24, 48, 96, 192, 384, 768, 1536, 3072, 6144};
};
