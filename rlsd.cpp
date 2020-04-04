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


using namespace std;

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
    sockaddr_in myaddr,client;//build our address here
    int c,lsock,csock;//Listening and client sockets
    FILE *sock_fp;//stream for socket IO
    FILE *pipe_fp;//use popen to run ls
    char dirname[BUFSIZ];//from client
    char command[BUFSIZ];//for popen
    sockaddr *serverptr = (sockaddr *)&myaddr;
    sockaddr *clientptr = (sockaddr *)&client;
    hostent *rem;

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
        socklen_t clientlen = sizeof(client);
        //accept connection,ignore client address
        if((csock=accept(lsock,clientptr,&clientlen))<0){
            perror_exit((char *)"accept");
        }
        //Get clients info
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
