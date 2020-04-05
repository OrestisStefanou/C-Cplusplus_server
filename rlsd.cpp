//A remote ls server
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<time.h>
#include<string.h>
#include<ctype.h>
#include<iostream>
#include <ctime>
#include <fstream> 
#include<signal.h>
#include<ctype.h>
#include<sys/wait.h>


using namespace std;

#define PORTNUM 15000 //rlsd listens on this port


void child_server(int newsock);
void perror_exit(char *message);
void sigchld_handler(int sig);

char *update_syslog(char *ip,char *hostname){
    char client_info[100];
    sprintf(client_info,"%s with ip:%s connected at:",hostname,ip);
    time_t current_time;
    tm *curr_tm;
    char date_string[100];
    char time_string[100];
    time(&current_time);
    curr_tm = localtime(&current_time);
	strftime(date_string, 50, "%B %d, %Y ", curr_tm);
	strftime(time_string, 50, "%T", curr_tm);
    char *date_time=strcat(date_string,time_string);
    char *final_string = strcat(client_info,date_time);
    cout<<final_string<<endl;

    //Write message to the file
    ofstream fout;
    fout.open("syslog.txt",ios::app);  
    // Write line in file 
    fout << final_string << endl;
    // Close the File 
    fout.close(); 
}


int main(int argc, char const *argv[])
{
    int port,sock,newsock;
    struct sockaddr_in server,client;
    socklen_t clientlen;
    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    struct hostent *rem;
    if(argc!=2){
        printf("Please give port number\n");
        exit(1);
    }
    port=atoi(argv[1]);
    //Reap dead children asynchronously
    signal(SIGCHLD,sigchld_handler);

    //Create socket
    if((sock=socket(AF_INET,SOCK_STREAM,0))<0){
        perror_exit((char *)"socket");
    }
    server.sin_family=AF_INET;//Internet domain
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(port);//the given port
    //Bind socket to addresss
    if(bind(sock,serverptr,sizeof(server))<0){
        perror_exit((char *)"bind");
    }
    //Listen for connections
    if(listen(sock,5)<0){
        perror_exit((char *)"listen");
    }
    printf("Listening for connections to port %d\n",port);

    while(1){
        clientlen=sizeof(client);
        //accept connection
        if((newsock=accept(sock,clientptr,&clientlen))<0){
            perror_exit((char *)"accept");
        }
        //Find client's name
        if((rem=gethostbyaddr((char *)&client.sin_addr.s_addr,sizeof(client.sin_addr.s_addr),client.sin_family))==NULL){
            herror("gethostbyaddr");
            exit(1);
        }
        printf("Accepted connection from %s\n",rem->h_name);
        //GET IP ADDRESS OF THE CLIENT
        char *IP_buffer;
        IP_buffer = inet_ntoa(*((struct in_addr*)rem->h_addr_list[0])); 
  
        printf("Ip address is %s and port %d\n",IP_buffer,client.sin_port);
        update_syslog(IP_buffer,rem->h_name);
        switch (fork()) //Create child for serving client
        {
        case -1://Error
            perror("fork");
            break;

        case 0: //Child process    
            close(sock);
            child_server(newsock);
            exit(0);
        default:
            break;
        }
        close(newsock);//parent closes socket to client
    }
}


void child_server(int newsock){
    char buf[1];
    while (read(newsock,buf,1)>0)//Receive 1 char
    {
        putchar(buf[0]);//Print received char
        //Capitalize character
        buf[0]=toupper(buf[0]);
        //Reply
        if(write(newsock,buf,1)<0){
            perror_exit((char *)"write");
        }
    }
    printf("Closing connection.\n");
    close(newsock);
}


//Wait for all dead child processes
void sigchld_handler(int sig){
    while(waitpid(-1,NULL,WNOHANG)>0);
}

void perror_exit(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}