/*********************************************************************************
 * Filename: smallsh.c
 * Author:   Ivan Timothy Halim
 * Date:     3/5/2019
 *
 * A basic shell that supports three built-in commands: exit, cd, and status.
 * Handles all other commands by spawning child processes to perform execvp().
 *
 * USAGE: command [arg1 arg2 ...] [< input_file] [> output_file] [&]
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

typedef enum {false, true} bool;

// This is an array to store unfinished background pid
int backgroundPid[100];
int nPid = 0;

// These variables store the exit status of the last terminated process
int exitStatus = 0;
int termSignal = -1;

// This flag specifies if the shell is in foreground-only mode
int fgonly = 0;

/*
 * This is a command struct, which stores the information of the
 * user input command
 */
struct Cmd {
    char* argv[512];
    int nArgs;
    char inputFile[64];
    char outputFile[64];
    bool redirInput;
    bool redirOutput;
    bool background;
};

// A function to create and initialize the command struct
struct Cmd* cmdCreate() {
    struct Cmd* cmd = malloc(sizeof(struct Cmd));
    int index;
    for (index = 0; index < 512; index++) {
        cmd->argv[index] = NULL;
    }
    cmd->nArgs = 0;
    memset(cmd->inputFile, '\0', sizeof(cmd->inputFile));
    memset(cmd->outputFile, '\0', sizeof(cmd->outputFile));
    cmd->redirInput = false;
    cmd->redirOutput = false;
    cmd->background = false;
    return cmd;
}

// A function to free the command struct
void freeCmd(struct Cmd* cmd) {
    free(cmd);
}

