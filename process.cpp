#include <iostream>
#include <string.h>
#include <vector>
#include <fstream>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "master.h"
#include "shared.h"

// Static process counter => Never > 20 (1 Parent + 19 Children)
const int MAX_PROCESSES = 19;
static int ProcessCount = 0;

using namespace std;

// Item Variables
vector<int> vecItemArray;
int* addItem_num;
struct AddItem* addItems;

// SIGINT handling
volatile sig_atomic_t sigIntFlag = 0;
void sigintHandler(int sig){ // can be called asynchronously
  sigIntFlag = 1; // set flag
}

// ProcessMaster
// The processMaster function to process data from the given input file.
int processMaster(int numberOfChildrenAllowed, int timeInSecondsToTerminate, string InputDataFile)
{
    // Register SIGINT handling
    signal(SIGINT, sigintHandler);
    bool isKilled = false;

    // Start Time for time Analysis
    time_t secondsStart;

    // Read in data file
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(InputDataFile.c_str(), "r");
    if (fp == NULL)
    {
        // Error - cant open file
        errno = ENOENT;
        perror(InputDataFile.c_str());
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        int i = atoi(line);
        vecItemArray.push_back(i);  // Place each line in Vec
    }
    fclose(fp);
    free(line);

    // Get the time in seconds for our process to make
    // sure we don't exceed the max amount of processing time
    secondsStart = time(NULL);   // Start time

    // Check that the input file is a power of 2.  Kind of a
    // wizzy way of doing it... but I like it.  When it's done
    // it should tell me what power of 2 it is
    int index = vecItemArray.size();
    int targetlevel = 0;
    // Could do this with log(), but then I've got
    // to use floats. :(
    while (index >>= 1) ++targetlevel;

    // While there is less than a full power of 2
    // Fill in zeros
    if(pow(2, targetlevel) < vecItemArray.size())
    {
        targetlevel++;
        do
        {
            // Pad Array with zeros
            vecItemArray.push_back(0);
        }
        while(pow(2, targetlevel) > vecItemArray.size());
    }

    // If this was successful, we are finally ready to process
    // the file adds with our child processes.
    int arrItemCount = vecItemArray.size();

    // Allocate the shared memory
    // And get ready for read/write
    if ((key = ftok(HostProcess, 100)) == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    // allocate a shared memory segment with size of struct array
    int memSize = sizeof(AddItem) * arrItemCount;
    shm_id = shmget(key, memSize, IPC_CREAT | IPC_EXCL | 0660);
    if (shm_id == -1) {
        perror("shmget: ");
        exit(EXIT_FAILURE);
    }
    // attach the shared memory segment to our process's address space
    shm_addr = (char*)shmat(shm_id, NULL, 0);
    if (!shm_addr) { /* operation failed. */
        perror("shmat: ");
        exit(EXIT_FAILURE);
    }
    addItem_num = (int*) shm_addr;
    *addItem_num = 0;
    addItems = (struct AddItem*) (shm_addr+sizeof(int));

    // Fill struct
    for(int i=0; i < arrItemCount; i++)
    {
        addItems[i].itemValue = vecItemArray[i];
        addItems[i].pidAssigned = 0;
        addItems[i].complete = false;
        addItems[i].readyToProcess = true; //(i%2==0);
        addItems[i].nodeDepth = -1;
        addItems[i].itemState = idle;
    }

    // Start Processing with bin_adder xx yy
    // Where xx is start of processing index and yy is depth.
    // And the result should just go in the yy spot
    bool bComplete = false;
    pid_t waitPID;
    int wstatus;

    // *********** Node Print *****************
        for(int j=0; j < arrItemCount; j++)
            cout << "\t" << addItems[j].itemValue;
        cout << endl;
    // ****************************************



    // Set a variable to keep track of target level
    int nDepth = targetlevel;
    // Now, start looping until the entire calculation is complete
    // This is in a giant loop that both forks items that need
    // to be calculated and responds to PID's that are returning/erroring.
    // Once everything is done, cleanup and exit
    while(!bComplete)
    {
        // Search for an "Ready" node first by depth
        // then by every node in the array
        // Only do this if we are no Stop Flag, no time to terminate,
        // not at Max Processing count, and not at max children allowed
        if(!sigIntFlag && !((time(NULL)-secondsStart) > timeInSecondsToTerminate)
            && ProcessCount < numberOfChildrenAllowed && ProcessCount < MAX_PROCESSES)
        {
            for(int i=0;i<nDepth;i++)
            {
                for(int j=0;j<arrItemCount && ProcessCount < numberOfChildrenAllowed && ProcessCount < MAX_PROCESSES
                    ;j+=pow(2, i+1))
                {
                    // j will always be the nodes we need to
                    // check.  If it is "Ready", check it's 
                    // partner.  If it's "Ready" too => send to bin_adder
                    int nCheck1 = j;
                    int nCheck2 = pow(2, i) + j;

                    // If the current nodes looked at are ready to process and
                    // haven't already been processed for this depth, process them
                    if(addItems[nCheck1].nodeDepth < i
                        && addItems[nCheck1].readyToProcess && addItems[nCheck2].readyToProcess
                        && addItems[nCheck1].nodeDepth == addItems[nCheck2].nodeDepth)
                    {
                        // Set as processing
                        addItems[nCheck1].readyToProcess = addItems[nCheck2].readyToProcess = false;
                        // Set the depth of it's last process run
                        addItems[nCheck1].nodeDepth = addItems[nCheck2].nodeDepth = i;
                        
                        // Increment our Process Count
                        ProcessCount++;

                        // Fork and store pid in each node
                        int pid = forkProcess(nCheck1, i);
                        addItems[nCheck1].pidAssigned = addItems[nCheck2].pidAssigned = pid;
                    }
                }
            }
        }

        // Loop through looking for a returning PID
        do {
            // Note :: We use the WNOHANG to call waitpid without blocking
            // If it returns 0, it does not have a PID waiting
            waitPID = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);

            // Terminate the process if CTRL-C is typed
            // or if the max time-to-process has been exceeded
            // but only send out messages to kill once
            if((sigIntFlag || (time(NULL)-secondsStart) > timeInSecondsToTerminate) && !isKilled)
            {
                isKilled = true;
                // Send signal for every child process to terminate
                for(int i=0;i<arrItemCount;i++)
                {
                    // Send signal to close if they are in-process
                    if(addItems[i].readyToProcess == false)
                        kill(addItems[i].pidAssigned, SIGQUIT); 
                }

                // We have notified children to terminate immediately
                // then let program shutdown naturally -- that way
                // memory is deallocated correctly
                cout << endl;
                if(sigIntFlag)
                {
                    errno = EINTR;
                    perror("Killing processes due to ctrl-c signal");
                }
                else
                {
                    errno = ETIMEDOUT;
                    perror("Killing processes due to timeout");
                }
            }


            // No PIDs are in-process
            if (waitPID == -1) {
                // Show finish message only if it's not killed
                if(!isKilled)
                {
                    // Otherwise, the addition is complete.  Show final value
                    // and gracefully shutdown
                    int length = snprintf( NULL, 0, "%d", addItems[0].itemValue);
                    char* sDep = (char*)malloc( length + 1 );
                    snprintf( sDep, length + 1, "%d", addItems[0].itemValue);
                    string strFinalVal = "*** Addition Process Finished: ";
                    strFinalVal.append(sDep);                    
                    free(sDep);

                    // Show success error code
                    errno = 0;

                    // Show the final value
                    cout << endl;
                    perror(strFinalVal.c_str());
                }
                bComplete = true;   // We say true so that we exit out of main
                break;              // loop and free up all necessary data
//                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(wstatus) && waitPID > 0)
            {
                // Decrement our ProcessCounter
                ProcessCount--;

                // Success! Child processed correctly = Show it
                for(int j=0; j < arrItemCount; j++)
                    cout << "\t" << addItems[j].itemValue;
                cout << endl;
                // ****************************************

                // Print out the instance terminated
                // Make string version of PID
                int length = snprintf( NULL, 0, "%d", waitPID);
                char* sDep = (char*)malloc( length + 1 );
                snprintf( sDep, length + 1, "%d", waitPID );
                string strPID = sDep;
                free(sDep);
                // Now add with time component and perror it
                strPID.append(" Exited: ");
                string strFormattedResult = GetTimeFormatted(strPID.c_str());
                perror(strFormattedResult.c_str());

                // When the PID that terminates is of 0 depth and it's 0 index
                // Terminate the entire process => Entire tree has processed
                if(addItems[0].pidAssigned == waitPID && addItems[0].nodeDepth==nDepth)
                {
                    // Flag to break down the entire memory structure
                    bComplete = true;
                    break;
                }
                else
                {
                    // Find the item in the array based on PID
                    for(int i=0;i<arrItemCount;i++)
                    {
                        if(addItems[i].pidAssigned == waitPID)
                        {
                            // Set this node as ready to process and continue
                            addItems[i].readyToProcess = true;
                            break;
                        }
                    }
                }
            } else if (WIFSIGNALED(wstatus) && waitPID > 0) {
                cout << waitPID << " killed by signal " << WTERMSIG(wstatus) << endl;
            } else if (WIFSTOPPED(wstatus) && waitPID > 0) {
                cout << waitPID << " stopped by signal " << WTERMSIG(wstatus) << endl;
            } else if (WIFCONTINUED(wstatus) && waitPID > 0) {
                continue;
            }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));        
    }

    // Dedetach shared memory segment from process's address space
    cout << endl;
    perror("De-allocating shared memory");
    if (shmdt(shm_addr) == -1) {
        perror("main: shmdt: ");
    }

    // De-allocate the shared memory segment.
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("main: shmctl: ");
    }

    perror("Shared memory De-allocated");
    cout << endl;

    // Success!
    return EXIT_SUCCESS;
}


int forkProcess(int nItemStart, int nDepth)
{
        int pid = fork();
        // No child made - exit with failure
        if(pid < 0)
        {
            // Signal to any child process to exit

            perror("Could not fork process");
            return EXIT_FAILURE;
        }
        // Child process here - Assign out it's work
        if(pid == 0)
        {
            // Make string version of nItemStart
            int length = snprintf( NULL, 0, "%d", nItemStart);
            char* sStart = (char*)malloc( length + 1 );
            snprintf( sStart, length + 1, "%d", nItemStart );
            string strItemStart = sStart;
            free(sStart);
            
            // Make string version of nDepth
            length = snprintf( NULL, 0, "%d", nDepth);
            char* sDep = (char*)malloc( length + 1 );
            snprintf( sDep, length + 1, "%d", nDepth );
            string strDepth = sDep;
            free(sDep);

            // Execute child process
            execl(ChildProcess, strItemStart.c_str(), strDepth.c_str(), NULL);

            fflush(stdout); // Mostly for debugging -> tty wasn't flushing
            exit(EXIT_SUCCESS);    // Exit from forked process successfully
        }
        else
            return pid; // Returns the Parent PID
}
