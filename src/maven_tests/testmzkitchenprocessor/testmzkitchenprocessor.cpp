#ifndef _OFF_T
#define _OFF_T
#include <sys/_types.h> /* __darwin_off_t */
typedef __darwin_off_t          off_t;
#endif  /* _OFF_T */
//#define _LARGEFILE_SOURCE
typedef off_t f_off;

#include "mzSample.h"

int main(int argc, char* argv[]) {
    cout << "Test Passed" << endl;
}
