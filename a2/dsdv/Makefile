#
# The compiler and its flags
#
CXX= g++ 
CXXFLAGS= -g -Wall
LIBS= -lnsl
#
# Sources and objects
#

PROGRAM=dsdv
SRCS=dsdv.cpp
OBJS=$(SRCS:.cpp=.o)

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	@echo "Removing objects..."
	rm -f $(PROGRAM) *.o *~ core

