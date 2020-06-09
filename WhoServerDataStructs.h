#ifndef WHOSERVERDATASTRUCTS_H_
#define WHOSERVERDATASTRUCTS_H_
#include"pipe.h"
#include"request.h"

//Hashtable to keep the stats the workers sent us.We also keep the address
//and port of the worker that handles the country we look for
struct ServerHashTableEntry{
    char country[30];
    char worker_address[100];
    int worker_port;
    FileStatsTreePtr CountryStatsTree;
    struct ServerHashTableEntry *next;  //In case of chain
};
typedef struct ServerHashTableEntry ServerHT_Entry;

ServerHT_Entry **ServerHT;  //ServerHashtable->Array with ServerHT_Entry pointers
int ServerHT_size;

//Initialize Hashtable
void ServerHT_init(int size){
    ServerHT_size = size;
    ServerHT = (ServerHT_Entry **)malloc(size * sizeof(ServerHT_Entry *));
    for(int i=0;i<size;i++){
        ServerHT[i] = NULL;
    }
}

//HashFunction
int ServerHashFunction(char *c){
    int i=0;
    int sum=0;
    while (c[i]!='\0')
    {
        sum = sum + c[i];
        i++;
    }
    return sum % ServerHT_size;    
}

//Insert an entry in the Hashtable
void ServerHT_insert(char *a,int p,File_Stats stats){
    int index = ServerHashFunction(stats.Country);

    if(ServerHT[index]==NULL){
        //No collision insert here
        ServerHT_Entry *new_entry = (ServerHT_Entry *)malloc(sizeof(ServerHT_Entry));
        strcpy(new_entry->country,stats.Country);
        strcpy(new_entry->worker_address,a);
        new_entry->worker_port = p;
        new_entry->CountryStatsTree=NULL;
        //Insert the stats in the tree
        new_entry->CountryStatsTree = add_FileStatsTree_node(new_entry->CountryStatsTree,stats);
        new_entry->next = NULL;
        ServerHT[index] = new_entry;
        return;
    }

    if(strcmp(ServerHT[index]->country,stats.Country)==0){
        //An entry for this country already exists
        //Just insert the stats in the tree

        //Check for error first
        if(strcmp(ServerHT[index]->worker_address,a)!=0 || ServerHT[index]->worker_port!=p){
            printf("Stats for this country should come from another worker\n");
            return;
        }
        //Insert the stats in the tree
        ServerHT[index]->CountryStatsTree = add_FileStatsTree_node(ServerHT[index]->CountryStatsTree,stats);
    }

    //We have a collision so we create a chain for the new entry
    ServerHT_Entry *new_entry = (ServerHT_Entry *)malloc(sizeof(ServerHT_Entry));
    strcpy(new_entry->country,stats.Country);
    strcpy(new_entry->worker_address,a);
    new_entry->worker_port = p;
    new_entry->CountryStatsTree=NULL;
    //Insert the stats in the tree
    new_entry->CountryStatsTree = add_FileStatsTree_node(new_entry->CountryStatsTree,stats);
    new_entry->next = NULL;
    
    ServerHT_Entry *temp = ServerHT[index];
    //Reach the end of the chain
    while (temp->next!=NULL)
    {
        temp=temp->next;
    }
    temp->next = new_entry;

}

//Get an entry from the Hashtable
ServerHT_Entry* ServerHT_get(char *c){
    int index = ServerHashFunction(c);
    ServerHT_Entry *temp = ServerHT[index];
    //Check all the entries of the chain
    while (temp!=NULL)
    {
        if(strcmp(temp->country,c)==0){
            //Entry found
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

//Free the Hashtable memory
void ServerHT_free(){
    for(int i=0;i<ServerHT_size;i++){
        //Free the stats tree
        freeFileStatsTree(ServerHT[i]->CountryStatsTree);
        free(ServerHT[i]);
    }
    free(ServerHT);
}

#endif /* WHOSERVERDATASTRUCTS_H_ */