#include "constants.h"

#include <climits>
#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <iterator>
#include <thread>
#include <shared_mutex>
#include <string.h>
#include <sstream>
#include <time.h>
#include <getopt.h>


typedef struct memItem {
	// v Heuristic value
	// m Best move
	// r Recursion level
	int32_t v;
	int8_t m;
	int8_t r;
} memItem;

typedef union ID {
	uint64_t l[2];
	uint32_t i[4];
	uint16_t s[8];
	uint8_t c[16];
} ID;

class IDj {
	public:
	ID t;
	uint8_t score[2];
	friend bool operator==(IDj &a, IDj &b);
	friend bool operator!=(IDj &a, IDj &b);
	IDj() {
		t.l[0] = 0;
		t.l[1] = 0;
		score[0] = 0;
		score[1] = 0;
	}
};

class joc {
	public:
		signed char board[6][2];
		uint8_t score[2];
		bool mou(short pos, signed char jug);
		std::pair<int, uint8_t> ia(const signed char jug, uint8_t rec, const uint8_t path=0);
		joc* copy();
		int8_t getMove();
		void print();
		void ini();
		IDj getId(const signed char jug);
		std::unordered_map<IDj, memItem > *memoize;
		~joc();
		std::string id2str(const IDj &a);
	private:
		int8_t moviment;
		int getValue(const signed char jug);
};
