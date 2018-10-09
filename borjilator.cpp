// Identifiers for self and rival
#define ELL 0
#define JO 1

// Print mode, stdio for human vs IA, piped for IA vs IA
#define HUMAN 0
#define MACHINE 1

#define BOARD_MODE ELL 
#define PRINT_MODE HUMAN
//#define PRINT_MODE MACHINE

#include <climits>
#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <iterator>
#include <thread>
#include <shared_mutex>
#include <cstdlib>
#include <string.h>
#include <time.h>
#include <getopt.h>

#if PRINT_MODE == MACHINE
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#endif
int RECURSION_LEVEL = 13;
int MATADES_MULT = 300;
int MORTES_MULT = 400;
// MIN_RECURSION: numero minim de recursions restants per permetre matar el thread
#define MIN_RECURSION 10
#define MAX_RECURSION 19
// TIME_BUDGET_HINT: time limit before we start to finish the process. In ms
#define TIME_BUDGET_HINT 60000
#define TIME_BUDGET_MIN TIME_BUDGET_HINT 
#define TIME_BUDGET_MAX (TIME_BUDGET_MIN*1.5)

//#define MEMOIZE_MAX_SIZE 250000
#define MEMOIZE_MAX_SIZE 50000000
//#define MEMOIZE_MAX_SIZE 20000000

#define _12AWARD 1
#define _12PENALTY 1
#define MESFITXES_MULT 1
#define ESPAISBUITS_PENALTY 5
#define FLOW_AWARD 5
#define DOBLEZERO_PENALTY 10
#define ACUM_PENALTY 16
#define PODA (INT_MIN/2)
#define PODA_PENALTY 0 

#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)

static short g_terminate = 0;
static int jugada = 1;

#ifdef DEBUG
static uint64_t used_memoizes = 0;
static std::mutex mtx2;
#endif

static std::shared_mutex mtx;

typedef union ID {
	uint64_t l[2];
	uint32_t i[4];
	uint16_t s[8];
	uint8_t c[16];
} ID;

