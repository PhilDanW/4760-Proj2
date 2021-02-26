/* Philip Wright
 * CMP SCI 4760
 * bin_adder cpp file that is called in the main part of the program
 */
#include <iostream>
#include "bin_adder.h"
#include <unistd.h>
#include "shared.h"
#include <fstream>

// SIGQUIT handling
volatile sig_atomic_t sigQuitFlag = 0;
void sigQuitHandler(int sig) { 
  sigQuitFlag = 1; //flag
}

extern int turn; // Critical Section Flag
const char* outputFile = "adder_log";

using namespace std;
int main(int argc, char* argv[])
{
    signal(SIGINT, sigQuitHandler);  //SIGQUIT handling

    int turn = 0;   // turn for Critical Section
    int firstNumber = 0;
    int secondNumber = 0;
    int depth = 0;
     
    // procress arguments and return any errors immediately
    try
    {
        if(argc!=2) throw std::exception();// ("Incorrect Arguements");
        firstNumber = atoi(argv[0]);
        depth = atoi(argv[1]);
    }
    catch(const std::exception& e)
    {
        errno = EINVAL;
        perror(e.what());
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Calculate the second index
    secondNumber = pow(2, depth) + firstNumber;

    pid_t childPid = getpid();

    string strChildPid = GetStringFromInt(childPid);
    string strShow = strChildPid + " Started - bin_adder " + GetStringFromInt(firstNumber) + " " + GetStringFromInt(depth);
    perror(strShow.c_str());
  
    
    // Allocate the shared memory and error if it fails
    if ((key = ftok(Host, 100)) == -1) {
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
    int length = (int) shmid_ds.shm_segsz / sizeof(SharedItem);

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
    struct SharedItem* addItems = (struct SharedItem*) (shm_addr+sizeof(int));
    // Determine the two numbers to add and store it in the first position
    addItems[firstNumber].value = addItems[firstNumber].value + addItems[secondNumber].value;
  
    // Critical Section Handling 
    int j; 
    do
    {
        addItems[firstNumber].nodeState = want_in; // Raise my flag
        j = turn; // Set local variable
        while ( j != firstNumber )
        j = ( addItems[j].inodeState != idle ) ? turn : ( j + 1 ) % length;

        // Declare intention to enter critical section
        addItems[firstNumber].nodeState = in_cs;
        // Check that no one else is in critical section
        for ( j = 0; j < length; j++ )
            if ( ( j != firstNumber ) && ( addItems[j].nodeState == in_cs ) )
        break;
    } while (!sigQuitFlag && ( j < length ) || 
        ( turn != firstNumber && addItems[turn].nodeState != idle ));

    // Get your turn and enter critical section
    turn = firstNumber;

    // Print it to perror
    string strFormattedResult = strChildPid + " " + GetTimeFormatted("Entered Critical Section: ");
    perror(strFormattedResult.c_str());

     // Write to log file
     ofstream outputFile (outputFile, ios::app);
     if (outputFile.is_open())
     {
         outputFile << GetTimeFormatted("") << "\t"
                    << childPid   << "\t"
                    << firstNumber << "\t"
                    << depth << endl;
         outputFile.close();
      }
  
    //Exit Critical Section
    // wait 1 second first
    time_t secondsFinish = time(NULL) + 1;   // time it exited
  
    while(!sigQuitFlag && secondsFinish > time(NULL));
    strFormattedResult = strChildPid + " " + GetTimeFormatted("Exited Critical Section: ");
    perror(strFormattedResult.c_str());
    addItems[firstNumber].nodeState = idle;

    return EXIT_SUCCESS;
}

// Handle errors in input arguments by showing usage screen
static void usage(std::string name)
{
    std::cerr << std::endl
              << name << " - bin_adder" << std::endl
              << std::endl
              << "Usage:\t" << name << " xx yy" << std::endl
              << "Parameters:" << std::endl
              << "  xx   The index of the first number to add in shared memory" << std::endl
              << "  yy   The depth of the tree in shared memory" << std::endl
              << "This program will only work when called by the master program." << std::endl
              << std::endl << std::endl;
}
