/*********************************************************************************
 * Author: Ivan Timothy Halim
 * Description: This is a text-driven adventure style game where the goal is to get to the end.
 * This particular file reads the created game files and allows playthrough of the game. It also
 * has a time function
 * Date: 2/15/2019
 *********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

typedef enum {true, false} bool;

/*
 * These variables stores the path of the newest room directory created
 * and the path of the room file the player is in.
 */
char newestDirName[256];
char roomPosition[256];

// Create a pthread mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * This is a player struct which stores the information
 * of the room the player is in.
 */
struct Player {
    char roomName[32];
    char roomType[32];
    char connections[6][32];
    int nConnections;
    char stepsTaken[1000][32];
    int nSteps;
};

// A function to create the player struct
struct Player* playerCreate() {
    struct Player* player = malloc(sizeof(struct Player));
    assert(player);
    player->nConnections = 0;
    player->nSteps = 0;
    return player;
}

/*
 * This function searches for the path to the newest room directory created
 * and store that path in the newestDirName.
 */
void findNewestDir() {
    int newestDirTime = -1; // Modified timestamp of newest subdir examined
    char targetDirPrefix[32] = "halimi.rooms."; // Prefix we're looking for

    // Reset the newestDirName
    memset(newestDirName, '\0', sizeof(newestDirName));

    DIR* dirToCheck;            // Holds the directory we're starting in
    struct dirent* fileInDir;   // Holds the current subdir of the starting dir
    struct stat dirAttributes;  // Holds information we've gained about subdir

    // Open up the directory this program was run in
    dirToCheck = opendir(".");

    if (dirToCheck > 0) // Make sure the current directory could be opened
    {
        while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
        {
            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
            {
                stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry
                if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
                {
                    newestDirTime = (int)dirAttributes.st_mtime; // Update newestDirTime

                    // Copy the directory name into newestDirName
                    memset(newestDirName, '\0', sizeof(newestDirName));
                    strcpy(newestDirName, fileInDir->d_name);
                }
            }
        }
    }
    closedir(dirToCheck); // Close the directory we opened
}

/*
 * This function searches for the start foom file path in the newest directory
 * and store this path inside roomPosition
 */
void searchStartRoom() {
    int file_descriptor;  // Holds a reference to the open file
    ssize_t nread;        // Stores the number of bytes read
    char readBuffer[256]; // Stores the contents of read
    char filePath[256];   // Holds the file path of the room file examined

    DIR* dirToCheck;          // Holds the room directory
    struct dirent* fileInDir; // Holds the room file inside the room directory

    // Open up the room directory
    dirToCheck = opendir(newestDirName);

    if (dirToCheck > 0) // Make sure the current directory could be opened
    {
        while((fileInDir = readdir(dirToCheck)) != NULL) // Check file in dir
        {

            /*
             * First we write the file path of the room file
             * by combining the directory name with the
             * name of the file examined.
             */
            memset(filePath, '\0', sizeof(filePath));
            sprintf(filePath, "%s%c%s", newestDirName, '/', fileInDir->d_name);

            // Read the file in the file path
            file_descriptor = open(filePath, O_RDONLY);

            // Read into readBuffer the content of the room file
            memset(readBuffer, '\0', sizeof(readBuffer));
            nread = read(file_descriptor, readBuffer, sizeof(readBuffer));

            /*
             * If the file is a START_FILE, then the 9th character
             * before the end of the file should be 'A'. This includes
             * the newline '\n' character.
             */
            if (readBuffer[nread-9] == 'A') {
                memset(roomPosition, '\0', sizeof(roomPosition));
                strcpy(roomPosition, filePath);
            }
        }
    }
    closedir(dirToCheck); // Close the directory we opened
}

/*
 * This function reads the file specified in the roomPosition filepath
 * and extracts the information of the room into the player struct
 */
