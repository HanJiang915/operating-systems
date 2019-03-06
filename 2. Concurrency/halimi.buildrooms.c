/*********************************************************************************
 * Author: Ivan Timothy Halim
 * Description: This is a text-driven adventure style game where the goalis to get to the end.
 * This particular file generates rooms for the game
 * Date: 2/15/2019
 *********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

typedef enum {true, false} bool;

// List of all the room names
char* roomNames[10] = {"AQUILA", "BONES", "CYCLOPS", "DIABLO", "ESPADA", "FALCO", "GARGOYLE", "HAMMERHEAD", "INDIGO", "KAIROS"};
char directoryName[32]; // Holds the name of the new directory

struct Room {
    char* name;
    char* type;
    struct Room* connections[6];
    int nConnections;
};

// A graph is just an array of rooms that are connected to each other
struct Graph {
    struct Room* rooms[7];
};

// This is a function to create the room struct
struct Room* RoomCreate(char* name) {
    struct Room* room = malloc(sizeof(struct Room));
    assert(room);
    room->name = name;
    room->type = "MID_ROOM"; // Set room type as MID_ROOM by default
    room->nConnections = 0;
    return room;
}

/*
 * This function checks if all of the rooms in a graph
 * has more than 3 connections
 */
bool IsGraphFull(struct Graph* graph) {
    assert(graph);
    int index;
    for (index = 0; index < 7; index++) {

        // If any of the room has less than 3 connections then the graph is not full
        if (graph->rooms[index]->nConnections < 3) {
            return false;
        }
    }

    // If all of the room has more than 3 connections then the graph is full
    return true;
}

// This function checks if 2 rooms can connect to each other
bool CanConnect(struct Room* x, struct Room* y) {
    assert(x && y);

    /*
     * The maximum number of connection for any given room is 6
     * So if any of the room already has 6 connections
     * then the rooms cannot connect.
     */
    if (x->nConnections == 6 || y->nConnections == 6) {
        return false;
    }

    // Next we're going to check if the rooms are already connected to each other
    // 2 rooms cannot connect to each other more than once
    int index;
    for (index = 0; index < x->nConnections; index++) {

        // If room y is already inside the connections of x
        // Room x and room y are already connected
        if (x->connections[index] == y) {
            return false;
        }
    }
    return true;
}

// This function connects 2 rooms to each other
void ConnectRoom(struct Room* x, struct Room* y) {
    assert(x && y && (CanConnect(x, y) == true));
    x->connections[x->nConnections] = y;
    y->connections[y->nConnections] = x;
    x->nConnections++;
    y->nConnections++;
}

// This function randomize the order of an array of ints
void Randomize(int array[], int length) {
    int index;
    for (index = 0; index < length; index++) {
        int temp = array[index];
        int randomIndex = rand() % length;
        array[index] = array[randomIndex];
        array[randomIndex] = temp;
    }
}

// This function randomize the order of an array of char*
void Shuffle(char* array[], int length) {
    int index;
    for (index = 0; index < length; index++) {
        char* temp = array[index];
        int randomIndex = rand() % length;
        array[index] = array[randomIndex];
        array[randomIndex] = temp;
    }
}

// This function adds random connections between the rooms in a graph
void AddRandomConnections(struct Graph* graph) {
    assert(graph);

    // Create an array of ints from 0 to 6
    // We're going to use this array to determine which rooms connect
    // By randomizing the array and taking the first 2 numbers
    int array[7];
    int index;
    for (index = 0; index < 7; index++) {
        array[index] = index;
    }

    // While the graph is not full, continue adding connections in the graph
    // Randomizing the int array each time
    while (IsGraphFull(graph) == false) {
        Randomize(array, 7);

        // We only connect the rooms if they can connect
        if (CanConnect(graph->rooms[array[0]], graph->rooms[array[1]]) == true) {
            ConnectRoom(graph->rooms[array[0]], graph->rooms[array[1]]);
        }
    }
}

// This function creates a graph struct
struct Graph* GraphCreate() {
    struct Graph* graph = malloc(sizeof(struct Graph));
    assert(graph);

    // First we're going to randomize the roomName array
    // and select 7 names to be the name of our rooms.
    Shuffle(roomNames, 10);
    int index;
    for (index = 0; index < 7; index++) {
        graph->rooms[index] = RoomCreate(roomNames[index]);
    }

    graph->rooms[0]->type = "START_ROOM"; // Choose a random room to be the starting point
    graph->rooms[6]->type = "END_ROOM";   // Choose a random room to be the end point

    // Connect the rooms until the graph is full
    AddRandomConnections(graph);

    return graph;
}

// This function frees the memory of a graph
void FreeGraph(struct Graph* graph) {
    assert(graph);

    // Before we free the graph we must free all the rooms inside it
    int index;
    for (index = 0; index < 7; index++) {
        free(graph->rooms[index]);
    }

    free(graph);
}

// This function creates the room directory
void CreateRoomDir() {
    char* prefix = "halimi.rooms."; // The prefix of our directory

    // Get the process id in which we run our program
    // Process id is going to be different every time we
    // run buildrooms
    int pid = getpid();

    // We combine our prefix and the process id to create our directory name
    memset(directoryName, '\0', sizeof(directoryName));
    sprintf(directoryName, "%s%d", prefix, pid);

    // Make a new directory using the new directory name
    mkdir(directoryName, 0777);
}

// This function takes a room struct as an argument,
// creates a room file and prints out the information
// inside the room struct into the new file
void CreateRoomFile(struct Room* room) {
    assert(room);

    // First we create our file path by combining the directory name with the name of the room
    char newFilePath[32];
    memset(newFilePath, '\0', sizeof(newFilePath));
    sprintf(newFilePath, "%s%c%s%s", directoryName, '/', room->name, "_room.txt");

    // Create a new file in the address specified by newFilePath
    int file_descriptor;
    file_descriptor = open(newFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    char content[64]; // Holds the string generated by sprintf

    // Print out the room name
    memset(content, '\0', sizeof(content));
    sprintf(content, "ROOM NAME: %s\n", room->name);
    write(file_descriptor, content, strlen(content) * sizeof(char));

    // For each room in connections, print out the room in the room file
    int index;
    for (index = 0; index < room->nConnections; index++) {
        memset(content, '\0', sizeof(content));
        sprintf(content, "CONNECTION %d: %s\n", index, room->connections[index]->name);
        write(file_descriptor, content, strlen(content) * sizeof(char));
    }

    // Print out the room type
    memset(content, '\0', sizeof(content));
    sprintf(content, "ROOM TYPE: %s\n", room->type);
    write(file_descriptor, content, strlen(content) * sizeof(char));
}

/*
 * This function generates the new directory and creates
 * a room file for each room struct in the graph
 */
void PrintGraph(struct Graph* graph) {
    assert(graph);
    CreateRoomDir();
    int index;
    for (index = 0; index < 7; index++) {
        CreateRoomFile(graph->rooms[index]);
    }
}

int main() {
    srand(time(NULL));
    struct Graph* graph = GraphCreate();
    PrintGraph(graph);
    FreeGraph(graph);
    return 0;
}