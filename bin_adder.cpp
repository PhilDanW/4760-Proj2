#include <iostream>
#include "bin_adder.h"
#include <unistd.h>
#include "shared.h"
#include <fstream>

// SIGQUIT handling
volatile sig_atomic_t sigQuitFlag = 0;
void sigQuitHandler(int sig){ // can be called asynchronously
  sigQuitFlag = 1; // set flag
}

// Critical Section Turn Flag
extern int turn;

const char* LogFile = "adder_log";

using namespace std;

int main(int argc, char* argv[])
{

    // Register SIGQUIT handling
    signal(SIGINT, sigQuitHandler);

    int turn = 0;   // Used for Critical Section
    
    // Variables to be used
    int nFirstNumberIndex = 0;
    int nSecondNumberIndex = 0;
    int nDepth = 0;

    // Argument processing
    // If any error happens, show error statement and
    // return error immediately
    try
    {
        if(argc!=2) throw std::exception();// ("Incorrect Arguements");
        int index = optind;
        nFirstNumberIndex = atoi(argv[0]);
        nDepth = atoi(argv[1]);
    }
    catch(const std::exception& e)
    {
        errno = EINVAL;
        perror(e.what());
        // General error
        show_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Calculate the second index
    nSecondNumberIndex = pow(2, nDepth) + nFirstNumberIndex;

    pid_t childPid = getpid();

    // Show startup params to perror
    string strChildPid = GetStringFromInt(childPid);
    string strShow = strChildPid + " Started - bin_adder " + 
        GetStringFromInt(nFirstNumberIndex) + " " + GetStringFromInt(nDepth);
    perror(strShow.c_str());

    // Allocate the shared memory
    // And get ready for read/write
    if ((key = ftok(HostProcess, 100)) == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    // Get a reference to the shared memory, if available
    shm_id = shmget(key, 0, 0);
    if (shm_id == -1) {
        perror("shmget1: ");
        exit(EXIT_FAILURE);
    }

    // Read the memory size and calculate the array size
    struct shmid_ds shmid_ds;
    shmctl(shm_id, IPC_STAT, &shmid_ds);
    size_t realSize = shmid_ds.shm_segsz;
    int length = (int) shmid_ds.shm_segsz / sizeof(AddItem);

    // Now we have the size - actually setup with shmget
    shm_id = shmget(key, realSize, 0);
    if (shm_id == -1) {
        perror("shmget2: ");
        exit(EXIT_FAILURE);
    }

    // attach the shared memory segment to our process's address space
    shm_addr = (char*)shmat(shm_id, NULL, 0);
    if (!shm_addr) { /* operation failed. */
        perror("shmat: ");
        exit(EXIT_FAILURE);
    }

    // Cast it to our structure array
    int* addItem_num = (int*) shm_addr;
    *addItem_num = 0;
    struct AddItem* addItems = (struct AddItem*) (shm_addr+sizeof(int));

    // Check that everything is okay before we go any further
    if(nFirstNumberIndex > length || nSecondNumberIndex > length)
    {
        // Error found
        perror("bin_adder: ");
        exit(EXIT_FAILURE);
    }

    // Determine the two numbers to add and store it in the first position
    addItems[nFirstNumberIndex].itemValue = 
        addItems[nFirstNumberIndex].itemValue + addItems[nSecondNumberIndex].itemValue;

//    cout << "Total: " << addItems[nFirstNumberIndex].itemValue << endl;


    // Critical Section Handling Routine
    // Keep the state flag in addItems[nFirstNumberIndex].state
    
    int j; // Local to this process
    do
    {
        addItems[nFirstNumberIndex].itemState = want_in; // Raise my flag
        j = turn; // Set local variable
        while ( j != nFirstNumberIndex )
        j = ( addItems[j].itemState != idle ) ? turn : ( j + 1 ) % length;

        // Declare intention to enter critical section
        addItems[nFirstNumberIndex].itemState = in_cs;
        // Check that no one else is in critical section
        for ( j = 0; j < length; j++ )
            if ( ( j != nFirstNumberIndex ) && ( addItems[j].itemState == in_cs ) )
        break;
    } while (!sigQuitFlag && ( j < length ) || 
        ( turn != nFirstNumberIndex && addItems[turn].itemState != idle ));

    // Assign turn to self and enter critical section
    turn = nFirstNumberIndex;

    // ************ Enter Critical Secion ************
    
    // Print it to perror
    string strFormattedResult = strChildPid + " " + GetTimeFormatted("Entered Critical Section: ");
    perror(strFormattedResult.c_str());

    // Write to log file
    ofstream ofLogFile (LogFile, ios::app);
    if (ofLogFile.is_open())
    {
        ofLogFile << GetTimeFormatted("") << "\t"
                << childPid   << "\t"
                << nFirstNumberIndex << "\t"
                << nDepth << endl;
        ofLogFile.close();
    }

    // ************ Exit Critical Section ************

    // Make a 1-second wait time
    time_t secondsFinish = time(NULL) + 1;   // Finish time

    // Loop until a SIGQUIT happens or we reach Finish Time
    while(!sigQuitFlag && secondsFinish > time(NULL))
    ;

    // Print it to perror
    strFormattedResult = strChildPid + " " + GetTimeFormatted("Exited Critical Section: ");
    perror(strFormattedResult.c_str());

    // Exit section is removed because we aren't looping in our example
    // we are just exiting the application.

    addItems[nFirstNumberIndex].itemState = idle;

    return EXIT_SUCCESS;
}
