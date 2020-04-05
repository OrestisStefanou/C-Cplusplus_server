#include<stdio.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<signal.h>
#include<string.h>


void perror_exit(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}


int main(int argc, char const *argv[])
{
    int port,sock,i;
    char buf[256];
    sockaddr_in server;
    sockaddr *serverptr=(sockaddr *)&server;
    hostent *rem;
    if(argc!=3){
        printf("Please give host name and port number\n");
        exit(1);
    }
    //Create socket
    if((sock=socket(AF_INET,SOCK_STREAM,0))<0){
        perror_exit((char *)"socket");
    }
    //Find server address
    if((rem=gethostbyname(argv[1]))==NULL){
        herror("gethostbyname");
        exit(1);
    }
    port=atoi(argv[2]);
    server.sin_family=AF_INET;//Internet doamin
    memcpy(&server.sin_addr,rem->h_addr,rem->h_length);
    server.sin_port=htons(port);//server port
    //Initiate Connection
    if(connect(sock,serverptr,sizeof(server))<0){
        perror_exit((char *)"connect");
    }
    printf("Connecting to %s port:%d\n",argv[1],port);
    do{
        printf("Give input string:");
        fgets(buf,sizeof(buf),stdin);
        for(i=0;buf[i]!='\0';i++){
            //Send i-th character
            if(write(sock,buf+i,1)<0){
                perror_exit((char *)"write");
            }
            //receive i-th character transformed
            if(read(sock,buf+i,1)<0){
                perror_exit((char *)"read");
            }
        }
        printf("Received string %s\n",buf);
    }while(strcmp(buf,"END\n")!=0);
    close(sock);
}
