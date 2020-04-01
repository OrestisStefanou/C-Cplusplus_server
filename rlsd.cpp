//A remote ls server
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<time.h>
#include<string.h>
#include<ctype.h>

#define PORTNUM 15000 //rlsd listens on this port

//CHECK CLIENT'S INPUT
void sanitize(char *str){
    char *src,*dest;
    for(src=dest=str;*src;src++){
        if(*src=='/' || isalnum(*src)){
            *dest++=*src;
        }
    }
    *dest='\0';
}

void perror_exit(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}


int main(int argc, char const *argv[])
{
    sockaddr_in myaddr;//build our address here
    int c,lsock,csock;//Listening and client sockets
    FILE *sock_fp;//stream for socket IO
    FILE *pipe_fp;//use popen to run ls
    char dirname[BUFSIZ];//from client
    char command[BUFSIZ];//for popen

    //create a TCP SOCKET
    if((lsock=socket(AF_INET,SOCK_STREAM,0))<0){
        perror_exit((char *)"socket");
    }  

    //bind address to socket
    myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    myaddr.sin_port=htons(PORTNUM);//port to bind socket
    myaddr.sin_family=AF_INET;//internet address family
    if(bind(lsock,(sockaddr *)&myaddr,sizeof(myaddr))){
        perror_exit((char *)"bind");
    }

    //Listen for connections with Qsize=5
    if(listen(lsock,5)!=0){
        perror_exit((char *)"listen");
    }

    //Main loop->Accept-read-write
    while(1){
        //accept connection,ignore client address
        if((csock=accept(lsock,NULL,NULL))<0){
            perror_exit((char *)"accept");
        }
        //open socket as buffered stream
        if((sock_fp=fdopen(csock,"r+"))==NULL){
            perror_exit((char *)fdopen);
        }
        //read dirname and build ls command line
        if(fgets(dirname,BUFSIZ,sock_fp)==NULL){
            perror_exit((char *)"reading dirname");
        }
        sanitize(dirname);
        snprintf(command,BUFSIZ,"ls %s",dirname);
        //invoke ls through popen
        if((pipe_fp=popen(command,"r"))==NULL){//Popen->Fork+exec+pipe
            perror_exit((char *)"popen");
        }
        //transfer data from ls to socket
        while((c=getc(pipe_fp))!=EOF){
            putc(c,sock_fp);
        }
        pclose(pipe_fp);
        fclose(sock_fp);
    }
    return 0;
}
