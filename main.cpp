#include "pars.hpp"

int main() {
    pars::Context ctx;

    ctx.exec(R"(
(define (fact1 x)
  (if (= x 0) 1
      (* x (fact1 (- x 1)))))

(define (fact2 x)
  (define (fact-tail x accum)
    (if (= x 0) accum
        (fact-tail (- x 1) (* x accum))))
  (fact-tail x 1))
    )", true);

    ctx.print(ctx.exec("(fact1 10)", true));
    ctx.print(ctx.exec("(fact2 10)", true));

    ctx.repl();
}
