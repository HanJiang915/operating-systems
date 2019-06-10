/*********************************************************************************
 * Filename: keygen.c
 * Author:   Ivan Timothy Halim
 * Date:     6/10/2019
 *
 * This program creates a key file of specified length. The characters in the file
 * generated will be any of the 27 allowed characters, generated using the standard
 * UNIX randomization methods. It then outputs the key to stdout or to the output
 * file if specified.
 * 
 * USAGE: keygen [length] [> output file]
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define SIZE 128000

// Generating the random number for key
int randomNum(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Error function used for reporting issues
void error(const char *msg, int exitStatus) {
    fprintf(stderr, "%s\n", msg);
    exit(exitStatus);
}

int main(int argc, char *argv[]) {

    char key[SIZE];
    int fileLength, i;
    time_t clock;

    // Check number of arguments 
    if (argc < 2) {
        error("keygen: ERROR missing argument", 1);
    }

    // Obtain file length from argument
    sscanf(argv[1], "%d", &fileLength);
    if (fileLength < 1) {
        error("keygen: ERROR missing argument", 1);
    }

    // Seed random number generator
    srand((unsigned)time(&clock));

    // Generate key
    memset(key, '\0', SIZE);
    for (i = 0; i < fileLength; i++) {
        key[i] = (char)randomNum(64, 90);

        // We use '@' to represent [space]
        // Replace '@' with blank space, ' '
        if (key[i] == '@') {
            key[i] = ' ';
        }
    }

    // Add newline to key
    key[fileLength] = '\n';
    fileLength++;

    // Output key
    write(STDOUT_FILENO, key, fileLength);

    return 0;
}