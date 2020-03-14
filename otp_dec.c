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
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

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
    //checkInput(bufSize, bigMessage, toCheck);
    int keySize = strlen(key);
   // checkInput(keySize, key, toCheck);
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
        fprintf(stderr,"otp_dec cannot talk to otp_enc_d.\n");fflush(stderr);exit(1);
    }
    
    
    sendInput2(socketFD, key);
    //fprintf(stdout,"\ncomplete send1\n");fflush(stdout);

    sendInput2(socketFD, bigMessage);
    memset(decoded,'\0',sizeof(decoded));
    recvInput2(socketFD, bigMessage);
    fprintf(stdout,"%s\n",bigMessage);fflush(stdout);
    memset(decoded,'\0',sizeof(decoded));
    decipher(key, decoded, bigMessage);
    fprintf(stdout,"%s\n",decoded);fflush(stdout);

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
    strcat(message,"@@<<<<");//terminator
    int bufSize=strlen(message);
    int charsWritten=0;
   // while(bufSize>0){
        charsWritten = send(socketFD, message,strlen(message), 0); // Write to the server           
     //   if (charsWritten < 0) error("CLIENT: ERROR writing to socket1");
     //   bufSize -= charsWritten;
      //  if(bufSize != 0){fprintf(stdout,"error12C");}
  //  }
}
void recvInput2(int socketFD, char * completeMessage){
    char readBuffer[10];//cite lecture!!!!!!!!!
       // memset(completeMessage, '\0', sizeof(completeMessage)); // Clear the buffer
    while (strstr(completeMessage, "@@<<<<") == NULL) // As long as we haven't found the terminal...
    {
        memset(readBuffer, '\0', sizeof(readBuffer)); // Clear the buffer
        int r = recv(socketFD, readBuffer, sizeof(readBuffer) - 1, 0); // Get the next chunk
        strcat(completeMessage, readBuffer); // Add that chunk to what we have so far
       // printf("PARENT: Message received from child: \"%s\", total: \"%s\"\n", readBuffer, completeMessage);
        if (r == -1) { printf("r == -1\n"); break; } // Check for errors
        if (r == 0) { printf("r == 0\n"); break; }
    }
    int terminalLocation = strstr(completeMessage, "@@<<<<") - completeMessage; // Where is the terminal
    completeMessage[terminalLocation] = '\0'; // End the string early to wipe out the terminal
   // printf("%s\n",message);
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
int redoSend(int socketFD,int buffer, int bufSize, int bufIdx){
        int charsWritten = send(socketFD, buffer+bufIdx,strlen(bufSize), 0); // Write to the server            if (charsRead < 0) error("ERROR reading from socket");
    return charsWritten;
}

void sendInput(int socketFD, char * message){
    int bufSize = strlen(message);
    char buffer[1001];
    bool startOver = FALSE;
    int haveRead= 0;//init to 0 as is index
    int remainChars=0;
    int charsWritten,bufIdx;
    int j=0;
    charsWritten = send(socketFD, &bufSize,sizeof(uint32_t), 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket1");
    j=0;
    do{//check for incomplete message and re revc 
        // Read the client's message from the socket
        //fprintf(stdout,"\nj=%d",j);fflush(stdout);
        j++;
	    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
        if(bufSize <= 1000 && startOver == FALSE){ //if buff less than 1000 just one loop  
           // fprintf(stdout,"\n1\n");fflush(stdout);
            remainChars=0;
            charsWritten = send(socketFD, message,strlen(message), 0); // Write to the server           
        	if (charsWritten < 0) error("CLIENT: ERROR writing to socket2");
            remainChars = bufSize-charsWritten;//sub from bufsize not 1000 as is size we wanted to send
            while(remainChars!=0){
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsWritten = redoSend(socketFD, message, remainChars, bufIdx);//new bufSize is remainChars
            	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
                remainChars = remainChars - charsWritten;
            }
            startOver = FALSE;
        }
        else if(bufSize <= 1000 ){ //if not first loop but less than 1000 chars (put in finalBuf) is last loop
           // fprintf(stdout,"\n2\nbufSize= %d\n haveRead= %d",bufSize,haveRead);fflush(stdout);
            charsWritten = send(socketFD, message+haveRead,strlen(message)-haveRead, 0); // Write the rest to the server  
            if (charsWritten < 0) error("ERROR reading from socket3");
            remainChars = bufSize-charsWritten;//dif from indended bufSize
            while(remainChars!=0){
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsWritten = redoSend(socketFD, buffer, remainChars, bufIdx);//new bufSize is remainChars
                remainChars = remainChars - charsWritten; 
            }
            startOver = FALSE;
        }
        else{//just read 1000 chars and sub bufsize and reloop
            //fprintf(stdout,"\n3\n");fflush(stdout);
            for (int i=0; i<1000;i++){//copy message portion to be sent
                buffer[i] = message[i+haveRead];
                //fprintf(stdout,"%c",buffer[i]);fflush(stdout);
            }
            //fprintf(stdout,"%s",buffer);fflush(stdout);
            charsWritten = send(socketFD, buffer,strlen(buffer), 0); // Write to the server  1000 chars      
            if (charsWritten < 0) error("CLIENT: ERROR writing to socket4");
            remainChars = 1000-charsWritten;
            while(remainChars!=0){
                fprintf(stdout,"\nincomplete send\n");fflush(stdout);
                fprintf(stdout,"\ncharsWritten= %d\n",charsWritten);fflush(stdout);
                bufIdx= bufSize-remainChars;//get params for redo send call, bufIdx to get to where message was left off in buffer
                charsWritten = redoSend(socketFD, buffer, remainChars, bufIdx);//new bufSize is remainChars
                remainChars = remainChars - charsWritten;
            }
            haveRead += 1000;
            startOver=TRUE;
            bufSize = (bufSize - 1000);//update chars to send after this loop cycle
        }
    }while(startOver == TRUE);
    /*memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	int charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket,make sure send over
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
    int sent=0;
    remainChars = 39 - charsRead;//update remaining
    while(remainChars!=0){
        sent+= charsRead;
        fprintf(stdout,"err2c\n");fflush(stdout);
        charsRead = recv(socketFD, &buffer[sent], sizeof(buffer)-sent, 0);//new bufSize is remainChars, charsRead is updated
        if (charsRead < 0) error("ERROR reading from socket");
        remainChars = remainChars - charsRead;//update remaining
    }
	fprintf(stdout,"CLIENT: I received this from the server: %s\n", buffer);fflush(stdout);*/

}



void recvInput(int establishedConnectionFD, char * message){
    int bufSize,remainChars,charsRead,bufIdx,sent;
    char buffer[1001];
    bool startOver=FALSE;
    memset(message,'\0',70000);
    charsRead = recv(establishedConnectionFD, &bufSize, sizeof(uint32_t),0); // Read the client's message from the socket
    if (charsRead < 0) error("ERROR reading from socket1");
   // printf(": bufSize = %d\n",bufSize);fflush(stdout);
    // Get the message from the client and display it
    int j=0;
    do{//check for incomplete message and re revc 
        // Read the client's message from the socket
        //fprintf(stdout,"j= %d\n",j);fflush(stdout);
        //j++;
        memset(buffer,'\0',1001);

        if(bufSize <= 1000 && startOver == FALSE){ //if buff less than 1000 just one loop (and first loop)  
           // fprintf(stdout,"s1\n");fflush(stdout);
            remainChars=0;
            charsRead = recv(establishedConnectionFD, message, bufSize, 0);
            if (charsRead < 0) error("ERROR reading from socket2");
            //fprintf(stdout,"afters1\n");fflush(stdout);

            remainChars = bufSize-charsRead;
            sent=0;
            while(remainChars>0){
                sent+= charsRead;
                fprintf(stdout,"err1s\n");fflush(stdout);
                
                charsRead = recv(establishedConnectionFD, &message[sent], sizeof(message)-sent, 0);//new bufSize is remainChars, charsRead is updated
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
            sent=0;
            while(remainChars!=0){
                sent+= charsRead;
                fprintf(stdout,"err2s\n");fflush(stdout);
                charsRead = recv(establishedConnectionFD, &buffer[sent], sizeof(buffer)-sent, 0);//new bufSize is remainChars, charsRead is updated
                if (charsRead < 0) error("ERROR reading from socket");
                remainChars = remainChars - charsRead;//update remaining
            }
            strcat(message, buffer);
            startOver = FALSE;
            //fprintf(stdout,"%s\n", buffer);fflush(stdout);

            //return(message);
        }
        else{//just read 1000 chars and sub bufsize and reloop
            //fprintf(stdout,"s3\n");fflush(stdout);
            
            bufSize = (bufSize - 1000);
            charsRead = recv(establishedConnectionFD, buffer, 1000, 0);
            if (charsRead < 0) error("ERROR reading from socket4");
            remainChars = 1000-charsRead;
            sent=0;
            while(remainChars!=0){
                sent+= charsRead;
                fprintf(stdout,"err2s\n");fflush(stdout);
                charsRead = recv(establishedConnectionFD, &buffer[sent], sizeof(buffer)-sent, 0);//new bufSize is remainChars, charsRead is updated
                if (charsRead < 0) error("ERROR reading from socket");
                remainChars = remainChars - charsRead;//update remaining
            }
            startOver=TRUE;
            strcat(message, buffer);
    //fprintf(stdout,"%s\n", buffer);fflush(stdout);

        }


    }while(startOver == TRUE);
    //printf("SERVER: I received this from the client: %s\n", message);
    // Send a Success message back to the client
    /*charsRead = send(establishedConnectionFD, "I am the server, and I got your message\n", 39, 0); // Send success back
    if (charsRead < 0) error("ERROR writing to socket5");*/
    //close(establishedConnectionFD); // Close the existing socket which is connected to the client

   // fprintf(stdout,"\n\n\n%s\n\n\n", message);fflush(stdout);
   // fprintf(stdout,"S finished\n", message);fflush(stdout);
    //ciper


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

