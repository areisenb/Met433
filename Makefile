
# Defines the RPI variable which is needed by rc-switch/RCSwitch.h
CXXFLAGS=-DRPI

all: Met433

simulate: CXXFLAGS+= -DMOCKWIRINGPI
simulate: Met433.o sequencer.o mockPi/mockWiringPi.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lpthread

Met433: Met433.o sequencer.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi -lwiringPiDev -lcrypt

clean:
	$(RM) *.o Met433 simulate

