#ifndef BUFFER_H_
#define BUFFER_H_
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct buffer_entry{
    int fd;
    char address[100];
};
typedef struct buffer_entry BufferEntry;

BufferEntry *fd_buffer;          //File descriptor buffer
int buffer_size;

//Initialize buffer
void buffer_init(){
    fd_buffer = (BufferEntry *)malloc(buffer_size * sizeof(BufferEntry));
    for(int i=0;i<buffer_size;i++){
        fd_buffer[i].fd=0;
        memset(fd_buffer[i].address,0,100);
    }
}

//Check if buffer is empty.Returns 1 if buffer is empty else returns 0
int buffer_is_empty(){
    for(int i=0;i<buffer_size;i++){
        if (fd_buffer[i].fd!=0)
        {
            return 0;
        }
    }
    return 1;
}

//Check if buffer is full.Returns 1 if buffer is full else returns 0
int buffer_is_full(){
    for(int i=0;i<buffer_size;i++){
        if(fd_buffer[i].fd==0){
            return 0;
        }
    }
    return 1;
}

//Insert a file descriptor,address in the buffer
void buffer_insert(int fd,char *a){
    for(int i=0;i<buffer_size;i++){
        if(fd_buffer[i].fd==0){
            fd_buffer[i].fd = fd;
            strcpy(fd_buffer[i].address,a);
            return;
        }
    }
}

//Get a file descriptor,address from the buffer
BufferEntry buffer_get(){
    BufferEntry entry;
    for (int i = 0; i < buffer_size; i++)
    {
        if(fd_buffer[i].fd!=0){
            entry.fd = fd_buffer[i].fd;
            strcpy(entry.address,fd_buffer[i].address);
            fd_buffer[i].fd = 0;
            memset(fd_buffer[i].address,0,100);
            return entry;
        }
    }
    
}

#endif /* BUFFER_H_ */