void updatePlayer(struct Player* player) {

    assert(player); // Check if player is not NULL

    int file_descriptor;  // Holds a reference to the open file
    ssize_t nread;        // Stores the number of bytes read
    char readBuffer[256]; // Stores the contents of read

    // Open the file in roomPosition as read only
    file_descriptor = open(roomPosition, O_RDONLY);

    // Read the contents of the file and put it into readBuffer
    memset(readBuffer, '\0', sizeof(readBuffer));
    nread = read(file_descriptor, readBuffer, sizeof(readBuffer));

    /*
     * We're going to use string token to separate the words in
     * room file with spaces ' ' and newline '\n' as separators.
     *
     * The string token is going to traverse the room file word-by-word
     * everytime we call strtok.
     */
    char* token = strtok(readBuffer, " \n");
    token = strtok(NULL, " \n");
    token = strtok(NULL, " \n");
    memset(player->roomName, '\0', sizeof(player->roomName));
    strcpy(player->roomName, token);
    token = strtok(NULL, " \n");

    /*
     * This while loop extracts the room connections in room file.
     * Because we don't know the number of connections, we use a while loop.
     *
     * First we reset the number of connections to zero
     * because we want to replace the contents in connections
     */
    player->nConnections = 0;
    do {
        token = strtok(NULL, " \n");
        token = strtok(NULL, " \n");
        memset(player->connections[player->nConnections], '\0', sizeof(player->connections[player->nConnections]));
        strcpy(player->connections[player->nConnections], token);
        player->nConnections++;
        token = strtok(NULL, " \n");
    } while (token[0] == 'C');

    // Lastly, we extract the room type at the bottom of the file
    token = strtok(NULL, " \n");
    token = strtok(NULL, " \n");
    memset(player->roomType, '\0', sizeof(player->roomType));
    strcpy(player->roomType, token);
}

/*
 * This is a function to print out the information
 * stored in player struct
 */
void printPlayer(struct Player* player) {
    assert(player);
    printf("CURRENT LOCATION: %s\n", player->roomName);
    printf("POSSIBLE CONNECTIONS: ");
    int index;
    for (index = 0; index < player->nConnections; index++) {
        printf("%s", player->connections[index]);
        if (index < player->nConnections - 1) {
            printf(", ");
        }
    }
    printf(".\n");
}

/*
 * This function checks if the game is won
 * by looking at the roomType stored in player struct.
 */
bool gameWon(struct Player* player) {

    // If the player is inside an END_ROOM then the game is won
    if (strcmp(player->roomType, "END_ROOM") == 0){
        return true;
    }
    return false; // Otherwise game is not won
}

/*
 * This function checks if a player can move into a particular room
 * by comparing that room to the rooms in player->connections
 */
bool canMove(struct Player* player, char* room) {
    int index;
    for (index = 0; index < player->nConnections; index++) {

        // If the room is in player->connections then the player can move to that room
        if (strcmp(player->connections[index], room) == 0) {
            return true;
        }
    }
    return false; // Otherwise player cannot move to that room
}

/*
 * This function changes the filepath in roomPosition to a new filepath
 * and then updates the player struct based on the new roomPosition
 */
void move(struct Player* player, char* room) {

    //First, we update the roomPosition with a new filepath
    memset(roomPosition, '\0', sizeof(roomPosition));
    sprintf(roomPosition, "%s%c%s%s", newestDirName, '/', room, "_room.txt");

    // After that, we update the player struct and adds the room into stepsTaken
    updatePlayer(player);
    memset(player->stepsTaken[player->nSteps], '\0', sizeof(player->stepsTaken[player->nSteps]));
    strcpy(player->stepsTaken[player->nSteps], room);
    player->nSteps++;
}