class IDj {
	public:
	ID t;
	friend bool operator==(IDj &a, IDj &b);
	friend bool operator!=(IDj &a, IDj &b);
	IDj() {
		t.l[0] = 0;
		t.l[1] = 0;
	}
};
bool operator==(IDj &a, IDj &b){
	return (a.t.l[0] == b.t.l[0] && a.t.l[1] == b.t.l[1]);
}
bool operator==(const IDj &a, const IDj &b){
	return (a.t.l[0] == b.t.l[0] && a.t.l[1] == b.t.l[1]);
}
bool operator!=(IDj &a, IDj &b){
	return !(a.t.l[0] == b.t.l[0] && a.t.l[1] == b.t.l[1]);
}
std::istream& operator>>(std::istream &in, IDj &a) {
	in.read((char*)a.t.c, 6*sizeof(uint8_t));
	in.read((char*)&a.t.c[8], 6*sizeof(uint8_t));
	//std::cout << "C=" << (int)a.t.c[0] << " " << (int)a.t.c[1] <<  (int)a.t.c[2] << " " <<  (int)a.t.c[3] << " " <<  (int)a.t.c[4] << " " << (int)a.t.c[5] << " " << std::endl;
	//std::cout << "C=" << (int)a.t.c[8] << " " << (int)a.t.c[9] <<  (int)a.t.c[10] << " " <<  (int)a.t.c[11] << " " <<  (int)a.t.c[12] << " " << (int)a.t.c[13] << " " << std::endl;
	return in;
}
std::ostream& operator<<(std::ostream &out, IDj &a) {
	out.write((char*)a.t.c, 6*sizeof(uint8_t));
	out.write((char*)&a.t.c[8], 6*sizeof(uint8_t));
	//std::cout << "C=" << (int)a.t.c[0] << " " << (int)a.t.c[1] <<  (int)a.t.c[2] << " " <<  (int)a.t.c[3] << " " <<  (int)a.t.c[4] << " " << (int)a.t.c[5] << " " << std::endl;
	//std::cout << "C=" << (int)a.t.c[8] << " " << (int)a.t.c[9] <<  (int)a.t.c[10] << " " <<  (int)a.t.c[11] << " " <<  (int)a.t.c[12] << " " << (int)a.t.c[13] << " " << std::endl;
	return out;
}
/*std::ostream& operator<<(std::ostream &out, const IDj &a) {
	//out.write((char*)a.t.c, 6*sizeof(uint8_t));
	//out.write((char*)&a.t.c[8], 6*sizeof(uint8_t));
	out << "C=" << (int)a.t.c[0] << " " << (int)a.t.c[1] <<  (int)a.t.c[2] << " " <<  (int)a.t.c[3] << " " <<  (int)a.t.c[4] << " " << (int)a.t.c[5] << " " << std::endl;
	out << "C=" << (int)a.t.c[8] << " " << (int)a.t.c[9] <<  (int)a.t.c[10] << " " <<  (int)a.t.c[11] << " " <<  (int)a.t.c[12] << " " << (int)a.t.c[13] << " " << std::endl;
	return out;
}*/
std::istream& operator>>(std::istream &in, std::pair<int,short> &vm) {
	char v[4];
	char m[2];
	vm.first = 0;
	vm.second = 0;
	in.read(v, sizeof(int));
	in.read(m, sizeof(short));
	for (int i = 0; i < sizeof(int); i++)
		vm.first = (vm.first<<8) + (unsigned char)v[i];
	for (int i = 0; i < sizeof(short); i++)
		vm.second = (vm.second<<8) + (unsigned char)m[i];

//	std::cout << vm.first << " " << vm.second << std::endl;
	return in;
}
std::ostream& operator<<(std::ostream &out, std::pair<int,short> &vm) {
//	std::cout << vm.first << " " << vm.second << std::endl;
	char v[4];
	char m[2];
	for (int i = 0; i < sizeof(int); i++)
		v[3 - i] = (vm.first >> (i * 8));
	for (int i = 0; i < sizeof(short); i++)
		m[1 - i] = (vm.second >> (i * 8));
	out.write(v, sizeof(int));
	out.write(m, sizeof(short));
	return out;
}

// Operació de hash per a poder fer servir IDj com a index del map de memoitzacio
namespace std {
template <>
    struct hash <IDj>{
    public :
        size_t operator()(const IDj &x ) const;
    };
    size_t hash<IDj>::operator()(const IDj &x ) const
    {
        size_t h = std::hash<uint64_t>()(x.t.l[0]) ^ std::hash<uint64_t>()(x.t.l[1] << 16);
        return  h;
    }
}



class joc {
	public:
		signed char taulell[6][2];
		int punts[2];
		bool mou(short pos, signed char jug);
		int ia(const signed char jug, unsigned short rec, const uint8_t path=0);
		joc* copy();
		short getMove();
		void print();
		void ini();
		IDj getId(const signed char jug);
		std::unordered_map<IDj, std::pair<int, short> > *memoize;
		clock_t start_time;
	private:
		short moviment;
		int getValue(const signed char jug);
};

short joc::getMove() {
	return moviment;
}


IDj joc::getId(const signed char jug) {
	IDj id;
	for (int j=0; j<2; j++) {
		for (int i=0; i<6; i++) {
			id.t.c[i+j*8] = taulell[i][(jug+j)%2];
		}
	}
	return id;
}

bool joc::mou(short pos, signed char jug) {
	if (pos >= 6 || pos < 0) {
		return false;
	}

	short fitxes = taulell[pos][jug];

	if (fitxes <= 0) {
		return false;
	}

	const signed char jug_ini = jug;
	const short pos_ini = pos;


	taulell[pos][jug] = 0;
	
	while (fitxes > 0) {
		if (pos == 0) {
			pos = 5;
			jug = (jug+1)%2;
		}
		else {
			pos--;
		}

		if (pos != pos_ini || jug != jug_ini) {
			taulell[pos][jug]++;
			fitxes--;
		}
	}

	// Matar
	if (jug != jug_ini) {
		while (pos <6 && (taulell[pos][jug] == 3 || taulell[pos][jug] == 2)) {
			punts[jug_ini] += taulell[pos][jug];
			taulell[pos][jug] = 0;
			pos++;
		}
	}


	return true;
}

