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
#endif /* BUFFER_H_ */