#ifndef PARENT_DATA_STRUCTURES_H_
#define PARENT_DATA_STRUCTURES_H_
#include"pipe.h"
#include"request.h"

//Hash table data structure
struct Hashtable_entry
{
    char country[30];
    pid_t worker_pid;
    FileStatsTreePtr StatsTree;//Pointer se root dentrou me tha exi ta statistics tis xoras
};
typedef struct Hashtable_entry htable_entry;    //Each entry of the hashtable is a country with
                                                //the pid of the worker that is handling it and 
                                                //a ptr to a root of a tree with the statistics 
                                                //for every date of the country

htable_entry *Hashtable;
int hashtable_size;

//Initialize hashtable.Size must be the number of the countries we have
void Hashtable_init(int table_size){
    hashtable_size=table_size;
    Hashtable = (htable_entry *)malloc(hashtable_size * sizeof(htable_entry));
    for(int i=0;i<hashtable_size;i++){
        Hashtable[i].country[0]=0;  //To know if is "empty"
        Hashtable[i].worker_pid = 0; //No worker on it yet
        Hashtable[i].StatsTree = NULL;
    } 
}

int hashfunction(char *c){
    int i=0;
    int sum=0;
    while (c[i]!='\0')
    {
        sum = sum + c[i];
        i++;
    }
    return sum % hashtable_size;
}

//Insert a country to the hashtable
void Hashtable_insert(char *c,pid_t pid){
    int index = hashfunction(c);
    int first_index = index;
    while(Hashtable[index].country[0]!=0){
        index++;
        if(index==hashtable_size){
            index=0;
        }
        if(index==first_index){
            printf("Hashtable is full\n");
            return ;
        }
    }
    strcpy(Hashtable[index].country,c);
}

//Get the index where country c is
int getHashtable_index(char *c){
    int index = hashfunction(c);
    int first_index = index;
    while (strcmp(Hashtable[index].country,c)!=0)
    {
        index++;
        if(index==hashtable_size){
            index = 0;
        }
        if(index==first_index){ //Country not found
            return -1;
        }
    }
    return index;
}

void Hashtable_Free(){
    for(int i=0;i<hashtable_size;i++){
        //free the trees
        freeFileStatsTree(Hashtable[i].StatsTree);
    }
    free(Hashtable);
}

#endif /* PARENT_DATA_STRUCTURES_H_ */