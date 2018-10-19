// Identifiers for self and rival
#define THEM 0
#define ME 1

// Print mode, stdio for human vs IA, piped for IA vs IA
#define HUMAN 0
#define MACHINE 1

#define BOARD_MODE THEM 

#ifndef PRINT_MODE
#error Please define PRINT_MODE to HUMAN or MACHINE in your Makefile
#endif

// MIN_RECURSION: minimum number of recursions before we allow killing the thread
// also servers as a threshold for the memoization table
#define MIN_RECURSION 8
#define MAX_RECURSION 19
// TIME_BUDGET_HINT: time limit before we start to finish the process. In ms
#define TIME_BUDGET_HINT 1000
#define TIME_BUDGET_MIN TIME_BUDGET_HINT 
#define TIME_BUDGET_MAX (TIME_BUDGET_MIN*1.5)

#define MEMOIZE_MAX_SIZE 50000000

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

