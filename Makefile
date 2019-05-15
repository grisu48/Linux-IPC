

# Default compiler and compiler flags
CXX=g++
CC=gcc

# Default flags for all compilers
O_FLAGS=-O3 -Wall -Werror -Wextra -pedantic
# Debugging flags
#O_FLAGS=-Og -g2 -Wall -Werror -Wextra -pedantic
CXX_FLAGS=$(O_FLAGS) -std=c++11
CC_FLAGS=$(O_FLAGS) -std=c99


# Binaries, object files, libraries and stuff
LIBS=
INCLUDE=
OBJS=ipc.o
BINS=example


# Default generic instructions
default:	all
all:	$(OBJS) $(BINS)
clean:	
	rm -f *.o
# Object files
%.o:	%.cpp %.hpp
	$(CXX) $(CXX_FLAGS) -c $(INCLUDE) -o $@ $< $(LIBS) 
	
example:	example.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) $(INCLUDE) -o $@ $< $(OBJS) $(LIBS) 
