#include <iostream>
#include <string.h>
#include <vector>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "master.h"
#include <errno.h>

// Constants
const int MaxNumberOfChildren = 20;
const int MaxNumberOfSeconds = 100;

// Forward declarations
static void show_usage(std::string);

// Main - expecting arguments
int main(int argc, char* argv[])
{
    // Argument processing
    int opt;
    int nNumberOfSeconds = 100; // Default setting
    int nNumberOfChildren = 20; // Default setting
    char *cvalue = NULL;

    // Go through each parameter entered and
    // prepare for processing
    opterr = 0;
    while ((opt = getopt(argc, argv, "hs:t:")) != -1) {
        switch (opt) {
            case 'h':
                show_usage(argv[0]);
                return EXIT_SUCCESS;
            case 's':
                nNumberOfChildren = atoi(optarg);
                break;
            case 't':
                nNumberOfSeconds = atoi(optarg);
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
                    perror("Unknown option character");
                }
                return EXIT_FAILURE;
            default:    // An bad input parameter was entered
                // Show error because a bad option was found
                perror ("master: Error: Illegal option found");
                show_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Set the correct default values (min of both)
    nNumberOfChildren = min(nNumberOfChildren, MaxNumberOfChildren);
    nNumberOfSeconds = min(nNumberOfSeconds, MaxNumberOfSeconds);

    // Check that a data file has been passed in to process
    int index = optind;
    if(index < argc)
    {
        // Get the string to process
        string FileToProcess = argv[index];

        // Output what is going to happen
        cout << "Master starting: " << endl 
            << "\t" << nNumberOfChildren << " Max Processes" << endl
            << "\t" << nNumberOfSeconds  << " Max Seconds" << endl << endl;

        // Start the Master process, returning whatever master returns.
        return processMaster(nNumberOfChildren, nNumberOfSeconds, FileToProcess);
    }

    // Otherwise, an error -- must pass a filename
    perror ("Error: You must enter a data file to process");
    show_usage(argv[0]);
    return EXIT_FAILURE;
}
