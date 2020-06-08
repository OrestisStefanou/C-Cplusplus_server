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

//Get sockaddr,IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


#endif /* BUFFER_H_ */