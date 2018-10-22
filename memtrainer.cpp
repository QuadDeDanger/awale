#include "borjilator.hpp"

int RECURSION_LEVEL = 15;
int MATADES_MULT = 300;
int MORTES_MULT = 400;

static int jugada = 1;

#ifdef DEBUG
static uint64_t used_memoizes = 0;
static std::mutex mtx2;
#endif

static std::shared_mutex mtx;


bool operator==(IDj &a, IDj &b){
	return (a.t.l[0] == b.t.l[0] && a.t.l[1] == b.t.l[1] && a.score[0] == b.score[0] && a.score[1] == b.score[1]);
}
bool operator==(const IDj &a, const IDj &b){
	return (a.t.l[0] == b.t.l[0] && a.t.l[1] == b.t.l[1] && a.score[0] == b.score[0] && a.score[1] == b.score[1]);
}
bool operator!=(IDj &a, IDj &b){
	return !(a.t.l[0] == b.t.l[0] && a.t.l[1] == b.t.l[1] && a.score[0] == b.score[0] && a.score[1] == b.score[1]);
}
std::istream& operator>>(std::istream &in, IDj &a) {
	in.read((char*)a.t.c, 6*sizeof(uint8_t));
	in.read((char*)&a.t.c[8], 6*sizeof(uint8_t));
	in.read((char*)&a.score[0], sizeof(uint8_t));
	in.read((char*)&a.score[1], sizeof(uint8_t));
	return in;
}
std::ostream& operator<<(std::ostream &out, IDj &a) {
	out.write((char*)a.t.c, 6*sizeof(uint8_t));
	out.write((char*)&a.t.c[8], 6*sizeof(uint8_t));
	out.write((char*)a.score, 2*sizeof(uint8_t));
	return out;
}
/*std::ostream& operator<<(std::ostream &out, const IDj &a) {
	//out.write((char*)a.t.c, 6*sizeof(uint8_t));
	//out.write((char*)&a.t.c[8], 6*sizeof(uint8_t));
	out << std::endl;
	out << "C=" << (int)a.t.c[0] << " " << (int)a.t.c[1] << " " << (int)a.t.c[2] << " " <<  (int)a.t.c[3] << " " <<  (int)a.t.c[4] << " " << (int)a.t.c[5] << " " << std::endl;
	out << "C=" << (int)a.t.c[8] << " " << (int)a.t.c[9] << " " << (int)a.t.c[10] << " " <<  (int)a.t.c[11] << " " <<  (int)a.t.c[12] << " " << (int)a.t.c[13] << " " << std::endl;
	return out;
}*/
std::istream& operator>>(std::istream &in, memItem &vmr) {
	size_t vsize = sizeof(vmr.v);
	size_t msize = sizeof(vmr.m);
	size_t rsize = sizeof(vmr.r);
	char v[4];
	char m[2];
	char r[2];
	in.read(v, vsize);
	in.read(m, msize);
	in.read(r, rsize);
	for (int i = 0; i < vsize; i++)
		vmr.v = (vmr.v<<8) + (unsigned char)v[i];
	for (int i = 0; i < msize; i++)
		vmr.m = (vmr.m<<8) + (unsigned char)m[i];
	for (int i = 0; i < rsize; i++)
		vmr.r = (vmr.r<<8) + (unsigned char)r[i];

	return in;
}

std::ostream& operator<<(std::ostream &out, memItem &vmr) {
	size_t vsize = sizeof(vmr.v);
	size_t msize = sizeof(vmr.m);
	size_t rsize = sizeof(vmr.r);
	char v[vsize];
	char m[msize];
	char r[rsize];
	for (int i = 0; i < vsize; i++)
		v[vsize - 1 - i] = (vmr.v >> (i * 8));
	for (int i = 0; i < msize; i++)
		m[msize - 1 - i] = (vmr.m >> (i * 8));
	for (int i = 0; i < rsize; i++)
		r[rsize - 1 - i] = (vmr.r >> (i * 8));
	out.write(v, vsize);
	out.write(m, msize);
	out.write(r, rsize);
	return out;
}

// Hash function to enable use of IDj as a map index for the memoization table
namespace std {
template <>
	struct hash <IDj>{
	public :
		size_t operator()(const IDj &x ) const;
	};
	size_t hash<IDj>::operator()(const IDj &x ) const
	{
	        size_t h = std::hash<uint64_t>()(x.t.l[0]) ^ std::hash<uint64_t>()((x.t.l[1]<<2) ^ ((uint64_t)x.score[0]<<50) ^ ((uint64_t)x.score[1]<<56));
		return  h;
	}
}


