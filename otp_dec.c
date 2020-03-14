//gcc -std=gnu99 -g -o  otp_enc  otp_enc.c

//todo 
/* 
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
#include <sys/ioctl.h>

//bool emulation
typedef enum
{
  FALSE,
  TRUE
} bool;


void error(const char *msg) { 
    fprintf(stderr,"SERVER: ");fflush(stdout);
    perror(msg); exit(1); } // Error function used for reporting issues
int redoSend(int socketFD,int buffer, int bufSize, int bufIdx);
void sendInput(int socketFD, char * message);
void recvInput(int establishedConnectionFD, char * message);
void decipher(char * key, char * message, char * cip);
void recvInput2(int socketFD, char * completeMessage);
void sendInput2(int socketFD, char * message);
void checkInput(int size, char *message, char *toCheck);
int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead, secretCode,j;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[1001];
    char bigMessage[70010];
    char decoded[70010];
    char key[70010];
    char toCheck[] = {'a','b','c','d','e','f','g','h','i','j', 
    'k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','!','@','#','$','%','^','&','*' ,
    '(',')',',','\'','"','-','_','[',']','{','}','`','~','\\','|','?','.','<','>'}; 
    char * p;
    int bufSize=0;
    int bufIdx=0;
    int remainChars = 0;
    bool startOver=FALSE;
    int haveRead=0;
    FILE *fptr;
    char * fileName;
	//if (argc < 5) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args
	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL)  { fprintf(stderr, "CLIENT: ERROR, no such host. Port: %s\n",argv[3]); exit(2); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0)  { fprintf(stderr, "CLIENT: ERROR, socket. Port: %s\n",argv[3]); exit(2); }
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		 { fprintf(stderr, "CLIENT: ERROR, connecting. Port: %s\n",argv[3]); exit(2); }

	// Get input message from user
    memset(bigMessage, '\0', sizeof(bigMessage)); // Clear out the buffer array
    
   // strcpy(fileName,
    fptr = fopen(argv[1],"r");
    if(fptr == NULL)
    {
        fprintf(stderr,"Error opening input file");   
        exit(1);//                ?             
    }

	fgets(bigMessage,70000, fptr);
    if( bigMessage[0]=='\0'){fprintf(stderr,"Nothing read from file");fflush(stderr);}
    // Get input from the user, trunc to buffer - 1 chars, leaving \0
	bigMessage[strcspn(bigMessage, "\n")] = '\0'; // Remove the trailing \n that fgets adds
    //fprintf(stdout,"\n\nfile = %s\n\n", bigMessage);fflush(stdout);

    // Get key
	//printf("CLIENT: Enter text to send to the server, and then hit enter: ");
    memset(key, '\0', sizeof(key)); // Clear out the buffer array
   // strcpy(fileName,
    fclose(fptr);
    fptr = fopen(argv[2],"r");
    if(fptr == NULL)
    {
        fprintf(stderr,"Error opening key file");   
        exit(1);//                ?             
    }

	fgets(key,70000, fptr);
    if( key[0]=='\0'){fprintf(stderr,"Nothing read from file");fflush(stderr);}
    // Get input from the user, trunc to buffer - 1 chars, leaving \0
	key[strcspn(key, "\n")] = '\0'; // Remove the trailing \n that fgets adds
    //send message size to server
   // fprintf(stdout,"\nkey = %s\n\n", key);fflush(stdout);
	bufSize = strlen(bigMessage);
    checkInput(bufSize, bigMessage, toCheck);
    int keySize = strlen(key);
    checkInput(keySize, key, toCheck);
    //printf("keySize, %d bufSize %d", keySize, bufSize);fflush(stdout);
    if (keySize<bufSize){
        fprintf(stderr,"CLIENT: Key too small for message");fflush(stderr);
        exit(1);
    }
    //printf("CLIENT: keySize = %d, messSize = %d",keySize, bufSize);fflush(stdout);
    secretCode = 6;
    int code2;
    charsWritten = send(socketFD, &secretCode,sizeof(uint32_t), 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket2");
    charsRead = recv(socketFD, &code2,sizeof(uint32_t), 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket2");
    if(code2!=6){
        fprintf(stderr,"otp_dec cannot talk to otp_enc_d.\n");fflush(stderr); 
        fprintf(stderr, " Port: %s\n",argv[3]); exit(2); }
    
    
    //fprintf(stdout,"\ncomplete send1\n");fflush(stdout);

    sendInput2(socketFD, bigMessage);
        sendInput2(socketFD, key);

    memset(decoded,'\0',sizeof(decoded));
    recvInput2(socketFD, bigMessage);
   // fprintf(stdout,"%s\n",bigMessage);fflush(stdout);
    //memset(decoded,'\0',sizeof(decoded));
    //decipher(key, decoded, bigMessage);
    printf("%s\n",bigMessage);fflush(stdout);

    // Send message to server

    //	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
    //fprintf(stdout,"\ncomplete send2\n");fflush(stdout);

	// Get return message from server
    int checkSend = -5;  // Bytes remaining in send buffer
    do
    {
    ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
    //printf("checkSend: %d\n", checkSend);  // Out of curiosity, check how many remaining bytes there are:
    }
    while (checkSend > 0);  // Loop forever until send buffer for this socket is empty

    if (checkSend < 0)  // Check if we actually stopped the loop because of an error
    error("ioctl error");
	close(socketFD); // Close the socket
	return 0;
}
void sendInput2(int socketFD, char * message){
    //char *sendBuffer=[10];
    strcat(message,"@@<<<<<<");//terminator
    int bufSize=strlen(message);
    //printf("C1   %s\n",message);fflush(stdout);
    int charsWritten=0;
   // while(bufSize>0){
        charsWritten = send(socketFD, message,strlen(message), 0); // Write to the server           
        if (charsWritten < 0) error("CLIENT: ERROR writing to socket1");
            //printf("cw   %d\n",charsWritten);fflush(stdout);

     //   bufSize -= charsWritten;
      //  if(bufSize != 0){fprintf(stdout,"error12C");}
  //  }
    char finished[4];
    memset(finished,'\0',sizeof(finished));
    recv(socketFD, finished, sizeof(finished) - 1, 0);
  //  printf("message sent\n");

}
void recvInput2(int socketFD, char * message){
    char readBuffer[10];//cite lecture!!!!!!!!!
        memset(message, '\0', sizeof(message)); // Clear the buffer
    while (strstr(message, "@@<<<<<<") == NULL) // As long as we haven't found the terminal...
    {
        memset(readBuffer, '\0', sizeof(readBuffer)); // Clear the buffer
        int r = recv(socketFD, readBuffer, sizeof(readBuffer) - 1, 0); // Get the next chunk
        strcat(message, readBuffer); // Add that chunk to what we have so far
       // printf("PARENT: Message received from child: \"%s\", total: \"%s\"\n", readBuffer, completeMessage);
        if (r == -1) { printf("r == -1\n"); break; } // Check for errors
        if (r == 0) { printf("r == 0\n"); break; }
    }
    int terminalLocation = strstr(message, "@@<<<<") - message; // Where is the terminal
    message[terminalLocation] = '\0'; // End the string early to wipe out the terminal
    //printf("%s\n",message);
}



void checkInput(int size, char *message, char *toCheck){
    for (int i=0;i<size;i++){
        for(int j=0;j<sizeof(toCheck);j++){//for each char in message check if correct 
            if (message[i] == toCheck[j]){
                fprintf(stderr,"Error: incorrect input chars detected\n");  fflush(stderr); 
                exit(1);
            } 
        }
    }
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
    //fprintf(stdout,"%s\n",message);
}