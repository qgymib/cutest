#include "cases/__init__.h"
#include "test.h"

int main(int argc, char* argv[])
{
    cutest_test_run_tools(argc, argv);
    return test_run_all_tests();
}
