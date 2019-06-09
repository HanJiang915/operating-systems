#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>

#define SIZE 128000

// This is an array to store unfinished background pid
int backgroundPid[100];
int nPid = 0;

// Error function used for reporting issues
void error(const char *msg, int exitStatus) {
    fprintf(stderr, "%s\n", msg);
    exit(exitStatus);
}

// This function sends a message to the client
int sendFile(int file_descriptor, char sendBuffer[], int size) {

    // Add newline '\n' to our message which is going to be our
    // new message. Newline is used as an identifier for EOF.
    char message[size+2];
    memset(message, '\0', sizeof(message));
    sprintf(message, "%s\n", sendBuffer);

    // Sends the message
    int charsWritten;
    charsWritten = write(file_descriptor, message, size+1);
    if (charsWritten < 0) {
        error("otp_enc_d: ERROR writing to socket", 2);
    }

    // Verify that the data has actually left the system
    int checkSend = -5; // Bytes remaining in send buffer
    do {
        // Check the send buffer for this socket
        ioctl(file_descriptor, TIOCOUTQ, &checkSend);
    } while (checkSend > 0); // Loop forever until send buffer for this socket is empty

    // Check if we actually stopped the loop because of an error
    if (checkSend < 0) {
        error("otp_enc_d: ERROR writing to socket", 2);
    }

    // Return the number of characters sent
    // Subtract 1 for newline
    return charsWritten-1;
}

// This function receives a message from the client and
// sends an error message to client if encountered error.
// We're going to use a while loop and adds the message
// incrementally into the buffer, to enable us to accept
// very large files.
int receiveFile(int file_descriptor, char completeMessage[], int size) {
    int charsRead;
    char readBuffer[10];
    memset(completeMessage, '\0', size);

    // Continues the loop as long as we haven't found the terminal
    while (strstr(completeMessage, "\n") == NULL) {

        // Clear the buffer and get the next chunk of message
        memset(readBuffer, '\0', sizeof(readBuffer));
        charsRead = read(file_descriptor, readBuffer, sizeof(readBuffer)-1);

        // Add that chunk to our complete message
        strcat(completeMessage, readBuffer);

        if (charsRead < 0) { // Error
            // Sends error message to client
            sendFile(file_descriptor, "?", 1);
            error("otp_enc_d: ERROR fail to read file", 2);
        } else if (charsRead == 0) { // No more message
            break; // Exit loop
        }
    }

    // Find the terminal location and remove the newline
    int terminalLocation = strstr(completeMessage, "\n") - completeMessage;
    completeMessage[terminalLocation] = '\0';

    return strlen(completeMessage);
}

// This function receives an authentication message from the client
// Sends an error message to client if receives wrong authentication
void receiveAuthentication(int file_descriptor) {
    char authentication[10];
    receiveFile(file_descriptor, authentication, 10);
    if (strcmp(authentication, "otp_enc")) {
        sendFile(file_descriptor, "?", 1);
        error("otp_enc_d: ERROR not otp_enc", 2);
    }
}

// Sends success message to client
void sendConfirmation(int file_descriptor) {
    sendFile(file_descriptor, "!", 1);
}

// Encrypt the plaintext using the key and puts it into ciphertext
void encrypt(char plaintext[], char key[], char ciphertext[], int size) {
    
    // Clear the buffer
    memset(ciphertext, '\0', sizeof(ciphertext));

    int i;
    for (i = 0; i < size; i++) {

        // Replace spaces with '@' to make our work easier
        if (plaintext[i] == ' ') {
            plaintext[i] = '@';
        }
        if (key[i] == ' ') {
            key[i] = '@';
        }

        // Convert character into integer
        int inputChar = (int)plaintext[i];
        int keyChar = (int)key[i];

        // Subtract character with 64 which is the ASCII index
        // of the first character '@'
        inputChar -= 64;
        keyChar -= 64;

        // To encrypt, add key character to input character
        // and find its remainder to 27
        int result = (inputChar + keyChar) % 27;

        // Convert back to ASCII index
        result += 64;

        // Convert integer back into character
        ciphertext[i] = (char)result + 0;

        // Replace '@' back into space
        if (ciphertext[i] == '@') {
            ciphertext[i] = ' ';
        }
    }
}

// This function checks for bad input format.
// Sends an error message to client if an error is encountered.
void checkBadInput(char input[], int size, int file_descriptor) {
    int i;

    // A good input is defined as having all
    // capital letters and spaces
    for (i = 0; i < size; i++) {
        if ((int)input[i] > 90 || ((int)input[i] < 65 && (int)input[i] != 32)) {
            sendFile(file_descriptor, "?", 1);
            error("otp_enc_d: ERROR bad input", 1);
        }
    }
}

// Checks if the key size is larger than the file size.
// Sends an error message to client if an error is encountered.
void checkSameLength(int fileSize, int keySize, int file_descriptor) {
    if (fileSize > keySize) {
        sendFile(file_descriptor, "?", 1);
        error("otp_enc_d: ERROR key is too short", 1);
    }
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

            // Shift everything on the right side to the left (yes, this is inefficient)
            nPid--;
            for (j = i-1; j < nPid; j++) {
                backgroundPid[j] = backgroundPid[j+1];
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int listenSocketFD, establishedConnectionFD, portNumber;
    int fileSize, keySize;
    socklen_t sizeOfClientInfo;
    char plaintext[SIZE];
    char key[SIZE];
    char ciphertext[SIZE];
    struct sockaddr_in serverAddress, clientAddress;
    pid_t spawnPid;

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Set up the address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress));
    portNumber = atoi(argv[1]);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Set up the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocketFD < 0) {
        error("otp_enc_d: ERROR opening socket", 1);
    }

    // Enable the socket to begin listening
    if (bind(listenSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("otp_enc_d: ERROR on binding", 2);
    }

    // Call listen for connection
    if (listen(listenSocketFD, 5) < 0) {
        error("otp_enc_d: ERROR cannot listen call", 2);
    }

    while(1) {
        checkBackgroundProcess();

        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress);
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr*)&clientAddress, &sizeOfClientInfo);
        if (establishedConnectionFD < 0) {
            fprintf(stderr, "otp_enc_d: ERROR on accept\n");
            continue;
        }

        // Spawn a new process
        spawnPid = fork();
        switch (spawnPid) {

            // Fail to spawn a new process
            case -1:
                fprintf(stderr, "otp_enc_d: ERROR fork failed\n");
                break;

            // Child process
            case 0:
                receiveAuthentication(establishedConnectionFD);
                sendConfirmation(establishedConnectionFD);
                fileSize = receiveFile(establishedConnectionFD, plaintext, SIZE);
                sendConfirmation(establishedConnectionFD);
                keySize = receiveFile(establishedConnectionFD, key, SIZE);

                checkBadInput(plaintext, fileSize, establishedConnectionFD);
                checkBadInput(key, keySize, establishedConnectionFD);
                checkSameLength(fileSize, keySize, establishedConnectionFD);
                sendConfirmation(establishedConnectionFD);

                encrypt(plaintext, key, ciphertext, fileSize);
                sendFile(establishedConnectionFD, ciphertext, fileSize);

                close(establishedConnectionFD);
                close(listenSocketFD);
                exit(0);
                break;

            // Parent process
            case 1:
                backgroundPid[nPid] = spawnPid;
                nPid++;
                close(establishedConnectionFD);
                break;
        }
    }
}