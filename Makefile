CXX=g++
CFLAGS=-O2 -std=c++1z
LDFLAGS=-pthread
# Debug flags: DEBUG, VERBOSE_DEBUG
CFLAGS_DBG=-g -std=c++1z -DDEBUG

# PRINT_MODE: HUMAN, MACHINE
DEFINES=-DPRINT_MODE=HUMAN -DMEMOIZE_ENABLED

all: borjilator


borjilator: borjilator.cpp 
	$(CXX) $(DEFINES) $(CFLAGS) $^ $(LDFLAGS) -o $@

borjilator-dbg: borjilator.cpp
	$(CXX) $(DEFINES) $(CFLAGS_DBG) $(LDFLAGS) $^ -o $@

borjilator-gprof: borjilator.cpp
	$(CXX) -pg $(DEFINES) $(CFLAGS) $(LDFLAGS) $^ -o $@

memtrainer: memtrainer.cpp
	$(CXX) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f borjilator borjilator-dbg borjilator-gprof memtrainer
