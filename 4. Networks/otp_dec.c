/*********************************************************************************
 * Filename: otp_dec.c
 * Author:   Ivan Timothy Halim
 * Date:     6/10/2019
 *
 * This program connects to otp_dec_d, and asks it to perform a one-time pad style
 * decryption. It sends a ciphertext and a key to the otp_dec_d server and receives
 * back a plaintext. It then outputs the plaintext to stdout or to an output file
 * if specified. Can be run in the background or foreground.
 * 
 * USAGE: otp_dec [ciphertext] [key] [port] [> output_file] [&]
 *********************************************************************************/

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
#include <netdb.h>

#define SIZE 128000

// Error function used for reporting issues
void error(const char *msg, int exitStatus) {
    fprintf(stderr, "%s\n", msg);
    exit(exitStatus);
}

// This function reads the contents of a file and puts it
// into readBuffer. It returns the number of characters read
int readFile(char* filename, char readBuffer[], int size) {
    int file_descriptor = open(filename, O_RDONLY);
    if (file_descriptor < 0) {
        error("otp_dec: ERROR cannot open file", 1);
    }
    memset(readBuffer, '\0', size);
    return read(file_descriptor, readBuffer, size);
}

// This function sends a message to the server
int sendFile(int file_descriptor, char sendBuffer[], int size) {

    // Add '@' to our message which is going to be our
    // identifier for EOF.
    char message[size+2];
    memset(message, '\0', sizeof(message));
    sprintf(message, "%s@", sendBuffer);

    // Sends the message
    int charsWritten;
    charsWritten = write(file_descriptor, message, size+1);
    if (charsWritten < 0) {
        error("otp_dec: ERROR writing to socket", 2);
    }

    // Verify that the data has actually left the system
    int checkSend = -5; // Bytes remaining in send buffer
    do {
        // Check the send buffer for this socket
        ioctl(file_descriptor, TIOCOUTQ, &checkSend);
    } while (checkSend > 0); // Loop forever until send buffer for this socket is empty

    // Check if we actually stopped the loop because of an error
    if (checkSend < 0) {
        error("otp_dec: ERROR writing to socket", 2);
    }

    // Return the number of characters sent
    // Subtract 1 for '@'
    return charsWritten-1;
}


// This function receives a message from the server.
// We're going to use a while loop and adds the message
// incrementally into the buffer, to enable us to accept
// very large files.
int receiveFile(int file_descriptor, char completeMessage[], int size) {
    int charsRead;
    char readBuffer[10];
    memset(completeMessage, '\0', size);

    // Continues the loop as long as we haven't found the terminal
    while (strstr(completeMessage, "@") == NULL) {

        // Clear the buffer and get the next chunk of message
        memset(readBuffer, '\0', sizeof(readBuffer));
        charsRead = read(file_descriptor, readBuffer, sizeof(readBuffer)-1);

        // Add that chunk to our complete message
        strcat(completeMessage, readBuffer);

        if (charsRead < 0) { // Error
            error("otp_dec: ERROR fail to read file", 2);
        } else if (charsRead == 0) { // No more message
            break; // Exit loop
        }
    }

    // Find the terminal location and remove the newline
    int terminalLocation = strstr(completeMessage, "@") - completeMessage;
    completeMessage[terminalLocation] = '\0';

    return strlen(completeMessage);
}

// This function sends an authentication message to the server
void sendAuthentication(int file_descriptor) {
    sendFile(file_descriptor, "otp_dec", 7);
}

// This function receives confirmation messages from the server
void receiveConfirmation(int file_descriptor) {
    // Message is '!' for OK and '?' for ERROR
    char confirmation[10];
    receiveFile(file_descriptor, confirmation, 10);
    if (!strcmp(confirmation, "?")) {
        exit(2);
    }
}

// This function checks for bad input format
// A good input is defined as having all
// capital letters and spaces
void checkBadInput(char input[], int size) {
    int i;
    for (i = 0; i < size; i++) {
        if ((int)input[i] > 90 || ((int)input[i] < 65 && (int)input[i] != 32)) {
            error("otp_dec: ERROR bad input", 1);
        }
    }
}

// Checks if the key size is larger than the file size
void checkSameLength(int fileSize, int keySize) {
    if (fileSize > keySize) {
        error("otp_dec: ERROR key is too short", 1);
    }
}

int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsRead;
    int fileSize, keySize;
    char plaintext[SIZE];
    char key[SIZE];
    char ciphertext[SIZE];
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;

    // Check usage & args
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Get input files
    fileSize = readFile(argv[1], ciphertext, SIZE);
    keySize = readFile(argv[2], key, SIZE);

    // Remove trailing newline
    ciphertext[fileSize] = '\0';
    fileSize--;
    key[keySize] = '\0';
    keySize--;

    // Check bad input
    checkBadInput(ciphertext, fileSize);
    checkBadInput(key, keySize);
    checkSameLength(fileSize, keySize);

    // Set up the server address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress));
    portNumber = atoi(argv[3]);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverHostInfo = gethostbyname("localhost");
    if (serverHostInfo == NULL) {
        error("otp_dec: ERROR no such host", 1);
    }
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("otp_dec: ERROR opening socket", 1);
    }

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("otp_dec: ERROR connecting", 2);
    }

    // Send authentication
    sendAuthentication(socketFD);
    receiveConfirmation(socketFD);

    // Send ciphertext and key
    sendFile(socketFD, ciphertext, fileSize);
    receiveConfirmation(socketFD);
    sendFile(socketFD, key, keySize);
    receiveConfirmation(socketFD);

    // Receive plaintext
    charsRead = receiveFile(socketFD, plaintext, SIZE);

    // Close socket
    close(socketFD);

    // Check plaintext for bad format
    checkBadInput(plaintext, charsRead);
    checkSameLength(charsRead, keySize);

    // Add newline
    plaintext[charsRead] = '\n';
    charsRead++;

    // Output plaintext
    write(STDOUT_FILENO, plaintext, charsRead);

    return 0;
}