// This function replaces all occurences of a substring in a string
char* str_replace(char* orig, char* rep, char* with) {
    char* result; // the return string
    char* ins;    // the next insert point
    char* tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep) {
        return NULL;
    }
    len_rep = strlen(rep);
    if (len_rep == 0) {
        return NULL; // empty rep causes infinite loop during count
    }
    if (!with) {
        with = "";
    }
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result) {
        return NULL;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

// This function parses an input string into a command struct
void parseInput(struct Cmd* cmd, char* input) {

    // First we initializes the command struct to its default state
    int index;
    for (index = 0; index < cmd->nArgs; index++) {
        cmd->argv[index] = NULL;
    }
    cmd->nArgs = 0;
    memset(cmd->inputFile, '\0', sizeof(cmd->inputFile));
    memset(cmd->outputFile, '\0', sizeof(cmd->outputFile));
    cmd->redirInput = false;
    cmd->redirOutput = false;
    cmd->background = false;

    /*
     * We're going to use string token to separate the words in
     * input string with spaces ' ' as separators.
     *
     * First we're going to take the command and the arguments. The while loop
     * is going to stop whenever we encounter special characters '<','>','#' or
     * there is no more words in input string (token == NULL). We are ignoring
     * '&' because it doesn't always mean background process (ex. echo)
     */
    char* token = strtok(input, " ");
    while (token != NULL &&
           token[0] != '<' &&
           token[0] != '>' &&
           token[0] != '#') {
        cmd->argv[cmd->nArgs] = token;
        cmd->nArgs++;
        token = strtok(NULL, " ");
    }

    /*
     * Next we're going to take the input output file
     * The input output is in no particular order so we're
     * going to use a while loop
     */
    while (token) {

        // If token is '<' then there's an input redirection
        if (token[0] == '<') {
            cmd->redirInput = true;
            token = strtok(NULL, " ");

            // Check to see if an input file is provided
            if (token != NULL &&
                token[0] != '>' &&
                token[0] != '&') {
                strcpy(cmd->inputFile, token);
                token = strtok(NULL, " ");
            }

        // If token is '>' then there's an output redirection
        } else if (token[0] == '>') {
            cmd->redirOutput = true;
            token = strtok(NULL, " ");

            // Check to see if an output file is provided
            if (token != NULL &&
                token[0] != '<' &&
                token[0] != '&') {
                strcpy(cmd->outputFile, token);
                token = strtok(NULL, " ");
            }

        // If token is neither '>' nor '<', exit the while loop
        } else {
            break;
        }
    }

    /*
     * Lastly, we're going to check if the command is a background process
     * A command is a background process if the last character of the string
     * is an ampersand '&'
     */

    // If after taking both the input and the output, the token is '&', then
    // we know that the last character in the string is '&'
    if (token != NULL && token[0] == '&') {
        cmd->background = true;

    // If token is not '&', it doesn't mean that the command is not a background process
    // We need to check if we have taken '&' as an argument earlier
    } else if (cmd->nArgs) {
        if (!strcmp(cmd->argv[cmd->nArgs - 1], "&")) {

            // Given that the last argument is '&', it is a background process only if
            // there's no input output redirection
            if (cmd->redirInput == false && cmd->redirOutput == false) {

                // If cmd is a background process, erase '&' from the argument
                cmd->argv[cmd->nArgs - 1] = NULL;
                cmd->nArgs--;
                cmd->background = true;
            }
        }
    }
}

// This function executes our built-in commands
void runBuiltIn(struct Cmd* cmd, char* lineEntered, char* input) {

    // If command is "cd"
    if (!strcmp(cmd->argv[0], "cd")) {

        // If target directory is specified
        if (cmd->argv[1]) {

            // Change directory to the target directory
            if (chdir(cmd->argv[1]) < 0) { // If we cannot find the directory

                // Print an error message and set the exit status to 1
                printf("Directory not found.\n");
                fflush(stdout);
                exitStatus = 1;
                termSignal = -1;
                return;
            }

        // If target directory is not specified
        } else {

            // Go to home directory
            chdir(getenv("HOME"));
        }

    // If command is "status"
    } else if (!strcmp(cmd->argv[0], "status")) {

        // Print out the exit status of the last terminated process
        // A process can either terminate naturally or be interrupted by a signal
        if (exitStatus >= 0) {
            printf("exit value %d\n", exitStatus);
            fflush(stdout);
        } else {
            printf("terminated by signal %d\n", termSignal);
            fflush(stdout);
        }

    // If command is "exit"
    } else if (!strcmp(cmd->argv[0], "exit")) {

        // Kill off all unfinished background processes
        int i;
        for (i = 0; i < nPid; i++) {
            kill(backgroundPid[i], SIGKILL);
        }

        // Before exiting, free up all resources to prevent memory leak
        freeCmd(cmd);
        free(lineEntered);
        free(input);
        exit(0);
    }

    // If we made this far then process terminates successfully
    // Set exit status to 0 and termSignal to -1 (process is not terminated by signal)
    exitStatus = 0;
    termSignal = -1;
}

// This function prevents a process from terminating when given a SIGINT signal
void disableSIGINT() {
    struct sigaction SIGINT_action = {{0}};
    SIGINT_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &SIGINT_action, NULL);
}

// This function allows a process to terminate when given a SIGINT signal
void enableSIGINT() {
    struct sigaction SIGINT_action = {{0}};
    SIGINT_action.sa_handler = SIG_DFL;
    sigfillset(&SIGINT_action.sa_mask);
    sigaction(SIGINT, &SIGINT_action, NULL);
}

/*
 * This function processes received SIGTSTP signal
 * SIGTSTP is used to enter/exit foreground-only mode
 */
void handleSIGTSTP(int signo) {

    // If the shell is not in foreground-only mode
    // Enter foreground-only mode
    if (!fgonly) {
        fgonly = 1;
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, strlen(message));

    // If the shell is in foreground-only
    // Exit foreground-only mode
    } else {
        fgonly = 0;
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, strlen(message));
    }
}