std::string joc::id2str(const IDj &a) {
	std::stringstream out;
	out << "A=" << (int)a.t.c[0] << " " << (int)a.t.c[1] << " " << (int)a.t.c[2] << " " <<  (int)a.t.c[3] << " " <<  (int)a.t.c[4] << " " << (int)a.t.c[5] << " (" << (int)a.score[0] << ")" << std::endl;
	out << "B=" << (int)a.t.c[8] << " " << (int)a.t.c[9] << " " << (int)a.t.c[10] << " " <<  (int)a.t.c[11] << " " <<  (int)a.t.c[12] << " " << (int)a.t.c[13] << " (" << (int)a.score[1] << ")" << std::endl;
	return out.str();
}
int8_t joc::getMove() {
	return moviment;
}


IDj joc::getId(const signed char jug) {
	IDj id;
	for (int j=0; j<2; j++) {
		for (int i=0; i<6; i++) {
			id.t.c[i+j*8] = board[i][(jug+j)%2];
		}
	}
	id.score[0] = score[jug];
	id.score[1] = score[(jug+1)%2];
	return id;
}

bool joc::mou(short pos, signed char jug) {
	bool cangive = false;
	short fitxes = board[pos][jug];
	short fitxes_rival = 0;
	const signed char jug_ini = jug;
	const short pos_ini = pos;

	if (pos >= 6 || pos < 0 || fitxes <= 0) {
		return false;
	}

	for (int i=0; i<6; i++) {
		cangive = cangive || board[i][jug] > i;
	}

	board[pos][jug] = 0;
	
	while (fitxes > 0) {
		if (pos == 0) {
			pos = 5;
			jug = (jug+1)%2;
		}
		else {
			pos--;
		}

		if (pos != pos_ini || jug != jug_ini) {
			board[pos][jug]++;
			fitxes--;
		}
	}

	// Capture seeds 
	if (jug != jug_ini) {
		while (pos < 6 && (board[pos][jug] == 3 || board[pos][jug] == 2)) {
			score[jug_ini] += board[pos][jug];
			board[pos][jug] = 0;
			pos++;
		}
	}

	// Can't starve opponent 
	for (int i=0; i<6; i++) {
		fitxes_rival = fitxes_rival + board[i][(jug_ini+1)%2];
	}

	return !(fitxes_rival == 0 && cangive);
}

// Returns the heuristic value for the board, recursion modifier
// First parameter: player
// Second parameter: recursion level
// Third parameter: initial path (for speculative exploration)
std::pair<int, uint8_t> joc::ia(const signed char jug, uint8_t rec, const uint8_t path) {
	int value = INT_MIN;
	long long int anti_val, valor_actual;
	uint8_t anti_rec_mod, rec_mod = 0;
	short pos;
	
	IDj id = getId(jug);
	assert(memoize);
	mtx.lock_shared();
	if (memoize->find(id) != memoize->end()) {
#ifdef DEBUG
		mtx2.lock();
		used_memoizes++;
		mtx2.unlock();
#endif
		if (((*memoize)[id]).r >= rec) {
			moviment = ((*memoize)[id]).m;
			auto retval = ((*memoize)[id]).v;
			mtx.unlock_shared();
			return std::make_pair(retval, 0);
		}
	}
	mtx.unlock_shared();

	valor_actual = getValue(jug);
	if (rec <= 0) {
		return std::make_pair(valor_actual, 0);
	}

	if (valor_actual < PODA && rec < RECURSION_LEVEL) {
#ifdef DEBUG
		std::cout << "Poda realitzada" << std::endl;
#endif
		if (valor_actual - PODA_PENALTY < INT_MIN)
			return std::make_pair(INT_MIN, 0);
		else
			return std::make_pair(valor_actual - PODA_PENALTY,0);
	}

	for (pos=0; pos < 6; pos++) {
		if (board[pos][jug] > 0) {
			joc *c = this->copy();
			if (! c->mou(pos, jug) ) {
				delete c;
				continue;
			}
			auto res = c->ia((jug+1)%2, rec-1, path);
			anti_val = res.first;
			anti_rec_mod = res.second;
			int valor_actual_tmp = (valor_actual-anti_val>INT_MIN)? valor_actual-anti_val: INT_MIN;
			valor_actual = (valor_actual-anti_val<INT_MAX)? valor_actual_tmp: INT_MAX;
			if (valor_actual > value || moviment == -1) {
				moviment = pos;
				value = valor_actual;
				rec_mod += anti_rec_mod;
			}
			delete c;
		}
	}
	
#ifdef VERBOSE_DEBUG
	if (rec > 0) {
		std::cout << "rec=" << rec << std::endl;
		print();
		for(int i=RECURSION_LEVEL-rec; i>0; i--) {
			std::cout << " ";
		}
		if (jug == ME)
			std::cout << "ME  ";
		else
			std::cout << "THEM ";
		std::cout << value << std::endl;
	}
	
	/*if (rec >= RECURSION_LEVEL-1) {
		if (jug == ME)
			std::cout << "ME  ";
		else
			std::cout << "THEM ";
		std::cout << "(" << moviment+1 <<") " << value << std::endl;
	}*/
#endif

	if (moviment == -1) {
		// We may not find a move because we can't move without starving. If so, do whatever, as we are forced to starve opponent.
		if (board[0][jug] > 0) {
			moviment = 0;
		}
		else if (board[1][jug] > 0) {
			moviment = 1;
		}
		else if (board[2][jug] > 0) {
			moviment = 2;
		}
		else if (board[3][jug] > 0) {
			moviment = 3;
		}
		else if (board[4][jug] > 0) {
			moviment = 4;
		}
		else if (board[5][jug] > 0) {
			moviment = 5;
		}
		else {
			// If we can't move, the opponent gets every remaining piece on the board. 
			value=value-(MORTES_MULT)*(48-score[ME]-score[THEM]);
		}
	}

	assert(memoize);
	if (rec-rec_mod >= MIN_RECURSION)  {
		mtx.lock();
		if ((memoize->size() < MEMOIZE_MAX_SIZE)) {
			if ((memoize->find(id) == memoize->end()) || (memoize->find(id)->second).r < (rec-rec_mod)) {
				memItem valmovrec = (memItem){ .v=value, .m=moviment, .r=rec-rec_mod };
				(*memoize)[id] = valmovrec;

				std::cout << id2str(id) << "\t\tsize=" << memoize->size() << "\t\t-> "<< "\t" << (int)valmovrec.m << "\t" << (int)valmovrec.r << "\t(" << valmovrec.v << ")"  << std::endl << std::endl; 
				if (rand()%200 == 0) {
					std::cout << "Saving to memoize.dat... " ;
					// save to a file
					std::ofstream myfile;
					myfile.open("memoize.dat", std::ios::out| std::ios::binary);
					for (auto it = memoize->begin(); it != memoize->end(); it++) {
						IDj key;
						memItem valmovrec;
						key = it->first;
						valmovrec = it->second;
						myfile << key << valmovrec;
					}
					myfile.close();
					std::cout << "done" << std::endl;
				}
			}
		}
		mtx.unlock();
	}

	return std::make_pair(value, rec_mod);
}

