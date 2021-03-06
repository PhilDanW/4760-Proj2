/* Philip Wright
 * shared.h contains shared structures, variables and functions
 */

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

// Semiphore union
union semun {
    int                 val;
    struct semid_ds *   buf;
    unsigned short *    array;
#if defined(__linux__)
    struct seminfo *    __buf;
#endif
};

// Critical Section Processing
enum state { idle, want_in, in_cs };

// Shared Memory structure
struct SharedItem {
    bool ready;             // Ready to Process
    pid_t pid;              // In-Process
    bool finished;          // Process that is done
    int nodeDepth;          // Depth that this node is processing
    int value;              // The actual value
    state nodeState;        // The Critical Secion Flag
};

key_t key = 0;  // Shared key
int shm_id; // Shared Mem ident
char* shm_addr;

const char* Host = "./master";
const char* Child = "./bin_adder";
// takes in a string and returns an int
string GetStringFromInt(const int nVal)
{
    int length = snprintf( NULL, 0, "%d", nVal);
    char* sDep = (char*)malloc( length + 1 );
    snprintf( sDep, length + 1, "%d", nVal);
    string strFinalVal = sDep;                    
    free(sDep);
    return strFinalVal;
}

//For time formatting
string GetTimeFormatted(const char* prePendString)
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[10];
    
    //Get time
    time (&rawtime);
    timeinfo = localtime (&rawtime);

    //HH:MM:SS
    strftime (buffer,80,"%T",timeinfo);

    string strReturn = prePendString;
    strReturn.append(buffer);
    return strReturn;
}