int joc::ia(const signed char jug, unsigned short rec, const uint8_t path) {
	int value = INT_MIN;
	long long int anti_val, valor_actual;
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
		moviment = (*memoize)[id].second;
		auto retval= (*memoize)[id].first;
		mtx.unlock_shared();
		return retval;
	}
	mtx.unlock_shared();

	valor_actual = getValue(jug);
	if (rec <= 0) {
		return valor_actual;
	}
	if (g_terminate != 0 && g_terminate != path) {
		return valor_actual;
	}

	if (valor_actual < PODA && rec < RECURSION_LEVEL) {
#ifdef DEBUG
		std::cout << "Poda realitzada" << std::endl;
#endif
		if (valor_actual - PODA_PENALTY < INT_MIN)
			return INT_MIN;
		else
			return valor_actual - PODA_PENALTY;
	}

	auto time = clock();
	if (rec > MIN_RECURSION && (time-start_time)*1000/CLOCKS_PER_SEC > TIME_BUDGET_HINT) {
		rec--;
	} else if (rec > 1 && (time-start_time)*1000/CLOCKS_PER_SEC > TIME_BUDGET_MAX) {
		rec--;
	}

	for (pos=0; pos < 6; pos++) {
		if (taulell[pos][jug] > 0) {
			joc *c = this->copy();
			if (! c->mou(pos, jug) ) {
				delete c;
				continue;
			}
			anti_val = c->ia((jug+1)%2, rec-1, path);
			int valor_actual_ = (valor_actual-anti_val>INT_MIN)? valor_actual-anti_val: INT_MIN;
			int valor_actual = (valor_actual-anti_val<INT_MAX)? valor_actual_: INT_MAX;
			if (valor_actual > value || moviment == -1) {
				moviment = pos;
				value = valor_actual;

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
		if (jug == JO)
			std::cout << "JO  ";
		else
			std::cout << "ELL ";
		std::cout << value << std::endl;
	}
	
	/*if (rec >= RECURSION_LEVEL-1) {
		if (jug == JO)
			std::cout << "JO  ";
		else
			std::cout << "ELL ";
		std::cout << "(" << moviment+1 <<") " << value << std::endl;
	}*/
#endif

	if (moviment == -1) {
		// Ho tornem a mirar, per si un cas. Això no cal i es pot esborrar.
		if (taulell[0][jug] > 0)
			moviment = 0;
		else if (taulell[1][jug] > 0)
			moviment = 1;
		else if (taulell[2][jug] > 0)
			moviment = 2;
		else if (taulell[3][jug] > 0)
			moviment = 3;
		else if (taulell[4][jug] > 0)
			moviment = 4;
		else if (taulell[5][jug] > 0)
			moviment = 5;
		else
//			value=INT_MAX/4; // Victoria perque no podem fer moviments? Depen de si l'altre estava obligat o no a donar-nos fitxes
			value=0; // Victoria perque no podem fer moviments? Depen de si l'altre estava obligat o no a donar-nos fitxes
	}

	assert(memoize);
	if (rec >= MIN_RECURSION)  {
		mtx.lock();
		if ((memoize->size() < MEMOIZE_MAX_SIZE) && (memoize->find(id) == memoize->end())) {
			std::pair<int, short> p (value, moviment);
			(*memoize)[id] = p;
		}
		mtx.unlock();
	}

	return value;
}

joc* joc::copy() {
	joc *r = new joc();
	memcpy(r->taulell, this->taulell, 12*sizeof(signed char));
	r->moviment = -1;
	r->punts[JO] = this->punts[JO];
	r->punts[ELL] = this->punts[ELL];
	r->memoize = this->memoize;
	r->start_time = this->start_time;
	return r;
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
			// Heuristic 1: taulell ilegal 
			if (taulell[i][j] < 0) {
				return INT_MIN;
			}

			// Heuristic 2: 2 o 3 fitxes
			if (taulell[i][j] == 2 || taulell[i][j] == 1) {
				if (j == altre)
					ret += _12AWARD;
				else
					ret -= _12PENALTY;
			}

			// Heuristic 4: obligat a donar fitxes
			if (j == altre)
				num_fitxes_altre += taulell[i][j];
			else
				num_fitxes_jug += taulell[i][j];

			//Heuristic 8: espais buits
			if (j == jug && taulell[i][j] == 0)
				ret -= ESPAISBUITS_PENALTY;

			//Heuristic 9: Flow
			if (j == jug && i - taulell[i][j] > 0) {
				ret += FLOW_AWARD;
			}

			//Heuristic 10: 2 0 seguits
			if (i>0 && j==jug && taulell[i][j] == 0 && taulell[i-1][j] == 0) {
				ret -= DOBLEZERO_PENALTY;
			}

			// Heuristic 11: rival acumulant fitxes
			if (j == altre && taulell[i][j] > 11+i) {
				ret -= (taulell[i][j] - 11-i) * ACUM_PENALTY;
			}


		}
	}

	ret += punts[jug] * MATADES_MULT;
	ret -= punts[altre] * MORTES_MULT;

	// Heuristic 4
	if (num_fitxes_altre == 0) {
		// Primer mirem si estem obligats a donar
		bool couldhaveplayed=false;
		for (int idx=0; idx<6; idx++) {
			couldhaveplayed=couldhaveplayed || (taulell[i][jug] > i);
		}
		if (couldhaveplayed) {
			if (punts[JO] < 25) {
				return (ret+INT_MIN/4 > INT_MIN)? ret+INT_MIN/4: INT_MIN+1;
			}
			else {
				ret -= (48 - punts[jug] - punts[altre]) * MORTES_MULT;
			}
		}
		else {
			ret += num_fitxes_jug * MATADES_MULT;
		}
	}


	// Heuristic 6: Partida perduda
	if (punts[altre] >= 25)
		//ret -= punts[altre] * MORTES_MULT;
		return (ret+INT_MIN/4 > INT_MIN)? ret+INT_MIN/4: INT_MIN+1;

	// Heuristic 7: Partida guanyada
	if (punts[jug] >= 25)
		//ret += punts[jug] * MATADES_MULT;
		return (ret+INT_MAX/4 < INT_MAX)? ret+INT_MAX/4: INT_MAX-1;

	// Heuristic 3: mes fitxes que l'adversari
	ventatja_fitxes = num_fitxes_jug - num_fitxes_altre;
	ret += ventatja_fitxes * MESFITXES_MULT;

	return ret;
}


