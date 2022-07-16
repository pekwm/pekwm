.POSIX:

MK       = ../mk

include $(MK)/config.mk

PROGS = check

CHECK_OBJS = check.o

all: $(PROGS)

check: $(CHECK_OBJS)
	$(LD) -o check $(CHECK_OBJS) $(LDFLAGS)

clean:
	rm -f $(PROGS) *.o

.SUFFIXES: .cc .o
.cc.o:
	$(CXX) $(CXXFLAGS) -c $<
