#include "pars.hpp"

int main(int argc, char **argv) {
    pars::Context ctx;

    if (argc > 1) {
        for (int i = 1; i < argc; i++)
            ctx.exec_file(argv[i], true);
    } else {
        ctx.repl();
    }

    return 0;
}