void joc::print() {
	int i;

#if BOARD_MODE == JO
/*	std::cout << "1 2 3 4 5 6" << std::endl;
	std::cout << "-----------" << std::endl;*/
	for (i=0; i<6; i++) {
		std::cout << (signed)taulell[i][ELL] <<" ";
	}
	std::cout << std::endl;
	for (i=5; i>=0; i--) {
		std::cout << (signed)taulell[i][JO] <<" ";
	}
#else
	for (i=0; i<6; i++) {
		std::cout << (signed)taulell[i][JO] <<" ";
	}
	std::cout << std::endl;
	for (i=5; i>=0; i--) {
		std::cout << (signed)taulell[i][ELL] <<" ";
	}
/*	std::cout << std::endl << "-----------";
	std::cout << std::endl << "6 5 4 3 2 1";*/
#endif
	std::cout << std::endl;
	std::cout << "Punts: JO=" << punts[JO] << " ELL=" << punts[ELL] << std::endl;
}

void joc::ini() {
	short i,j;

	for (i=0; i<6; i++) {
		for (j=0; j<2; j++) {
			taulell[i][j] = 4;
		}
	}
	punts[JO] = 0;
	punts[ELL] = 0;

	start_time = clock();
}


std::unordered_map<IDj, std::pair<int,short> >* load_memoize(std::string filename) {
	std::unordered_map<IDj, std::pair<int, short> > *ret = new std::unordered_map<IDj, std::pair<int, short> >;
	IDj key;
	std::pair <int, short> valmov;
	std::ifstream file;
	file.open(filename, std::ios::in |std::ios::binary);
	std::cout << "Loading from " << filename << "..." << std::endl;;
	while (!file.eof() && !file.fail()) {
		file >> key >> valmov;
		(*ret)[key] = valmov;
	}
	std::cout << ret->size() << " values loaded." << std::endl;
	return ret;
}

