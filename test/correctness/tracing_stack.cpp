
// This test demonstrates using tracing to give you something like a
// stack trace in case of a crash (due to a compiler bug, or a bug in
// external code). We use a posix signal handler, which is probably
// os-dependent, so I'm going to enable this test on linux only.

#ifdef __linux__

#include <Halide.h>
#include <stdio.h>
#include <signal.h>
#include <stack>
#include <string>

using namespace Halide;
using std::stack;
using std::string;

stack<string> stack_trace;

void my_trace(const char *function, int event_type,
              int type_code, int bits, int width,
              int value_index, const void *value,
              int num_int_args, const int *int_args) {

    const string event_types[] = {"Load ",
                                  "Store ",
                                  "Begin realization ",
                                  "End realization ",
                                  "Produce ",
                                  "Update ",
                                  "Consume ",
                                  "End consume "};

    if (event_type == 3 || event_type > 4) {
        // These events signal the end of some previous event
        stack_trace.pop();
    }
    if (event_type == 2 || event_type == 4 || event_type == 5 || event_type == 6) {
        // These events signal the start of some new region
        stack_trace.push(event_types[event_type] + function);
    }
}

void signal_handler(int signum) {
    printf("Correctly triggered a segfault. Here is the stack trace:\n");
    while (!stack_trace.empty()) {
        printf("%s\n", stack_trace.top().c_str());
        stack_trace.pop();
    }

    printf("Success!\n");
    exit(0);
}


int main(int argc, char **argv) {

   signal(SIGSEGV, signal_handler);

    // Loads from this image will barf, because we've messed up the host pointer
    Image<int> input(100, 100);
    buffer_t *buf = (buffer_t *)input;
    buf->host = (uint8_t *)17;

    Func f("f"), g("g"), h("h");
    Var x("x"), y("y");

    f(x, y) = x+y;
    f.compute_root().trace_realizations();

    g(x, y) = f(x, y) + 37;
    g.compute_root().trace_realizations();

    h(x, y) = g(x, y) + input(x, y);
    h.trace_realizations();

    h.set_custom_trace(&my_trace);
    h.realize(100, 100);

    printf("The code should not have reached this print statement.\n");
    return -1;
}

#else

int main(int argc, char **argv) {
    printf("Test skipped because we're not on linux\n");
    return 0;
}

#endif
