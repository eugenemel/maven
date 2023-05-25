#include <iostream>
#include <omp.h>

#include "isotopicenvelopeutils.h"

using namespace std;

void processOptions(int argc, char* argv[]);

int main(int argc, char *argv[]){

    omp_set_num_threads(4);
    #pragma omp parallel for
    for (unsigned int i = 0; i < 10; i++) {

        #pragma omp critical
        cout << "Hello from thread #" << omp_get_thread_num() << " (nthreads=" << omp_get_num_threads() << ")"<< endl;
    }
}

void processOptions(int argc, char* argv[]) {

    IsotopeProcessorOptions& options = IsotopeProcessorOptions::instance();

    for (unsigned int i = 0; i < static_cast<unsigned int>(argc-1); i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            options.setOptions(argv[i+1]);
        }
    }
}
