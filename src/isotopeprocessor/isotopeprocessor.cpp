#include <iostream>
#include <omp.h>

#include "isotopicenvelopeutils.h"
#include "../maven/projectDB.h"

using namespace std;

// static variables - CL inputs
static ProjectDB* project = nullptr;
static vector<mzSample*> samples{};
static string sampleDir = "";
static string outputDir = "";

//function declarations
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
        } else if (strcmp(argv[i], "-m") || strcmp(argv[i], "--mzrollDB") == 0) {
            project = new ProjectDB(QString(argv[i+1]));
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sampleDir") == 0) {
            sampleDir = argv[i+1];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outputDir") == 0) {
            outputDir = argv[i+1];
        }
    }
}
