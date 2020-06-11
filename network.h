#ifndef NETWORK_H_
#define NETWORK_H_

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include<netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include<arpa/inet.h>
#include<poll.h>
#include"pipe.h"

//Struct to send query info to workers
struct queryinfo{
    char query[25];
    char VirusName[30];
    Date entry_date;
    Date exit_date;
    char Country[30];
    int record_id;
};
typedef struct queryinfo QueryInfo;

//Read query info from a socket.Returns -1 in case of error
int read_QueryInfo(int fd,char *buf,QueryInfo *info){
    int nbytes,k=0,flag=0;
    while(1){
        nbytes = read(fd,&buf[k],1);
        if(buf[k]=='\n' || nbytes<=0){
            if(k==0){
                flag=1; //Got error or connection closed by client
            }
            break;
        }
        k++;
    }
    buf[++k] = '\0';
    if(nbytes<=0 && flag==1){
        return -1;
    }
    sscanf(buf,"%s %s %d-%d-%d %d-%d-%d %s %d\n",info->query,info->VirusName,&info->entry_date.day,
    &info->entry_date.month,&info->entry_date.year,&info->exit_date.day,&info->exit_date.month,
    &info->exit_date.year,info->Country,&info->record_id);
    return 0;    
}

//Get sockaddr,IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Add a new file descriptor to the set(For poll)
void add_to_pfds(struct pollfd *pfds[],int newfd,int *fd_count,int *fd_size){
    //If we don't have room,add more space in the pfds array
    if(*fd_count == *fd_size){
        *fd_size *=2;   //Double it
        *pfds = realloc(*pfds,sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; //Check ready to read

    (*fd_count)++;
}

//Remove a file descriptor from the set(Poll)
void del_from_pfds(struct pollfd pfds[],int i,int *fd_count){
    //Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];
    (*fd_count)--;
}

//Read a request from a socket,Returns -1 if connection closed of error occured,else returns 0
int read_request(int fd,char *buf){
    int nbytes,k=0,flag=0;
    while(1){
        nbytes = read(fd,&buf[k],1);
        if(buf[k]=='\n' || nbytes<=0){
            if(k==0){
                flag=1; //Got error or connection closed by client
            }
            break;
        }
        k++;
    }
    buf[++k] = '\0';
    if(nbytes<=0 && flag==1){
        return -1;
    }
    return 0;
}

//Send a message through the network(returns -1 in case of error)
//If flag=0->send the message close socket file descriptor and return
//if flag=1->send the message and return the file descriptor of the socket
int send_message(char *server_name,int port,char *message,int flag){
    int server_port,socket_fd;
    struct hostent *server_host;
    struct sockaddr_in server_address;

    server_port = port;

    /* Get server host from server name. */
    server_host = gethostbyname(server_name);

    /* Initialise IPv4 server address with server host. */
    memset(&server_address, 0, sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr, server_host->h_length);

    /* Create TCP socket. */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    /* Connect to socket with server address. */
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1) {
		perror("connect");
        exit(1);
	}

    int res = write(socket_fd,message,strlen(message));
    if(res<strlen(message))
        return -1;
    
    if(flag==0){
        close(socket_fd);
        return 0;
    }else
    {
        return socket_fd;
    }
    
}
#endif /* BUFFER_H_ */