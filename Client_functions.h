#ifndef CLIENT_FUNCTIONS_H_
#define CLIENT_FUNCTIONS_H_
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

char **requests_array;  //Array with requests for threads
int requests_array_size;

//Create requests array size:<size>
void init_requests_array(){
    requests_array = (char **)malloc(requests_array_size * sizeof(char *));
    for(int i=0;i<requests_array_size;i++){
        requests_array[i] = (char *)malloc(100 *sizeof(char));
        memset(requests_array[i],0,100);
    }
}

//Insert a request in the array
int requests_array_insert(char *r){
    for(int i=0;i<requests_array_size;i++){
        if(requests_array[i][0]==0){
            strcpy(requests_array[i],r);
            return 0;
        }
    }
    return -1;
}

//Get a request from the array
int requests_array_get(char *r){
    for (int i=0;i<requests_array_size;i++){
        if (requests_array[i][0]!=0)
        {  
            strcpy(r,requests_array[i]);
            memset(requests_array[i],0,100);
            return 0;
        }
        
    }
    return -1;
}

//Check if array is full
int requests_array_full(void){
    for (int i=0;i<requests_array_size;i++){
        if (requests_array[i][0]==0)
        {  
            return 0;
        }
        
    }
    return 1;    
}

//Check if array is empty
int requests_array_empty(void){
    for (int i=0;i<requests_array_size;i++){
        if (requests_array[i][0]!=0)
        {  
            return 0;
        }
        
    }
    return 1;    
}

void print_request_array(){
    for(int i=0;i<requests_array_size;i++){
        printf("%s",requests_array[i]);
    }
}
#endif /* CLIENT_FUNCTIONS_H_ */