int main(int argc, char**argv) {
	short pos;
	joc t;
	joc *spec[6];
	std::thread *th[6];
	std::string primer = "";
	clock_t time;
	short last_eviction=0; // indica quin quart de cache de memoització toca esborrar
	int c;

	MATADES_MULT = 0;
	MORTES_MULT = 0;
	while ((c = getopt (argc, argv, "p:a:d:")) != -1) {
		switch(c) {
			case 'p':
				if (!strcmp(optarg, "JO"))
					primer = "JO";
				else 
					primer = "ELL";
				break;
			case 'd':
				MORTES_MULT = atoi(optarg);
				break;
			case 'a':
				MATADES_MULT = atoi(optarg);
				break;
		}

	}
	
	srand(clock());
	if (MATADES_MULT == 0)
		MATADES_MULT = rand() % 150 + 250;
	if (MORTES_MULT == 0)
		MORTES_MULT = rand() % 150 + 250;


#ifdef DEBUG
	std::cout << "Nivell d'agressivitat: " << MATADES_MULT << "/" << MORTES_MULT << std::endl << std::endl;
#endif

	t.ini();
	t.memoize = load_memoize("memoize.dat");

	if (primer == "") {
		std::cout << "Qui comença? [JO/ELL] ";
		std::cin >> primer;
	}
#if PRINT_MODE == MACHINE
	int pipe_r, pipe_w;
	const char *fifo1= "/tmp/awale1";
	const char *fifo2= "/tmp/awale2";
	mkfifo(fifo1, 0666);
	mkfifo(fifo2, 0666);
	if (primer == "JO") {
		pipe_r = open(fifo1, O_RDONLY);
		pipe_w = open(fifo2, O_WRONLY);
	}
	else {
		pipe_w = open(fifo1, O_WRONLY);
		pipe_r = open(fifo2, O_RDONLY);
	}
	if (!pipe_r || !pipe_w)
		exit(1);
#endif


	if (primer == "JO") {
		t.print();
		t.ia(JO, RECURSION_LEVEL);
		std::cout << "Mou desde ";
		std::cout << t.getMove()+1 << std::endl;
#if PRINT_MODE == MACHINE
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", t.getMove()+1);
		write(pipe_w, buf, sizeof(buf));
#endif
		if (!t.mou(t.getMove(), JO)) {
			std::cout << "Posició incorrecta: " << (int)pos+1 << std::endl;
		}
		jugada++;
	}

	for(;;) {
		g_terminate = 0;
#if PRINT_MODE == MACHINE
		if (t.punts[JO] > 24 || t.punts[ELL] > 24) {
			t.print();
			exit(0);
		}
#endif

		// multithread mentre esperem
		for (int i=0; i<6; i++) {
			// fem 6 taulells, un per cada moviment possible que pot fer el rival. Per cada un d'ells, obtenim el millor resultat possible per a nosaltres.
			spec[i] = t.copy();
			spec[i]->start_time = clock();
			if (spec[i]->mou(i, ELL)) {
				th[i] = new std::thread(&joc::ia, spec[i], JO, RECURSION_LEVEL, i+1);
			}
			else {
				th[i] = NULL;
			}
		}

		// Guardem la taula al final de cada torn
		std::ofstream myfile;
		myfile.open("memoize.dat", std::ios::out| std::ios::binary);
		for (auto it = t.memoize->begin(); it != t.memoize->end(); it++) {
			IDj key;
			std::pair<int,short> valmov;
			key = it->first;
			valmov.first = it->second.first;
			valmov.second = it->second.second;
			myfile << key << valmov;
		}
		myfile.close();

		time = clock();

		t.print();
		std::cout << "Posició del seu moviment: ";
#if PRINT_MODE == HUMAN
		std::cin >> pos;
#endif
#if PRINT_MODE == MACHINE
		char buf[16];
		read(pipe_r, buf, 16);
		pos = atoi(buf);
		std::cout << pos << std::endl;
#endif
		bool ok = t.mou(pos-1, ELL);
		if (ok)  {
			jugada++;
			t.print();
			// Intentem proporcionar el resultat abans de que finalitzin els demés threads
			g_terminate = pos;
			th[pos-1]->join();
			t.mou(spec[pos-1]->getMove(), JO);

			//Condició d'acabament
			if (spec[pos-1]->getMove() == - 1) {
				int fitxes_fi = 0;
				std::cout << std::endl;
				std::cout << "Sense moviments possibles.";
				for (int i=0; i<6; i++) {
					if (t.taulell[i][ELL] > i) {
						// Victoria
						std::cout << "VICTORIA per moviment ilegal del rival.";
						exit(0);
					}
					fitxes_fi += t.taulell[i][ELL];
				}
				t.punts[ELL] += fitxes_fi; 
				std::cout << "Punts: JO=" << t.punts[JO] << " ELL=" << t.punts[ELL] << std::endl;
				exit(0);
			}

			std::cout << "Mou desde ";
			std::cout << spec[pos-1]->getMove()+1 << std::endl;
#if PRINT_MODE == MACHINE
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", spec[pos-1]->getMove()+1);
		write(pipe_w, buf, sizeof(buf));
#endif
			jugada++;
			// ... i esperem a la resta de threads per continuar
			for (int i=pos%6; i!=pos-1; i=(i+1)%6) {
				if (th[i])
					th[i]->join();
			}

			// Calcul de temps
			int ms = (clock()-time)*1000/CLOCKS_PER_SEC;
#ifdef DEBUG
			std::cout << "Calculat en " << ms << " milisegons. " << std:: endl;;
			std::cout << "Memoize table size: " << t.memoize->size() << std:: endl;;
#endif
			if (ms < TIME_BUDGET_MIN) {
				RECURSION_LEVEL = MIN(RECURSION_LEVEL+1, MAX_RECURSION);
#ifdef DEBUG
				std::cout << "Incrementant dificultat (" << RECURSION_LEVEL << ")" << std::endl;
#endif
			}
			if (ms > TIME_BUDGET_MAX) {
				RECURSION_LEVEL--;
#ifdef DEBUG
				std::cout << "Disminuint dificultat (" << RECURSION_LEVEL << ")" << std::endl;
#endif
			}
#ifdef DEBUG
			std::cout << "Memoizes utilitzats: " << used_memoizes << std::endl;
#endif

			// Anem fent espai a la taula de memoització per acomodar noves jugades i anar eliminar les branques impossibles (random)
			if (t.memoize->size() >= MEMOIZE_MAX_SIZE) {
				auto quart = std::distance(t.memoize->begin(),t.memoize->end()) / 4;
				auto it1 = t.memoize->begin();
				auto it2 = t.memoize->begin();
				auto it3 = t.memoize->begin();
				switch (last_eviction) {
				case 0:
					std::advance(it1, quart);
					t.memoize->erase(t.memoize->begin(), it1);
					break;
				case 1:
					std::advance(it1, quart);
					std::advance(it2, quart*2);
					t.memoize->erase(it1, it2);
					break;
				case 2:
					std::advance(it2, quart*2);
					std::advance(it3, quart*3);
					t.memoize->erase(it2, it3);
					break;
				case 3:
					std::advance(it3, quart*3);
					t.memoize->erase(it3, t.memoize->end());
					break;
				}
				last_eviction = (last_eviction + 1) % 4;
			}
#ifdef DEBUG
			//used_memoizes = 0;
#endif
			// els taulells especulatius son inutils ja, la propera iteració en crearem de nous a partir de copies de l'actual.
			for (int i=0; i<6; i++) {
				if (spec[i]) {
					delete spec[i];
					spec[i] = NULL;
				}
			}
		}
	}
}
