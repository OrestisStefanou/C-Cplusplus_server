//A client for a remote directory listing service
//usage:rls hostname directory
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

#define PORTNUM 15000
#define BUFFSIZE 256

void perror_exit(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}


int write_all(int fd,char *buff,size_t size){
    int sent,n;
    for(sent=0;sent<size;sent+=n){
        if((n=write(fd,buff+sent,size-sent))==-1){
            return -1;//error
        }
    }
    return sent;
}


int main(int argc, char const *argv[])
{
    sockaddr_in servadd;//The address of the server
    hostent *hp;//to resolve server ip
    int sock,n_read;//socket and message length
    char buffer[BUFFSIZE];//to receive message

    if(argc!=3){
        printf("Usage:rls <hostname> <directory>\n");
        exit(1);
    }

    //STEP 1:Get a socket
    if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror_exit((char *)"socket");
    }

    //STEP 2:Lookup server's address and connect there
    if((hp=gethostbyname(argv[1]))==NULL){
        herror("gethostbyname");
        exit(1);
    }

    memcpy(&servadd.sin_addr,hp->h_addr,hp->h_length);
    servadd.sin_port=htons(PORTNUM);//set port number
    servadd.sin_family = AF_INET;//set socket type
    if(connect(sock,(sockaddr *)&servadd,sizeof(servadd))!=0){
        perror_exit((char *)"connect");
    }

    //STEP 3:send directory name + newline
    if(write_all(sock,(char *)argv[2],strlen(argv[2]))==-1){
        perror_exit((char *)"write");
    }
    if(write_all(sock,(char *)"\n",1)==-1){
        perror_exit((char *)"write");
    }

    //STEP 4:read back results and send them to stdout
    while((n_read=read(sock,buffer,BUFFSIZE))>0){
        if(write_all(STDOUT_FILENO,buffer,n_read)<n_read){
            perror_exit((char *)"fwrite");
        }
    }
    close(sock);
    return 0;
}