// This function catches SIGTSTP signal and process it using handleSIGTSTP function
void enableSIGTSTP() {
    struct sigaction SIGTSTP_action = {{0}};
    SIGTSTP_action.sa_handler = handleSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

void disableSIGTSTP() {
    struct sigaction SIGTSTP_action = {{0}};
    SIGTSTP_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

// This function checks if any background process has terminated
void checkBackgroundProcess() {
    int i, j;               // Indexes
    pid_t childPid;         // Holds the child PID
    int childExitMethod;    // Holds the child exit method

    // For every background process in backgroundPid array
    for (i = nPid; i > 0; i--) {

        // Check if the process has terminated
        // The flag "WNOHANG" means it does not block the parent process (With No Hang)
        childPid = waitpid(backgroundPid[i-1], &childExitMethod, WNOHANG);

        if (childPid) { // If the child process has been terminated

            // Kill that process
            kill(childPid, SIGKILL);

            // Print out the PID of the terminated background process
            printf("background pid %d is done: ", childPid);

            // Print out the exit status of the terminated process
            // or the terminating signal if interrupted by a signal
            if (WIFEXITED(childExitMethod)) {
                printf("exit value %d\n", WEXITSTATUS(childExitMethod));
            } else {
                printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
            }
            fflush(stdout);

            // Shift everything on the right side to the left (yes, this is inefficient)
            nPid--;
            for (j = i-1; j < nPid; j++) {
                backgroundPid[j] = backgroundPid[j+1];
            }
        }
    }
}

// This function gets an input from the user
char* getInput() {
    char* lineEntered = NULL;  // Holds the user input
    size_t bufferSize = 0;     // Holds the size of the input buffer (only for requirement)
    int numCharsEntered = 0;   // Holds the number of chars entered by user

    // Get input string from user and remove its trailing newline '\n'
    while(1) {

        // Before each prompt, check if any background process has terminated
        checkBackgroundProcess();

        // Get the input from the user
        printf(": ");
        fflush(stdout);
        numCharsEntered = getline(&lineEntered, &bufferSize, stdin);

        // If user input is invalid, clear error in stdin and repeat the process
        if (numCharsEntered == -1) {
            clearerr(stdin);

        // If the user did not enter any input, repeat the process
        // numCharsEntered is 1 because of newline '\n'
        } else if (numCharsEntered == 1) {
            continue;

        // If user input is valid, exit the while loop
        } else {
            break;
        }
    }

    // Remove the trailing newline '\n' from our input string
    lineEntered[numCharsEntered - 1] = '\0';

    return lineEntered;
}

// This function performs input redirection
void redirectInput(struct Cmd* cmd, char* lineEntered, char* input) {

    // If command is background process or
    // if command is foreground process and the user specifies input redirection
    if ((cmd->background == true && !fgonly) ||
        ((cmd->background == false || fgonly) && cmd->redirInput == true)) {

        // If input file is not specified
        if (cmd->inputFile[0] == '\0') {

            // Redirect input to /dev/null
            int devNull = open("/dev/null", O_WRONLY);
            dup2(devNull, STDIN_FILENO);
            close(devNull);

        // If input file is specified
        } else {

            // Open up the input file as read-only
            int file_descriptor = open(cmd->inputFile, O_RDONLY);

            if (file_descriptor < 0) { // If fail to open input file

                // Print out an error message
                printf("cannot open %s for input\n", cmd->inputFile);

                // Free up allocated memory before exiting
                freeCmd(cmd);
                free(lineEntered);
                free(input);
                exit(1);
            }

            // Redirect input to input file
            dup2(file_descriptor, STDIN_FILENO);
            close(file_descriptor);
        }
    }
}

// This function performs output redirection
void redirectOutput(struct Cmd* cmd, char* lineEntered, char* input) {

    // If command is background process or
    // if command is foreground process and the user specifies output redirection
    if ((cmd->background == true && !fgonly) ||
        ((cmd->background == false || fgonly) && cmd->redirOutput == true)) {

        // If output file is not specified
        if (cmd->outputFile[0] == '\0') {

            // Redirect output to /dev/null
            int devNull = open("/dev/null", O_WRONLY);
            dup2(devNull, STDOUT_FILENO);
            close(devNull);

        // If output file is specified
        } else {

            // Create the output file or overwrite it if it already exists
            int file_descriptor = open(cmd->outputFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

            if (file_descriptor < 0) { // If fail to open output file

                // Print out an error message
                printf("cannot open %s for output\n", cmd->outputFile);

                // Free up allocated memory before exiting
                freeCmd(cmd);
                free(lineEntered);
                free(input);
                exit(1);
            }

            // Redirect output to output file
            dup2(file_descriptor, STDOUT_FILENO);
            close(file_descriptor);
        }
    }
}

void main() {

    // Disable process termination by SIGINT
    disableSIGINT();

    // Enable SIGTSTP to enter/exit foreground-only mode
    enableSIGTSTP();

    struct Cmd* cmd = cmdCreate();  // Create a new command struct
    pid_t spawnPid;                 // Holds the process id created by fork()
    int childExitMethod;            // Holds the child exit method
    char* lineEntered = NULL;       // Holds the user input
    char* input = NULL;             // Holds the expanded user input

    // Get the parent process ID and convert it to a string
    int pid = getpid();
    char pidString[20];
    memset(pidString, '\0', sizeof(pidString));
    sprintf(pidString, "%d", pid);

    while(1) {

        // Get input from user and expand all instances of "$$" into process ID
        lineEntered = getInput();
        input = str_replace(lineEntered, "$$", pidString);

        // Parse line entered into command struct
        parseInput(cmd, input);

        // If there's no command to be evaluated
        if (!cmd->nArgs) {

            // Free up memory allocated to prevent memory leak
            // Continue the while loop
            free(lineEntered);
            free(input);
            lineEntered = NULL;
            input = NULL;
            continue;

        // If command is a built-in command (cd, status, exit)
        } else if (!strcmp(cmd->argv[0], "cd") || !strcmp(cmd->argv[0], "exit") || !strcmp(cmd->argv[0], "status")) {

            // Run the built-in command
            runBuiltIn(cmd, lineEntered, input);

        // If command is not a built-in command
        } else {

            // Fork that shit
            spawnPid = fork();
            switch (spawnPid) {

                // Fail to spawn a new process
                case -1:
                    perror("Hull Breach!\n");   // Print out error message
                    freeCmd(cmd);               // Free up memory allocated before exiting
                    free(lineEntered);
                    free(input);
                    exit(1);
                    break;

                // Child process
                case 0:

                    // Disable SIGTSTP to enter/exit foreground-only mode
                    disableSIGTSTP();

                    // Enable process termination by SIGINT if it is a foreground process
                    if (cmd->background == false || fgonly) {
                        enableSIGINT();
                    }

                    // Perform input output redirection
                    redirectInput(cmd, lineEntered, input);
                    redirectOutput(cmd, lineEntered, input);

                    // Execute the command
                    execvp(cmd->argv[0], cmd->argv);

                    // If execvp failed
                    perror("");         // Print out error message
                    freeCmd(cmd);       // Free up memory allocated before exiting
                    free(lineEntered);
                    free(input);
                    exit(1);
                    break;

                // Parent process
                default:

                    // If it is a background process
                    if (cmd->background == true && !fgonly) {

                        // Insert the child process id to backgroundPid array
                        backgroundPid[nPid] = spawnPid;
                        nPid++;

                        // Print out the child process id
                        printf("background pid is %d\n", spawnPid);
                        fflush(stdout);

                    // If it is a foreground process
                    } else {

                        // Wait until the child process has terminated
                        waitpid(spawnPid, &childExitMethod, 0);

                        // If the child process terminated successfully
                        if (WIFEXITED(childExitMethod)) {

                            // Store the exit status of child process
                            // Set the terminating signal to -1 (process not terminated by signal)
                            exitStatus = WEXITSTATUS(childExitMethod);
                            termSignal = -1;

                        // If the child is terminated by a signal
                        } else {

                            // Store the signal that terminates child process
                            // Set the exit status to -1 (process did not terminate successfully)
                            termSignal = WTERMSIG(childExitMethod);
                            exitStatus = -1;

                            // Print out the terminating signal
                            printf("terminated by signal %d\n", termSignal);
                            fflush(stdout);
                        }
                    }
                    break;
            }
        }

        // Free the memory allocated by getline() and str_replace() or else memory leak
        free(lineEntered);
        free(input);
        lineEntered = NULL;
        input = NULL;
    }
} // NO MEMORY LEAK B*TCHES!!!