#include "cutest.h"

//! [ENTRYPOINT]
int main(int argc, char* argv[])
{
	return cutest_run_tests(argc, argv, stdout, NULL);
}
//! [ENTRYPOINT]