// This function writes the current time in currentTime.txt
void* writeTimeFile() {
    /*
     * First we're going to lock the mutex. We don't want any other
     * thread to be able to access currentTime.txt before we finish
     * writing it.
     */
    pthread_mutex_lock(&lock);

    //Next we're going to create currentTime.txt or overwrite it if it already exists.
    int file_descriptor;
    file_descriptor = open("currentTime.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    char buffer[64];     // Holds the time string
    time_t rawtime;      // Holds the raw time
    struct tm* timeinfo; // Holds the local time

    time(&rawtime); // Obtain the current time and store it in rawtime
    timeinfo = localtime(&rawtime); // Converts the raw time into local time

    // Generates a time string based on the specified format and store it in buffer
    strftime(buffer, 64, "%I:%M%p, %A, %B %d, %Y", timeinfo);

    // Write the new time string into currentTime.txt
    write(file_descriptor, buffer, strlen(buffer) * sizeof(char));

    // Unlocks the mutex after the process is finished
    pthread_mutex_unlock(&lock);

    return NULL;
}

// This is a function to read the time in currentTime.txt
void readTimeFile() {

    // Open currentTime.txt as a read-only
    int file_descriptor; // Holds a reference to the open file
    file_descriptor = open("currentTime.txt", O_RDONLY);

    // Read the contents of the file and store it in readBuffer
    char readBuffer[64];
    memset(readBuffer, '\0', sizeof(readBuffer));
    read(file_descriptor, readBuffer, sizeof(readBuffer));

    // Print the readBuffer
    printf("\n %s\n\n", readBuffer);
}

// This is our main game function
void GameStart(struct Player* player) {
    /*
     * At the beginning of the game, we first look for the newest directory,
     * find the start room file inside the directory and then update the player struct
     */
    findNewestDir();
    searchStartRoom();
    updatePlayer(player);

    char* lineEntered = NULL; // Holds the user input
    size_t bufferSize = 0;    // Holds the size of the input buffer (only for requirement)
    int numCharsEntered = 0;  // Holds the number of chars entered by user

    /*
     * First we're going to lock the mutex to prevent
     * the thread that we're going to create from proceeding
     */
    pthread_mutex_lock(&lock);

    // Initialize a pthread variable into which we're going to create our thread
    pthread_t writeTime;

    /*
     * Create a thread which executes the writeTimeFile() function.
     * Because we have locked our mutex, this thread is now "waiting"
     * for the mutex to be unlocked before it can do its job.
     */
    pthread_create(&writeTime, NULL, writeTimeFile, NULL);

    while (gameWon(player) == false) {

        // Print out the prompt
        printPlayer(player);
        printf("WHERE TO? >");

        // Get input string from user and remove its trailing newline '\n'
        numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
        lineEntered[numCharsEntered - 1] = '\0';

        if (strcmp(lineEntered, "time") == 0) // If the user entered "time"
        {
            // We're going to unlock the mutex to allow the writeTime thread to proceed
            pthread_mutex_unlock(&lock);

            // We wait until the writeTime thread is finished before continuing
            pthread_join(writeTime, NULL);

            // We're going to lock the mutex again before creating another thread
            pthread_mutex_lock(&lock);

            // Create a new writeTime thread
            pthread_create(&writeTime, NULL, writeTimeFile, NULL);

            // Read the time in currentTime.txt
            readTimeFile();
        }
        else if (canMove(player, lineEntered) == false) // If the player cannot move into the room
        {
            printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        }
        else
        {
            // Otherwise, move the player into the new room
            move(player, lineEntered);
            printf("\n");
        }
    }

    // Prints out a congratulatory message
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", player->nSteps);

    // Prints out all the rooms in stepsTaken
    int index;
    for (index = 0; index < player-> nSteps; index++) {
        printf("%s\n", player->stepsTaken[index]);
    }

    // Unlock the mutex and allow the thread to finish to prevent memory leak
    pthread_mutex_unlock(&lock);
    pthread_join(writeTime, NULL);
    free(lineEntered);
}

int main() {
    struct Player* p = playerCreate();
    GameStart(p);
    free(p);
    return 0;
}