joc* joc::copy() {
	joc *r = new joc();
	memcpy(r->board, this->board, 12*sizeof(signed char));
	r->moviment = -1;
	r->score[ME] = this->score[ME];
	r->score[THEM] = this->score[THEM];
	r->memoize = this->memoize;
	return r;
}
joc::~joc() {
}

int joc::getValue(const signed char jug) {
	const signed char altre = (jug+1)%2;
	int ret = 0;
	short i,j;
	int ventatja_fitxes = 0;
	int num_fitxes_jug = 0;
	int num_fitxes_altre = 0;

	for (j=0; j<2; j++) {
		for (i=0; i<6; i++) {
			// Heuristic 1: board ilegal 
			if (board[i][j] < 0) {
				return INT_MIN;
			}

			// Heuristic 2: 2 o 3 fitxes
			if (board[i][j] == 2 || board[i][j] == 1) {
				if (j == altre)
					ret += _12AWARD;
				else
					ret -= _12PENALTY;
			}

			// Heuristic 4: obligat a donar fitxes
			if (j == altre)
				num_fitxes_altre += board[i][j];
			else
				num_fitxes_jug += board[i][j];

			//Heuristic 8: espais buits
			if (j == jug && board[i][j] == 0)
				ret -= ESPAISBUITS_PENALTY;

			//Heuristic 9: Flow
			if (j == jug && i - board[i][j] > 0) {
				ret += FLOW_AWARD;
			}

			//Heuristic 10: 2 0 seguits
			if (i>0 && j==jug && board[i][j] == 0 && board[i-1][j] == 0) {
				ret -= DOBLEZERO_PENALTY;
			}

			// Heuristic 11: rival acumulant fitxes
			if (j == altre && board[i][j] > 11+i) {
				ret -= (board[i][j] - 11-i) * ACUM_PENALTY;
			}
		}
	}

	ret += score[jug] * MATADES_MULT;
	ret -= score[altre] * MORTES_MULT;

	// Heuristic 4. Si l'altre no pot moure, no es culpa nostra (controlat a mou())
	if (num_fitxes_altre == 0) {
		ret += num_fitxes_jug * MATADES_MULT;
	}

	// Heuristic 6: Partida perduda
	if (score[altre] >= 25)
		return (ret+INT_MIN/4 > INT_MIN)? ret+INT_MIN/4: INT_MIN+1;

	// Heuristic 7: Partida guanyada
	if (score[jug] >= 25)
		return (ret+INT_MAX/4 < INT_MAX)? ret+INT_MAX/4: INT_MAX-1;

	// Heuristic 3: mes fitxes que l'adversari
	ventatja_fitxes = num_fitxes_jug - num_fitxes_altre;
	ret += ventatja_fitxes * MESFITXES_MULT;

	return ret;
}

