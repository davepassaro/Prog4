//gcc -std=gnu99 -g -o  otp_enc_d otp_enc_d.c      
//otp_enc_d 54321 &

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
//bool emulation
typedef enum
{
  FALSE,
  TRUE
} bool;

void error(const char *msg) { 
    fprintf(stderr,"SERVERERROR: ");fflush(stdout);
    perror(msg); exit(1); } // Error function used for reporting issues

int redoRecv(int establishedConnectionFD,char *buffer,int bufSize,int bufIdx);
void recvInput(int establishedConnectionFD, char *message);

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
    bool startOver;
	struct sockaddr_in serverAddress, clientAddress;
    int bufSize=0;
    int secretCode;
    int rereadNum=0;
    int remainChars=0;
    int redoBufSize=0;
    char message[70000];        
    char key[70000];
    int bufIdx=0;
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args
	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
    while(1){//kill -TERM {job number} to kill

        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
        if (establishedConnectionFD < 0){error("ERROR on accept");}
        printf("\nSERVER: Connected Client at port %d\n",ntohs(clientAddress.sin_port));
        charsRead = recv(establishedConnectionFD, &secretCode, sizeof(uint32_t),0); // Read the code 
        if (charsRead < 0) error("ERROR reading from socket1 sc");
        if(secretCode != 9){
            fprintf(stderr,"SERVER: Incorrect entry code for otp_enc_d\n");fflush(stderr);
            continue;}
        // Get the message size from the client (in bufSize) then send key and message
        //fprintf(stderr,"SERVER: ecfd1= %d\n",establishedConnectionFD);fflush(stderr);
        recvInput(establishedConnectionFD,key);
        //fprintf(stderr,"SERVER: ecfd2= %d\n",establishedConnectionFD);fflush(stderr);
        recvInput(establishedConnectionFD,message);

        //printf("%s\n", key);
    }
    int checkSend = -5;  // Bytes remaining in send buffer
    do
    {
    ioctl(listenSocketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
    //printf("checkSend: %d\n", checkSend);  // Out of curiosity, check how many remaining bytes there are:
    }
    while (checkSend > 0);  // Loop forever until send buffer for this socket is empty

    if (checkSend < 0)  // Check if we actually stopped the loop because of an error
    error("ioctl error");
    close(listenSocketFD); // Close the listening socket
	exit(0); 
}
int redoRecv(int establishedConnectionFD,char *buffer,int bufSize,int bufIdx){
    int charsRead = recv(establishedConnectionFD, buffer+bufIdx, bufSize,0);//new bufSize is remainChars
    return charsRead;
}
void recvInput(int establishedConnectionFD, char * message){
    int bufSize,remainChars,charsRead,bufIdx;
    char buffer[1001];
    bool startOver=FALSE;
    memset(message,'\0',70000);
    charsRead = recv(establishedConnectionFD, &bufSize, sizeof(uint32_t),0); // Read the client's message from the socket
    if (charsRead < 0) error("ERROR reading from socket1");
    printf("\nSERVER: bufSize = %d\n",bufSize);fflush(stdout);
    // Get the message from the client and display it
    int j=0;
    do{//check for incomplete message and re revc 
        // Read the client's message from the socket
        //fprintf(stdout,"j= %d\n",j);fflush(stdout);
        //j++;
        memset(buffer,'\0',1000);

        if(bufSize <= 1000 && startOver == FALSE){ //if buff less than 1000 just one loop (and first loop)  
            //fprintf(stdout,"s1\n");fflush(stdout);
            remainChars=0;
            charsRead = recv(establishedConnectionFD, message, bufSize, 0);
            if (charsRead < 0) error("ERROR reading from socket2");
            remainChars = bufSize-charsRead;
            while(remainChars!=0){
                fprintf(stdout,"err1\n");fflush(stdout);
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsRead = redoRecv(establishedConnectionFD, buffer, remainChars, bufIdx);//new bufSize is remainChars, charsRead is updated
                if (charsRead < 0) error("ERROR reading from socket");
                remainChars = remainChars - charsRead;//update remaining
            }
            startOver = FALSE;

            //return(message);
        }
        else if(bufSize <= 1000){ //if not first loop (put in finalBuf) 
            //fprintf(stdout,"s2\n");fflush(stdout);
            
            remainChars=0;
            charsRead = recv(establishedConnectionFD, buffer, bufSize, 0);
            if (charsRead < 0) error("ERROR reading from socket3");
            remainChars = bufSize-charsRead;
            while(remainChars!=0){
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsRead = redoRecv(establishedConnectionFD, buffer, remainChars, bufIdx);//new bufSize is remainChars
                remainChars = remainChars - charsRead;
            }
            strcat(message, buffer);
            startOver = FALSE;
            //return(message);
        }
        else{//just read 1000 chars and sub bufsize and reloop
            //fprintf(stdout,"s3\n");fflush(stdout);
            
            bufSize = (bufSize - 1000);
            charsRead = recv(establishedConnectionFD, buffer, 1000, 0);
            if (charsRead < 0) error("ERROR reading from socket4");
            remainChars = 1000-charsRead;
            while(remainChars!=0){
                
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsRead = redoRecv(establishedConnectionFD, buffer, remainChars, bufIdx);//new bufSize is remainChars
                remainChars = remainChars - charsRead;
            }
            startOver=TRUE;
            strcat(message, buffer);
        }


    }while(startOver == TRUE);
    //printf("SERVER: I received this from the client: %s\n", message);
    // Send a Success message back to the client
    charsRead = send(establishedConnectionFD, "I am the server, and I got your message\n", 39, 0); // Send success back
    if (charsRead < 0) error("ERROR writing to socket5");
    //close(establishedConnectionFD); // Close the existing socket which is connected to the client

    fprintf(stdout,"%s\n", message);fflush(stdout);
    fprintf(stdout,"S finished\n", message);fflush(stdout);


}