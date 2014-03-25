#include <cstdio>
#include "pars.hpp"

int main(int argc, char **argv) {
    pars::Context ctx;

    if (argc > 1) {
        pars::Value args = pars::nil;

        for (int i = argc - 1; i >= 2; i--)
            args = ctx.cons(ctx.str(argv[i]), args);

        ctx.define("argv", args);

        ctx.exec_file((const char *)argv[1], true);
    } else {
        ctx.repl();
    }

    return 0;
}
