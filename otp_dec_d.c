//gcc -std=gnu99 -g -o  otp_enc_d otp_enc_d.c      
//otp_enc_d 54321 &

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

void decipher(char * key, char * message, char * cip);
int redoRecv(int establishedConnectionFD,char *buffer,int bufSize,int bufIdx);
void recvInput(int establishedConnectionFD, char *message);
void cipher(char * key, char * message, char * cip);
int checkInput(int size, char *message, char *toCheck);
void sendCip(int socketFD, char * message);
void recvInput2(int socketFD, char * completeMessage,int sig);
void sendInput2(int socketFD, char * message);

int main(int argc, char *argv[])
{
    char toCheck[] = {'a','b','c','d','e','f','g','h','i','j', 
    'k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','!','@','#','$','%','^','&','*' ,
    '(',')',',','\'','"','-','_','[',']','{','}','`','~','\\','|','?','.','<','>'}; 
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
    bool startOver;
    int childExitMethod;
	struct sockaddr_in serverAddress, clientAddress;
    int bufSize=0;
    int secretCode;
    int rereadNum=0;
    int remainChars=0;
    int redoBufSize=0;
    int forkReturn=-5;
    int children=0;
    int invalidReturn;
    char message[70010];        
    char key[70010];
    char cip[70010];
    int bufIdx=0;
	//if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args
	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) fprintf(stderr,"ERROR listening socket SERVER");fflush(stderr);


	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		 fprintf(stderr,"ERROR binding SERVER ");fflush(stderr);
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
    while(forkReturn!=0){//kill -TERM {job number} to kill
        memset(key, '\0',sizeof(key));
        memset(key, '\0',sizeof(cip));//reset all arrays
        memset(key, '\0',sizeof(message));
        // Accept a connection, blocking if one is not available until one connects
        
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        do{
            establishedConnectionFD=-5;
            establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
            if (establishedConnectionFD < 0){ fprintf(stderr,"ERROR  accept SERVER");fflush(stderr);}
            if( children>=5 ){
                wait(&childExitMethod);
                children--;
                forkReturn = fork();
                children++;}//limit to five
            else{
                forkReturn = fork();
                children++;} //if less than 5 allow more
        }while(forkReturn!=0);   
       // printf("\nSERVER: Connected Client at port %d\n",ntohs(clientAddress.sin_port));
        charsRead = recv(establishedConnectionFD, &secretCode, sizeof(uint32_t),0); // Read the code 
        if (charsRead < 0) error("ERROR reading from socket1 sc");
        int code2=6;
        charsRead = send(establishedConnectionFD, &code2, sizeof(uint32_t),0); // Read the code 
        if (charsRead < 0) error("ERROR reading from socket1 sc");
        if(secretCode != 6){
            fprintf(stderr,"SERVER: Incorrect entry code for otp_enc_d\n");fflush(stderr);
            exit(1);}
        // Get the message size from the client (in bufSize) then send key and message
        //fprintf(stderr,"SERVER: ecfd1= %d\n",establishedConnectionFD);fflush(stderr);
        memset(message, '\0', sizeof(message)); // Clear the buffer
        recvInput2(establishedConnectionFD,message,1);
        //printf("message: %s\n", message);
        //printf("key: %s\n", key);

        memset(key, '\0', sizeof(key)); // Clear the buffer
        recvInput2(establishedConnectionFD,key,0);
      //  printf("key: %s\n", key);
        //printf("message: %s\n", message);
        //printf("key: %s\n", key);

        //fprintf(stderr,"SERVER: ecfd2= %d\n",establishedConnectionFD);fflush(stderr);
    /*    if(checkInput(strlen(key), key, toCheck)!=0){
            fprintf(stderr,"SERVER: Invalid input");fflush(stderr);
            exit(1);
        }
        checkInput(strlen(message),message, toCheck);
        if(checkInput(strlen(key), key, toCheck)!=0){
            fprintf(stderr,"SERVER: Invalid input");fflush(stderr);
            exit(1);
        }*/
        //printf("%s\n%d", message,strlen( message));
        //printf("%s\n%d",key,strlen(key));


        //if(strlen(key) < strlen(message)){
        //    fprintf(stderr,"SERVER: Key too small for message");fflush(stderr);
        //    exit(1);
       // }
        char  decoded[70010];
        memset(decoded,'\0',sizeof(decoded));
        decipher(key, decoded, message);

        //decipher(key, message, cip);
       // printf(" %s\n",decoded);

        sendInput2(establishedConnectionFD, decoded);
        //decipher(key, message, cip);
        //sendInput2(establishedConnectionFD, message);
    }
    int checkSend = -5;  // Bytes remaining in send buffer
    /*do
    {
    ioctl(listenSocketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
    //printf("checkSend: %d\n", checkSend);  // Out of curiosity, check how many remaining bytes there are:
    }
    while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
    if (checkSend < 0)  // Check if we actually stopped the loop because of an error
    error("ioctl error");*/
    close(listenSocketFD); // Close the listening socket
	exit(0); 
}

