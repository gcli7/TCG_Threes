#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <vector>
#include <algorithm>

#define NO_OP 4

/**
 * array-based board for 2048
 *
 * index (1-d form):
 *  (0)  (1)  (2)  (3)
 *  (4)  (5)  (6)  (7)
 *  (8)  (9) (10) (11)
 * (12) (13) (14) (15)
 *
 */
class board {
public:
	typedef uint32_t cell;
	typedef std::array<cell, 4> row;
	typedef std::array<row, 4> grid;
	typedef uint64_t data;
	typedef int reward;

public:
	board() : tile(), attr(0), last_op(NO_OP), max_tile(0), tile_counter(0),
              bag({1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3}) {}
	board(const grid& b, data v = 0) : tile(b), attr(v), last_op(NO_OP), max_tile(0), tile_counter(0),
                                       bag({1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3}) {}
	board(const board& b) = default;
	board& operator =(const board& b) = default;

	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 4][i % 4]; }
	const cell& operator ()(unsigned i) const { return tile[i / 4][i % 4]; }

	data info() const { return attr; }
	data info(data dat) { data old = attr; attr = dat; return old; }

public:
	bool operator ==(const board& b) const { return tile == b.tile; }
	bool operator < (const board& b) const { return tile <  b.tile; }
	bool operator !=(const board& b) const { return !(*this == b); }
	bool operator > (const board& b) const { return b < *this; }
	bool operator <=(const board& b) const { return !(b < *this); }
	bool operator >=(const board& b) const { return !(*this < b); }

public:

	/**
	 * place a tile (index value) to the specific position (1-d form index)
	 * return 0 if the action is valid, or -1 if not
	 */
	reward place(unsigned pos, cell tile) {
		if (pos >= 16) return -1;
		operator()(pos) = tile;
        remove_tile(tile);
        tile_counter++;
		if (tile >= 3) return std::pow(3, tile - 2);
		return 0;
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	reward slide(unsigned opcode) {
		switch (opcode & 0b11) {
			case 0: last_op = 0;    return slide_up();
			case 1: last_op = 1;    return slide_right();
			case 2: last_op = 2;    return slide_down();
			case 3: last_op = 3;    return slide_left();
			default: last_op = NO_OP;    return -1;
		}
	}

    int get_max_tile() { return max_tile; }
    const int get_max_tile() const { return max_tile; }
    int get_last_op() { return last_op; }
    const int get_last_op() const { return last_op; }
    std::vector<int> get_bag_all_tiles() { return bag; }
    const std::vector<int> get_bag_all_tiles() const { return bag; }
    int get_bag_tile(int n) { return bag[n]; }
    const int get_bag_tile(int n) const { return bag[n]; }
    int get_bag_size() { return bag.size(); }
    const int get_bag_size() const { return bag.size(); }

protected:
	reward slide_left() {
		board prev = *this;
		reward score = 0;
		for (int r = 0; r < 4; r++) {
			auto& row = tile[r];
			for (int c = 1; c < 4; c++) {
				if (row[c-1] == 0) {
					row[c-1] = row[c];
					row[c] = 0;
				}
				else if (row[c] == 1 || row[c] == 2) {
					if(row[c] + row[c-1] != 3) continue;
					row[c-1] = 3;
					score += 3;
					row[c] = 0;
				}
				else if (row[c] == row[c-1]) {
					row[c-1] += 1;
                    if(row[c-1] > max_tile)
                        max_tile = row[c-1];
					score += pow(3, row[c] - 2);
					row[c] = 0;
				}
			}
		}
		return (*this != prev) ? score : -1;
	}

	reward slide_right() {
		reflect_horizontal();
		reward score = slide_left();
		reflect_horizontal();
		return score;
	}

	reward slide_up() {
		rotate_right();
		reward score = slide_right();
		rotate_left();
		return score;
	}

	reward slide_down() {
		rotate_right();
		reward score = slide_left();
		rotate_left();
		return score;
	}

	void transpose() {
		for (int r = 0; r < 4; r++)
			for (int c = r + 1; c < 4; c++)
				std::swap(tile[r][c], tile[c][r]);
	}

	void reflect_horizontal() {
		for (int r = 0; r < 4; r++) {
			std::swap(tile[r][0], tile[r][3]);
			std::swap(tile[r][1], tile[r][2]);
		}
	}

	void reflect_vertical() {
		for (int c = 0; c < 4; c++) {
			std::swap(tile[0][c], tile[3][c]);
			std::swap(tile[1][c], tile[2][c]);
		}
	}

	/**
	 * rotate the board clockwise by given times
	 */
	void rotate(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotate_right(); break;
		case 2: reverse(); break;
		case 3: rotate_left(); break;
		}
	}

	void rotate_right() { transpose(); reflect_horizontal(); } // clockwise
	void rotate_left() { transpose(); reflect_vertical(); } // counterclockwise
	void reverse() { reflect_horizontal(); reflect_vertical(); }

    void remove_tile(int t) {
        std::vector<int>::iterator vi = find(bag.begin(), bag.end(), t);
        if (vi != bag.end())
            bag.erase(vi);
        else
            ; //std::cout << "Bag : an error at removing tile." << std::endl;

        if (bag.empty())
            for(int i = 1; i <= 3; i++)
                for(int j = 0; j < 4; j++) {
                    if(max_tile >= 7 && tile_counter >= 21) {
                        std::default_random_engine engine;
                        std::uniform_int_distribution<int> random_generator;
                        random_generator.param(std::uniform_int_distribution<>::param_type {0, 20});
                        int random_num = random_generator(engine);

                        if(random_num == 0) {
                            std::vector<int> bonus;
                            int max_index = max_tile - 3;
                            for(int k = 4; k <= max_index; k++)
                                bonus.push_back(k);
                            std::shuffle(bonus.begin(), bonus.end(), engine);
                            bag.push_back(bonus[0]);
                            tile_counter %= 21;
                        }
                    }
                    bag.push_back(i);
                }
                    
    }

public:
	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		out << "+------------------------+" << std::endl;
		for (auto& row : b.tile) {
			out << "|" << std::dec;
			for (auto t : row) out << std::setw(6) << ((1 << t) & -2u);
			out << "|" << std::endl;
		}
		out << "+------------------------+" << std::endl;
		return out;
	}

private:
	grid tile;
	data attr;
	int last_op;
    int max_tile;
    int tile_counter;
	std::vector<int> bag;
};
