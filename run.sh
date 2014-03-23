./genbuiltins.sh > builtins/generated.cpp
g++ -std=c++11 -g -Wall -Wextra -Werror *.cpp builtins/*.cpp -o pars && ./pars test.pars