void sendInput2(int socketFD, char * message){
    //char *sendBuffer=[10];
    strcat(message,"@@<<<<<<");//terminator
    int bufSize=strlen(message);
    int charsWritten=0;
   // while(bufSize>0){
    charsWritten = send(socketFD, message,strlen(message), 0); // Write to the server           
     //   if (charsWritten < 0) error("CLIENT: ERROR writing to socket1");
     //   bufSize -= charsWritten;
      //  if(bufSize != 0){fprintf(stdout,"error12C");}
  //  }
}
void recvInput2(int socketFD, char * completeMessage, int sig){
    char readBuffer[10];//cite lecture!!!!!!!!!
    char *finished="$$$";
    while (strstr(completeMessage, "@@<<<<<<") == NULL) // As long as we haven't found the terminal...
    {
        memset(readBuffer, '\0',10); // Clear the buffer
        int r = recv(socketFD, readBuffer, sizeof(readBuffer) - 1, 0); // Get the next chunk
        strcat(completeMessage, readBuffer); // Add that chunk to what we have so far
       // if(sig==1) printf("%s\n", completeMessage);
        if (r == -1) { printf("r == -1\n"); break; } // Check for errors
        if (r == 0) { printf("r == 0\n"); break; }
    }
    int terminalLocation = strstr(completeMessage, "@@<<<<") - completeMessage; // Where is the terminal
   // if(sig==1) printf("%s\n",completeMessage);
    completeMessage[terminalLocation] = '\0'; // End the string early to wipe out the terminal
   //if(sig==1) printf("%s\n",completeMessage);
    int charsWritten = send(socketFD, finished,strlen(finished), 0); // Write to the server           
    //if(sig==1) printf("%s\n",completeMessage);
}









void cipher(char * key, char * message, char * cip){
    int x=0;
    int offset,newOffset,cipherOffset, keyOffset;
    memset(cip, '\0',sizeof(cip));
    for(x=0;x<strlen(message);x++){
        if(message[x]==' '){
            message[x] = '['  ;
        }// ------------set to int 91 after 'Z' to keep subtraction of 'A' correct
        offset = message[x] - 'A' ;
        keyOffset = key[x] - 'A' ;
        newOffset = offset + keyOffset ;
        if(newOffset>26){newOffset -= 27;}
        cipherOffset = newOffset % 27 ;
        cip[x] = cipherOffset + 'A';
        //fprintf(stdout,"%d= %d + %d ",cipherOffset, offset, keyOffset);
    }
    //fprintf(stdout,"cip:\n%s\n",cip);

}
void decipher(char * key, char * message, char * cip){
    int x=0;
    int offset,newOffset,cipherOffset, keyOffset;
    memset(message, '\0',sizeof(message));
    for(x=0;x<strlen(cip);x++){
        offset = cip[x] - 'A' ;
        keyOffset = key[x] - 'A' ;
        newOffset = offset - keyOffset ;
        if(newOffset<0){newOffset += 27;}
        cipherOffset = newOffset % 27 ;
        //fprintf(stdout,"%d= %d + %d ",cipherOffset, offset, keyOffset);

        message[x] = cipherOffset + 'A';
        if(message[x]=='['){
            message[x]=' ';
        }
    }
    //fprintf(stdout,"message:\n%s\n",message);
}
