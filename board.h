#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>

/**
 * array-based board for Threes
 *
 * index (1-d form):
 *  (0)  (1)  (2)
 *  (3)  (4)  (5)
 *
 */
class board {
public:
	typedef uint32_t cell;
	typedef std::array<cell, 3> row;
	typedef std::array<row, 2> grid;
	typedef uint64_t data;
	typedef int reward;

public:
	board() : last_op(-1), tile(), attr(0) {}
	board(const grid& b, data v = 0) : last_op(-1), tile(b), attr(v) {}
	board(const board& b) = default;
	board& operator =(const board& b) = default;

	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 3][i % 3]; }
	const cell& operator ()(unsigned i) const { return tile[i / 3][i % 3]; }

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
		if (pos >= 6) return -1;
		if (tile != 1 && tile != 2 && tile != 3) return -1;
		operator()(pos) = tile;
        if (tile == 3) return 3;
		return 0;
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	reward slide(unsigned opcode) {
		switch (opcode & 0b11) {
		case 0: return slide_up();
		case 1: return slide_right();
		case 2: return slide_down();
		case 3: return slide_left();
		default: return -1;
		}
	}

	reward slide_left() {
		board prev = *this;
		reward score = 0;

		for (int r = 0; r < 2; r++) {
			for (int c = 1; c < 3; c++) {
				if (tile[r][c-1] == 0) {
					tile[r][c-1] = tile[r][c];
					tile[r][c] = 0;
				}
				else if (tile[r][c] == 1 || tile[r][c] == 2) {
					if(tile[r][c] + tile[r][c-1] != 3) continue;
					tile[r][c-1] = 3;
					score += 3;
					tile[r][c] = 0;
				}
				else if (tile[r][c] == tile[r][c-1]) {
					tile[r][c-1] += 1;
					score += pow(3, tile[r][c] - 2);
					tile[r][c] = 0;
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
        board prev = *this;
		reward score = 0;

        for (int c = 0; c < 3; c++) {
            if (tile[0][c] == 0) {
                tile[0][c] = tile[1][c];
                tile[1][c] = 0;
            }
            else if (tile[0][c] == 1 || tile[0][c] == 2) {
                if(tile[0][c] + tile[1][c] != 3) continue;
                tile[0][c] = 3;
                score += 3;
                tile[1][c] = 0;
            }
            else if (tile[0][c] == tile[1][c]) {
                tile[0][c] += 1;
                score += pow(3, tile[1][c] - 2);
                tile[1][c] = 0;
            }
        }

		return (*this != prev) ? score : -1;
	}

	reward slide_down() {
        reflect_vertical();
		reward score = slide_up();
        reflect_vertical();
		return score;
	}

	void reflect_horizontal() {
        std::swap(tile[0][0], tile[0][2]);
        std::swap(tile[1][0], tile[1][2]);
	}

	void reflect_vertical() {
        std::swap(tile[0][0], tile[1][0]);
        std::swap(tile[0][1], tile[1][1]);
        std::swap(tile[0][2], tile[1][2]);
	}

public:
	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		for (int i = 0; i < 6; i++) {
            out << std::setw(std::min(i, 1)) << "";
            if (b(i) > 3)
			    out << std::pow(2, b(i) - 3) * 3;
            else
                out << b(i);
		}
		return out;
	}
	friend std::istream& operator >>(std::istream& in, board& b) {
		for (int i = 0; i < 6; i++) {
			while (!std::isdigit(in.peek()) && in.good()) in.ignore(1);
			in >> b(i);
            if (b(i) > 3)
			    b(i) = std::log2(b(i) / 3) + 3;
		}
		return in;
	}

public:
    void check_bag() {
        if (bag.empty())
            bag = {1, 2, 3};
    }

    void remove_tile(int t) {
        std::vector<int>::iterator vi = find(bag.begin(), bag.end(), t);
        if (vi != bag.end())
            bag.erase(vi);
        else
            std::cout << "Bag : an error at removing tile." << std::endl;
    }

public:
    std::vector<int> bag;
    int last_op;

private:
	grid tile;
	data attr;
};
