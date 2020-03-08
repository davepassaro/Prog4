//Dave Passaro
//Citation some code modeled from the lecture reading and planning guidence code
//compile gcc -std=gnu99 -g -o keygen keygen.c
//Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//declarations
void generator(int len);

int main(int argc, char *argv[]){
    if(argc < 2){return 1;}
    int len = atoi(argv[1]);
    generator(len);
    return 0;
}
void generator(int len){
    char key[len+1];
    char options[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' '};
    int x=0;
    time_t t;    
    srand((unsigned) time(&t));//seed rand()
    while (x<len){
        key[x]= options[(rand() % 28)]; 
        //give key index a char value from random index options array  
        x++;
    }
    key[len+1]='\0';
    fprintf(stdout,"%s\n",key);fflush(stdout);
}