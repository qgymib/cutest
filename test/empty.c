#undef NDEBUG
#include <assert.h>

#include "cutest.h"

int main(int argc, char* argv[])
{
    assert(cutest_run_tests(argc, argv, NULL) == 0);
    return 0;
}
