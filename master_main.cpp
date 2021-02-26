/* Philip Wright
* master program that accepts the command line arguments
*/
#include <iostream>
#include <string.h>
#include <vector>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "master.h"
#include <errno.h>

const int maxChildren = 20;
const int maxSeconds = 100;

static void usage(std::string);

int main(int argc, char* argv[])
{
    // Process the command line arguments
    int option;
    int numSeconds = 100; //Default 
    int numChildren = 20; //Default

    while ((option = getopt(argc, argv, "hs:t:")) != -1) {
        switch (option) {
            case 'h':
                usage(argv[0]);
                return EXIT_SUCCESS;
            case 's':
                numChildren = atoi(optarg);
                break;
            case 't':
                numSeconds = atoi(optarg);
                break;
            case '?': // Unknown arguement                
                if (isprint (optopt))
                {
                    errno = EINVAL;
                    perror("Unknown option");
                }
                else
                {
                    errno = EINVAL;
                    perror("Option character not known.");
                }
                return EXIT_FAILURE;
            default:    
                // Show error because a bad option was found
                perror ("master: Error: Invalid argument found.");
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    //Clamp the values
    numChildren = min(numChildren, maxChildren);
    numSeconds = min(numSeconds, maxSeconds);

    // Check that a data file has been passed in to process
    int index = optind;
    if(index < argc)
    {
        // Get the string of the file to process
        string File = argv[index];
        cout << "Master process starting: " << endl 
            << "\t" << numChildren << " Processes" << endl
            << "\t" << numSeconds  << " Seconds" << endl << endl;

        //invoke the master process to read the file and spawn child processes
        return processMaster(numChildren, numSeconds, File);
    }
    
    perror ("Error: Must provide data file in order to process");
    usage(argv[0]);
    return EXIT_FAILURE;
}


// Handle errors in input arguments by showing usage screen
static void usage(std::string name)
{
    std::cerr << std::endl
              << name << " - master" << std::endl
              << std::endl
              << "Usage:\t" << name << " [-h]" << std::endl
              << "\t" << name << " [-h] [-s i] [-t time] inputfile" << std::endl
              << "Options:" << std::endl
              << "  -h        This help menu is displayed" << std::endl
              << "  -s x      The number of child processes allowed in the system at the same time. (Default 20)" << std::endl
              << "  -t time   The time in seconds it takes to terminate a process, even if its running. (Default 100)"
              << "  inputfile  Input file containing one integers."
              << std::endl << std::endl;
}
