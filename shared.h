#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

// Arguement processing
extern int opterr;

// Critical Section Processing
enum state { idle, want_in, in_cs };

// Shared Memory structure
struct AddItem {
    bool readyToProcess;    // Ready to Process
    pid_t pidAssigned;      // In-Process
    bool complete;          // Completed Process
    int nodeDepth;          // Depth that this node is processing
    int itemValue;          // The actual value
    state itemState;        // The Critical Secion Flag
};

key_t key = 0;  // Shared key
int shm_id; // Shared Mem ident
char* shm_addr;

const char* HostProcess = "./master";
const char* ChildProcess = "./bin_adder";


/***************************************************
 * Helper Functions
 * *************************************************/
// For time formatting used throughout both programs
string GetTimeFormatted(const char* prePendString)
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[10];
    
    // Get time
    time (&rawtime);
    timeinfo = localtime (&rawtime);

    // Format time for HH:MM:SS
    strftime (buffer,80,"%T",timeinfo);

    string strReturn = prePendString;
    strReturn.append(buffer);
    return strReturn;
}

// Returns a string from an int
string GetStringFromInt(const int nVal)
{
    int length = snprintf( NULL, 0, "%d", nVal);
    char* sDep = (char*)malloc( length + 1 );
    snprintf( sDep, length + 1, "%d", nVal);
    string strFinalVal = sDep;                    
    free(sDep);
    return strFinalVal;
}

void show_usage(std::string name)
{
    std::cerr << std::endl
              << name << " - master app by Brett Huffman for CMP SCI 4760" << std::endl
              << std::endl
              << "Usage:\t" << name << " [-h]" << std::endl
              << "\t" << name << " [-h] [-s i] [-t time] datafile" << std::endl
              << "Options:" << std::endl
              << "  -h        This help information is shown" << std::endl
              << "  -s x      Indicate the number of children allowed to exist in the system at the same time. (Default 20)" << std::endl
              << "  -t time   The time in seconds after which the process will terminate, even if it has not finished. (Default 100)"
              << "  datafile  Input file containing one integer on each line."
              << std::endl << std::endl;
}
