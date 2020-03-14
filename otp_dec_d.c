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
void recvInput2(int socketFD, char * completeMessage);
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
           // fprintf(stderr,"SERVER: Incorrect entry code for otp_enc_d\n");fflush(stderr);
            exit(1);}
        // Get the message size from the client (in bufSize) then send key and message
        //fprintf(stderr,"SERVER: ecfd1= %d\n",establishedConnectionFD);fflush(stderr);
        recvInput2(establishedConnectionFD,key);
        //fprintf(stderr,"SERVER: ecfd2= %d\n",establishedConnectionFD);fflush(stderr);
        recvInput2(establishedConnectionFD,message);
        if(checkInput(strlen(key), key, toCheck)!=0){
            fprintf(stderr,"SERVER: Invalid input");fflush(stderr);
            exit(1);
        }
        //checkInput(strlen(message),message, toCheck);
        //if(checkInput(strlen(key), key, toCheck)!=0){
        //    fprintf(stderr,"SERVER: Invalid input");fflush(stderr);
        //    exit(1);
        //}
        if(strlen(key) < strlen(message)){
            fprintf(stderr,"SERVER: Key too small for message");fflush(stderr);
            exit(1);
        }
        //printf("message pre \n%s\n", message);fflush(stdout);
        char decode[70000];
        decipher(key, decode, message);
        sendInput2(establishedConnectionFD, decode);
        //decipher(key, message, cip);
        //printf("send\n", key);fflush(stdout);
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
   // printf("exinting\n");fflush(stdout);
    close(listenSocketFD); // Close the listening socket
	exit(0); 
}


void sendInput2(int socketFD, char * message){
    //char *sendBuffer=[10];
    strcat(message,"@@");//terminator
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
        memset(completeMessage, '\0', sizeof(completeMessage)); // Clear the buffer
    while (strstr(completeMessage, "@@") == NULL) // As long as we haven't found the terminal...
    {
        memset(readBuffer, '\0', sizeof(readBuffer)); // Clear the buffer
        int r = recv(socketFD, readBuffer, sizeof(readBuffer) - 1, 0); // Get the next chunk
        strcat(completeMessage, readBuffer); // Add that chunk to what we have so far
       // printf("PARENT: Message received from child: \"%s\", total: \"%s\"\n", readBuffer, completeMessage);
        if (r == -1) { printf("r == -1\n"); break; } // Check for errors
        if (r == 0) { printf("r == 0\n"); break; }
    }
    int terminalLocation = strstr(completeMessage, "@@") - completeMessage; // Where is the terminal
    completeMessage[terminalLocation] = '\0'; // End the string early to wipe out the terminal
   // printf("%s\n",message);
}




int checkInput(int size, char *message, char *toCheck){
    for (int i=0;i<size;i++){
        for(int j=0;j<sizeof(toCheck);j++){//for each char in message check if correct 
            if (message[i] == toCheck[j]){
                fprintf(stderr,"Error: incorrect input chars detected\n");  fflush(stderr); 
                exit(1);
            } 
        }
    }
    return 0;
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
        //fprintf(stdout,"%d= %d + %d ",cipherOffset, offset, keyOffset);fflush(stdout);
        message[x] = cipherOffset + 'A';
        if(message[x]=='['){
            message[x]=' ';
        }
    }
    fprintf(stdout,"message:\n%s\n",message);
}


/*
void recvInput(int establishedConnectionFD, char * message){
    int bufSize,remainChars,charsRead,bufIdx,sent;
    char buffer[1001];
    bool startOver=FALSE;
    memset(message,'\0',70000);
    charsRead = recv(establishedConnectionFD, &bufSize, sizeof(uint32_t),0); // Read the client's message from the socket
    if (charsRead < 0) error("ERROR reading from socket1");
    //printf("\nSERVER: bufSize = %d\n",bufSize);fflush(stdout);
    // Get the message from the client and display it
    int j=0;
    do{//check for incomplete message and re revc 
        // Read the client's message from the socket
        //fprintf(stdout,"j= %d\n",j);fflush(stdout);
        //j++;
        memset(buffer,'\0',1001);

        if(bufSize <= 1000 && startOver == FALSE){ //if buff less than 1000 just one loop (and first loop)  
            //fprintf(stdout,"s1\n");fflush(stdout);
            remainChars=0;
            charsRead = recv(establishedConnectionFD, message, bufSize, 0);
            if (charsRead < 0) error("ERROR reading from socket2");
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
                fprintf(stdout,"err3s\n");fflush(stdout);
                charsRead = recv(establishedConnectionFD, &(buffer[sent]), sizeof(buffer)-sent, 0);//new bufSize is remainChars, charsRead is updated
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




/*
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


void sendCip(int socketFD, char * message){
    int bufSize = strlen(message);
    char buffer[1001];
    bool startOver = FALSE;
    int sent=0;
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
            sent=0;
            while(remainChars!=0){
                sent+= charsWritten;
                fprintf(stdout,"err4s\n");fflush(stdout);
                charsWritten = send(socketFD, &message[sent], sizeof(message)-sent, 0);//new bufSize is remainChars, charsRead is updated
                if (charsWritten < 0) error("ERROR reading from socket");
                remainChars = remainChars - charsWritten;//update remaining
            }
            startOver = FALSE;
        }
        else if(bufSize <= 1000 ){ //if not first loop but less than 1000 chars (put in finalBuf) is last loop
           // fprintf(stdout,"\n2\nbufSize= %d\n haveRead= %d",bufSize,haveRead);fflush(stdout);
            charsWritten = send(socketFD, message+haveRead,strlen(message)-haveRead, 0); // Write the rest to the server  
            if (charsWritten < 0) error("ERROR reading from socket3");
            remainChars = bufSize-charsWritten;//dif from indended bufSize
            sent=0;
            while(remainChars!=0){
                sent+= charsWritten;
                fprintf(stdout,"err5s\n");fflush(stdout);
                charsWritten = send(socketFD, message+haveRead+sent, sizeof(message+haveRead)-sent, 0);//new bufSize is remainChars, charsRead is updated
                if (charsWritten < 0) error("ERROR reading from socket");
                remainChars = remainChars - charsWritten;//update remaining
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
            sent=0;
            while(remainChars!=0){
                sent+= charsWritten;
                fprintf(stdout,"err6s\n");fflush(stdout);
                charsWritten = send(socketFD, &buffer[sent], sizeof(buffer)-sent, 0);//new bufSize is remainChars, charsRead is updated
                if (charsWritten < 0) error("ERROR reading from socket");
                remainChars = remainChars - charsWritten;//update remaining
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
