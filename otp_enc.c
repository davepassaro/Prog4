//gcc -std=gnu99 -g -o  otp_enc  otp_enc.c

//todo 
/* 
    1 reject connect to otp_dec
    2 dyn arr? for storage
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

//bool emulation
typedef enum
{
  FALSE,
  TRUE
} bool;


void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues
int redoSend(int socketFD,int buffer, int bufSize, int bufIdx);


int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[1001];
    char bigMessage[30000];
    int bufSize=0;
    int bufIdx=0;
    int remainChars = 0;
    bool startOver=FALSE;
    int haveRead=-1;
	if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args
	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[2]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(argv[1]); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	// Get input message from user
	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
    memset(bigMessage, '\0', sizeof(bigMessage)); // Clear out the buffer array
	fgets(bigMessage, sizeof(bigMessage) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(bigMessage, "\n")] = '\0'; // Remove the trailing \n that fgets adds
    //send message size to server
	bufSize = strlen(bigMessage);
    printf("CLIENT: bufSize = %d",bufSize);fflush(stdout);
    charsWritten = send(socketFD, bufSize,sizeof(uint32_t), 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket1");

    do{//check for incomplete message and re revc 
        // Read the client's message from the socket
        startOver = FALSE;
        haveRead= 0;//reinit to 0 as is index
	    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
        if(bufSize <= 1000 && startOver == FALSE){ //if buff less than 1000 just one loop  
            remainChars=0;
            charsWritten = send(socketFD, bigMessage,strlen(bigMessage), 0); // Write to the server            if (charsRead < 0) error("ERROR reading from socket");
        	if (charsWritten < 0) error("CLIENT: ERROR writing to socket2");
            remainChars = bufSize-charsWritten;
            while(remainChars!=0){
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsWritten = redoSend(socketFD, bigMessage, remainChars, bufIdx);//new bufSize is remainChars
            	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
                remainChars = remainChars - charsWritten;
            }
            startOver = FALSE;
        }
        else if(bufSize <= 1000 ){ //if not first loop but less than 1000 chars (put in finalBuf) is last loop
            /*for (int i=0; i<strlen(bigMessage);i++){//put remaining message in buffer
                buffer[i] = bigMessage[i+haveRead];
            }*/
            charsWritten = send(socketFD, bigMessage+haveRead,strlen(buffer)-haveRead, 0); // Write the rest to the server  
            if (charsWritten < 0) error("ERROR reading from socket3");
            remainChars = bufSize-charsWritten;
            while(remainChars!=0){
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsWritten = redoSend(socketFD, buffer, remainChars, bufIdx);//new bufSize is remainChars
                remainChars = remainChars - charsWritten;
            }
            startOver = FALSE;
        }
        else{//just read 1000 chars and sub bufsize and reloop
            for (int i=0; i<1000;i++){
                buffer[i] = bigMessage[i+haveRead];
            }
            charsWritten = send(socketFD, buffer,strlen(buffer), 0); // Write to the server  1000 chars      
            if (charsWritten < 0) error("CLIENT: ERROR writing to socket4");
                        remainChars = bufSize-charsWritten;
            while(remainChars!=0){
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsWritten = redoSend(socketFD, buffer, remainChars, bufIdx);//new bufSize is remainChars
                remainChars = remainChars - charsWritten;
            }
            haveRead += 1000;
            startOver=TRUE;
            bufSize = (bufSize - 1000);//update chars to send after this loop cycle
        }
    }while(startOver == TRUE);
    // Send message to server

//	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	printf("CLIENT: I received this from the server: \"%s\"\n", buffer);

	close(socketFD); // Close the socket
	return 0;
}
int redoSend(int socketFD,int buffer, int bufSize, int bufIdx){
        int charsWritten = send(socketFD, buffer+bufIdx,strlen(bufSize), 0); // Write to the server            if (charsRead < 0) error("ERROR reading from socket");
    return charsWritten;
}























/*      }*/