void joc::print() {
	int i;

#if BOARD_MODE == ME
/*	std::cout << "1 2 3 4 5 6" << std::endl;
	std::cout << "-----------" << std::endl;*/
	for (i=0; i<6; i++) {
		std::cout << (signed)board[i][THEM] <<" ";
	}
	std::cout << std::endl;
	for (i=5; i>=0; i--) {
		std::cout << (signed)board[i][ME] <<" ";
	}
#else
	for (i=0; i<6; i++) {
		std::cout << (signed)board[i][ME] <<" ";
	}
	std::cout << std::endl;
	for (i=5; i>=0; i--) {
		std::cout << (signed)board[i][THEM] <<" ";
	}
#endif
	std::cout << std::endl;
	std::cout << "Score: ME=" << (int)score[ME] << " THEM=" << (int)score[THEM] << std::endl;
}

void joc::ini() {
	short i,j;

	for (i=0; i<6; i++) {
		for (j=0; j<2; j++) {
			board[i][j] = 4;
		}
	}
	score[ME] = 0;
	score[THEM] = 0;
}


std::unordered_map<IDj, memItem >* load_memoize(std::string filename) {
	std::unordered_map<IDj, memItem > *ret = new std::unordered_map<IDj, memItem >;
	IDj key;
	memItem valmovrec;
	std::ifstream file;
	file.open(filename, std::ios::in |std::ios::binary);
	std::cout << "Loading from " << filename << "..." << std::endl;;
	while (!file.eof() && !file.fail()) {
		file >> key >> valmovrec;
		// easy fast check to detect data corruption
		if (!file.fail() && valmovrec.m >= 0 && valmovrec.m < 6) {
			(*ret)[key] = valmovrec;
		}
	}
	std::cout << ret->size() << " values loaded." << std::endl;
	return ret;
}

int main(int argc, char**argv) {
	short pos;
	joc t;
	joc *spec[6];
	std::thread *th[6];
	clock_t time;
	short last_eviction=0; // indica quin quart de cache de memoització toca esborrar
	int c;
	char *memoize_file = NULL;

	MATADES_MULT = 0;
	MORTES_MULT = 0;
	while ((c = getopt (argc, argv, "a:d:m:")) != -1) {
		switch(c) {
			case 'd':
				MORTES_MULT = atoi(optarg);
				break;
			case 'a':
				MATADES_MULT = atoi(optarg);
				break;
			case 'm':
				memoize_file = (char*)malloc(sizeof(optarg));
				strcpy(memoize_file, optarg);

		}
	}

	if (!memoize_file) {
		memoize_file = (char*)malloc(sizeof("memoize.dat"));
		strcpy(memoize_file, "memoize.dat");
	}
	
	srand(clock());
	if (MATADES_MULT == 0)
		MATADES_MULT = rand() % 150 + 250;
	if (MORTES_MULT == 0)
		MORTES_MULT = rand() % 150 + 250;


#ifdef DEBUG
	std::cout << "Agression level: " << MATADES_MULT << "/" << MORTES_MULT << std::endl << std::endl;
#endif

	t.ini();
	t.memoize = load_memoize(memoize_file);
	std::cout << std::endl;

	// multithread while waiting for rival 
	for (int i=0; i<6; i++) {
		// Generate 6 games with each possible play by the opponent. Launch threaded execution.
		spec[i] = t.copy();
		if (spec[i]->mou(i, THEM)) {
			th[i] = new std::thread(&joc::ia, spec[i], ME, RECURSION_LEVEL, i+1);
		}
		else {
			// Illegal move, discard
			th[i] = NULL;
		}
	}

	time = clock();

	t.print();
	// Speculative boards can be deleted now, as we will create them anew the next iteration.
	std::cout << "Their move [1-6]: ";
	for (int test_all=0; test_all<6; test_all++) {
		if (th[test_all]) {
			th[test_all]->join();
		}
		else {
			continue;
		}
		auto *t2 = spec[test_all]; 
		bool ok = t2->mou(test_all, THEM);
		if (ok)  {
			jugada++;
			t2->print();
			// Join 'pos' thread. Kill the rest later.
			assert(t2->mou(spec[test_all]->getMove(), ME));

			std::cout << "Move from ";
			std::cout << spec[pos-1]->getMove()+1 << std::endl;
			jugada++;

			std::cout << "Memoize table size: " << t2->memoize->size() << std:: endl;;
#ifdef DEBUG
			std::cout << "Memoized values used: " << used_memoizes << std::endl;
#endif

		}
		delete t2;
	}
	for (int i=0; i<6; i++) {
		if (spec[i]) {
			delete spec[i];
			spec[i] = NULL;
		}
	}
}
