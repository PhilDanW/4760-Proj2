/* Philip Wright
 * master process program that is the workhorse of the application
 */
#include <iostream>
#include <string.h>
#include <vector>
#include <fstream>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "master.h"
#include "shared.h"

const int MAX_PROCESSES = 19;
static int myCount = 0;

using namespace std;

//variables to keep track of struct items and info
vector<int> myArray;
int* num;
struct SharedItem* node;

// SIGINT handling
volatile sig_atomic_t sigIntFlag = 0;
void sigintHandler(int sig){ 
  sigIntFlag = 1; //flag
}

// processes data from the input file.
int processMaster(int numChildren, int seconds, string dataFile)
{
    // Register SIGINT handling
    signal(SIGINT, sigintHandler);
    bool dead = false;

    // Start Time for time Analysis
    time_t start;

    // Read in data file
    FILE *fileName;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fileName = fopen(dataFile.c_str(), "r");
    if (fileName == NULL)
    {
        // Error - cant open file
        errno = ENOENT;
        perror(dataFile.c_str());
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fileName)) != -1) {
        int i = atoi(line);
        myArray.push_back(i);  // Place each line in Vector
    }
    fclose(fileName);
    free(line);

    // time in seconds for our process so we dont't exceed the max processing time
    start = time(NULL);

    // Determine power of 2
    int index = myArray.size();
    int level = 0;
    while (index >>= 1) ++level;

    // Fill in zeros if not a power of 2
    if(pow(2, level) < myArray.size())
    {
        level++;
        do
        {
            // Pad Array with zeros
            myArray.push_back(0);
        }
        while(pow(2, level) > myArray.size());
    }
    // process the file with our child processes.
    int itemCount = myArray.size();
    
    // Allocate the shared memory
    if ((key = ftok(Host, 100)) == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    else {
        int memSize = sizeof(SharedItem) * itemCount;
        shm_id = shmget(key, memSize, IPC_CREAT | IPC_EXCL | 0660);
        if (shm_id == -1) {
            perror("shmget: ");
            exit(EXIT_FAILURE);
        }
    }
  
    // attach the shared memory segment to our process's address space
    shm_addr = (char*)shmat(shm_id, NULL, 0);
    if (!shm_addr) { /* operation failed. */
        perror("shmat: ");
        exit(EXIT_FAILURE);
    }
    num = (int*) shm_addr;
    *num = 0;
    node = (struct SharedItem*) (shm_addr+sizeof(int));

    // Fill the struct with values
    for(int i=0; i < itemCount; i++)
    {
        node[i].ready = true; //(i%2==0);
        node[i].pid = 0;
        node[i].finished = false;
        node[i].nodeDepth = -1;
        node[i].value = array[i];
        node[i].nodeState = idle;
    }

    // Start Processing with bin_adder
    bool done = false;
    pid_t waitPID;
    int wstatus;

    for(int j=0; j < itemCount; j++)
        cout << "\t" << node[j].value;
        cout << endl;
    
    // Set a variable to keep track of target level
    int nDepth = level;
  
    // start looping until the entire calculation is complete
    while(!done)
    {
        // look for Ready node first by depth then by every node in the array
        if(!sigIntFlag && !((time(NULL)-start) > seconds)
            && myCount < numChildren && myCount < MAX_PROCESSES)
        {
            for(int i=0;i<nDepth;i++)
            {
                for(int j=0; j < itemCount && myCount < numChildren && myCount < MAX_PROCESSES; j += pow(2, i+1))
                {
                    // j isthe nodes we need to check.  
                    //If process is Ready, check it's partner.  
                    //If partner is Ready too => send to bin_adder
                    int nCheck1 = j;
                    int nCheck2 = pow(2, i) + j;

                    // If the current nodes looked at are ready to process and
                    // haven't already been processed for this depth, process them
                    if(node[nCheck1].nodeDepth < i
                        && node[nCheck1].ready && node[nCheck2].ready
                        && node[nCheck1].nodeDepth == node[nCheck2].nodeDepth)
                    {
                        // Set as processing
                        node[nCheck1].ready = node[nCheck2].ready = false;
                        // Set the depth of it's last process run
                        node[nCheck1].nodeDepth = node[nCheck2].nodeDepth = i;
                        
                        // Increment our Process Count
                        myCount++;

                        // Fork and store pid in each node
                        int pid = forkProcess(nCheck1, i);
                        node[nCheck1].pid = node[nCheck2].pid = pid;
                    }
                }
            }
        }

        // Loop through looking for a returning PID
        do {
            // If it returns 0, it does not have a PID waiting
            waitPID = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);

            // Terminate the process if CTRL-C is typed
            if((sigIntFlag || (time(NULL)-start) > seconds) && !dead)
            {
                dead = true;
                // Send a signal for every child process to terminate
                for(int i=0;i<itemCount;i++)
                {
                    // Send a signal to close if they are in-process
                    if(node[i].ready == false)
                        kill(node[i].pid, SIGQUIT); 
                }

                // We have notified children to terminate immediately
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
                if(!dead)
                {
                    // addition is complete.  Show final value
                    int length = snprintf( NULL, 0, "%d", node[0].value);
                    char* sDep = (char*)malloc( length + 1 );
                    snprintf( sDep, length + 1, "%d", node[0].value);
                    string strFinalVal = "*** Addition Process Finished: ";
                    strFinalVal.append(sDep);                    
                    free(sDep);

                    // Show success error code
                    errno = 0;

                    // Show the final value
                    cout << endl;
                    perror(strFinalVal.c_str());
                }
                done = true;   // We say true so that we exit out of main
                break;             
            }

            if (WIFEXITED(wstatus) && waitPID > 0)
            {
                // Decrement our process counter
                myCount--;

                // Success! Child processed correctly = Show it
                for(int j=0; j < itemCount; j++)
                    cout << "\t" << node[j].value;
                cout << endl;

                // Print out the instance terminated
                int length = snprintf( NULL, 0, "%d", waitPID);
                char* sDep = (char*)malloc( length + 1 );
                snprintf( sDep, length + 1, "%d", waitPID );
                string strPID = sDep;
                free(sDep);
                // Now add with time component and perror it
                strPID.append(" Exited: ");
                string strFormattedResult = GetTimeFormatted(strPID.c_str());
                perror(strFormattedResult.c_str());

                // Terminate the entire process => Entire tree has processed
                if(node[0].pid == waitPID && node[0].nodeDepth==nDepth)
                {
                    // Flag to break down structure
                    done = true;
                    break;
                }
                else
                {
                    // Find the item in the array based on PID
                    for(int i=0;i<itemCount;i++)
                    {
                        if(node[i].pid == waitPID)
                        {
                            // Set this node as ready to process and continue
                            node[i].ready = true;
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

    // Detatch shared memory from process's address space
    cout << endl;
    perror("Detatching shared memory");
    if (shmdt(shm_addr) == -1) {
        perror("main: shmdt: ");
    }

    // Detatch the shared memory segment.
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("main: shmctl: ");
    }

    perror("Shared memory Detatched");
    cout << endl;

    // Success!
    return EXIT_SUCCESS;
}


int forkProcess(int start, int depth)
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
            int length = snprintf( NULL, 0, "%d", start);
            char* sStart = (char*)malloc( length + 1 );
            snprintf( sStart, length + 1, "%d", start );
            string strItemStart = sStart;
            free(sStart);
            
            // Make string version of nDepth
            length = snprintf( NULL, 0, "%d", depth);
            char* sDep = (char*)malloc( length + 1 );
            snprintf( sDep, length + 1, "%d", depth );
            string strDepth = sDep;
            free(sDep);

            // Execute child process
            execl(Child, strItemStart.c_str(), strDepth.c_str(), NULL);

            fflush(stdout);
            exit(EXIT_SUCCESS);    // Exit from forked process successfully
        }
        else
            return pid; // Returns the Parent PID
}
