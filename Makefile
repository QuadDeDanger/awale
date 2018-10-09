CXX=g++
CFLAGS=-O2 -std=c++1z
LDFLAGS=-pthread
CFLAGS_DBG=-g -std=c++1z -DDEBUG

all: borjilator borjilator_dbg


borjilator: borjilator.cpp 
	$(CXX) $(CFLAGS) $^ $(LDFLAGS) -o $@

borjilator_dbg: borjilator.cpp
	$(CXX) $(CFLAGS_DBG) $^ $(LDFLAGS) -o $@ 

clean:
	rm borjilator borjilator_dbg