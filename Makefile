MAIN=pars

GEN_SRCS=builtins/generated.cpp
BUILTIN_SRCS=$(filter-out $(GEN_SRCS),$(wildcard builtins/*.cpp))
SRCS=$(wildcard *.cpp) $(BUILTIN_SRCS)
OBJS=$(patsubst %.cpp,%.o,$(SRCS) $(GEN_SRCS))
CFLAGS=-std=c++11 -g -Wall -Wextra -Werror

$(MAIN): $(GEN_SRCS) $(OBJS)
	$(CXX) $(CFLAGS) -o $(MAIN) $(OBJS)

.cpp.o:
	$(CXX) $(CFLAGS) -o $@ -c $<

test: pars test.pars
	./pars test.pars

check: pars test.pars
	valgrind --leak-check=full ./pars test.pars

run: pars
	./pars

$(GEN_SRCS): $(BUILTIN_SRCS)
	./genbuiltins.sh

clean:
	rm $(MAIN)
	rm *.o
	rm builtins/*.o
	rm $(GEN_SRCS)

.PHONY: clean
