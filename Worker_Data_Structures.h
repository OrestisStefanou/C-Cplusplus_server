#ifndef WORKER_DATA_STRUCTURES_H_
#define WORKER_DATA_STRUCTURES_H_
#include"pipe.h"
#include"patient.h"
#include"request.h"

//BinaryTree to keep PatientRecords
struct RecordTreenode{
    struct patient_record *record;
    struct RecordTreenode *left;
    struct RecordTreenode *right;
};

typedef struct RecordTreenode *RecordTreeptr;
//Insert node sorted by the recordID
RecordTreeptr add_Recordtree_node(RecordTreeptr p,struct patient_record *entry){
    int cond;
    if(p==NULL){//if tree is empty
        p = (struct RecordTreenode *)malloc(sizeof(struct RecordTreenode));//Allocate space for a new node
        p->record=entry;
        p->left=NULL;
        p->right=NULL;
    }
    else if((cond=strcmp(entry->recordID,p->record->recordID))>0){
        p->right=add_Recordtree_node(p->right,entry);
    }
    else if(cond<0){
        p->left=add_Recordtree_node(p->left,entry);
    }
    return p;
}

//Insert node sorted by the date
RecordTreeptr add_RecordDatetree_node(RecordTreeptr p,struct patient_record *entry){
    if(p==NULL){//if tree is empty
        p = (struct RecordTreenode *)malloc(sizeof(struct RecordTreenode));//Allocate space for a new node
        p->record=entry;
        p->left=NULL;
        p->right=NULL;
    }
    else if(check_dates(entry->filedate,p->record->filedate)){
        p->right=add_RecordDatetree_node(p->right,entry);
    }
    else if(check_dates(entry->filedate,p->record->filedate)==0){
        p->left=add_RecordDatetree_node(p->left,entry);
    }
    return p;
}

void RecordTreenode_print(RecordTreeptr p){
    if(p!=NULL){
        print_record_info(p->record);
        RecordTreenode_print(p->left);
        RecordTreenode_print(p->right);
    }
}

//if entry exists in the tree return 1 else return 0
int RecordTreesearch(RecordTreeptr p,struct patient_record *entry){
    int cond;
    if(p==NULL){//tree is empty
        return 0;
    }
    if((cond=strcmp(entry->recordID,p->record->recordID))==0){
        return 1;
    }
    else if(cond>0){
        return RecordTreesearch(p->right,entry);
    }
    else
    {
        return RecordTreesearch(p->left,entry);
    }
}

//Search for record with id='id'.If found fill the data and return 1 else return 0
//If flag==1->Fill the entry date
//If flag==0->Fill the exit date
int RecordTreesearchPatientId(RecordTreeptr p,char *id,struct searchPatientData *data,int flag){
    int cond;
    if(p==NULL){    //Tree is empty
        return 0;
    }
    if((cond=strcmp(id,p->record->recordID))==0){   //Record found
        if(flag==1){
            data->patientEntryDate = p->record->filedate;
        }else
        {
            data->patientExitDate = p->record->filedate;
        }
        strcpy(data->id,p->record->recordID);
        strcpy(data->patientDisease,p->record->disease);
        strcpy(data->patientName,p->record->name);
        strcpy(data->patientLastName,p->record->surname);
        data->patientAge=p->record->age;
        return 1;
    }
    else if(cond>0){
        return RecordTreesearchPatientId(p->right,id,data,flag);
    }
    else
    {
        return RecordTreesearchPatientId(p->left,id,data,flag);
    }
}

//Count the patient's records that are between date1 and date2
int RecordTreeCountWithDates(RecordTreeptr p,Date date1,Date date2){
    int count = 0;
    if(p!=NULL){
        if(check_dates(p->record->filedate,date1)==1 && check_dates(date2,p->record->filedate)==1){
            count++;
        }
        if(check_dates(date1,p->record->filedate)){
            count+=RecordTreeCountWithDates(p->right,date1,date2);
        }
        else
        {
            count+=RecordTreeCountWithDates(p->right,date1,date2);
            count+=RecordTreeCountWithDates(p->left,date1,date2);
        }
        
    }
    return count;
}

//For DiseaseFrequency
int DiseaseFrequencyCount(RecordTreeptr p,Date date1,Date date2,char *c){
    int count = 0;
    if(p!=NULL){
        if(check_dates(p->record->filedate,date1)==1 && check_dates(date2,p->record->filedate)==1 && strcmp(p->record->country,c)==0){
            count++;
        }
        if(check_dates(date1,p->record->filedate)){
            count+=DiseaseFrequencyCount(p->right,date1,date2,c);
        }
        else
        {
            count+=DiseaseFrequencyCount(p->right,date1,date2,c);
            count+=DiseaseFrequencyCount(p->left,date1,date2,c);
        }
        
    }
    return count;
}

//For numPatientDischarges
int PatientDischargesCount(RecordTreeptr p,Date date1,Date date2,char *c,char *d){
    int count = 0;
    if(p!=NULL){
        if(check_dates(p->record->filedate,date1)==1 && check_dates(date2,p->record->filedate)==1 && strcmp(p->record->country,c)==0 && strcmp(p->record->disease,d)==0){
            count++;
        }
        if(check_dates(date1,p->record->filedate)){
            count+=PatientDischargesCount(p->right,date1,date2,c,d);
        }
        else
        {
            count+=PatientDischargesCount(p->right,date1,date2,c,d);
            count+=PatientDischargesCount(p->left,date1,date2,c,d);
        }
        
    }
    return count;    
}

void freeRecordTree(RecordTreeptr p){
    if(p==NULL){
        return;
    }
    freeRecordTree(p->left);
    freeRecordTree(p->right);
    free(p->record);
    free(p);
}
/////////////////////

//BinaryTree that keeps the name of the files the worker opened
struct FileTreenode{
    char *filename;
    struct FileTreenode *left;
    struct FileTreenode *right;
};

typedef struct FileTreenode *FileTreeptr;

//Insert filename in the tree
FileTreeptr add_filetree_node(FileTreeptr p,char *w){
    int cond;
    if(p==NULL){//if tree is empty
        p = (struct FileTreenode *)malloc(sizeof(struct FileTreenode));//Allocate space for a new node
        p->filename=malloc((strlen(w))+1);  //allocate space for the filename
        strcpy(p->filename,w);  //Copy the filename to the treenode
        p->left=NULL;
        p->right=NULL;
    }
    else if((cond=strcmp(w,p->filename))<0){
        p->left = add_filetree_node(p->left,w);
    }
    else if (cond>0){
        p->right=add_filetree_node(p->right,w);
    }
    return p;
}



void FileTreenode_print(FileTreeptr p){
    if(p!=NULL){
        FileTreenode_print(p->left);
        printf("Filename:%s\n",p->filename);
        FileTreenode_print(p->right);
    }
}


int FileTreesearch(FileTreeptr p,char *w){//if entry exists in the tree return 1 else return 0
    int cond;
    if(p==NULL){//tree is empty
        return 0;
    }
    if((cond=strcmp(w,p->filename))==0){
        return 1;
    }
    else if(cond>0){
        return FileTreesearch(p->right,w);
    }
    else
    {
        return FileTreesearch(p->left,w);
    }
}

void freeFilesTree(FileTreeptr p){
    if(p==NULL){
        return;
    }
    freeFilesTree(p->left);
    freeFilesTree(p->right);
    free(p->filename);
    free(p);
}
////////////////////////////////////////////////////////////////

//Hashtable with key the disease name and value a pointer to a recordTree wich is sorted by date
struct DiseaseHashTableEntry
{
    char diseaseID[25];
    RecordTreeptr root;
    struct DiseaseHashTableEntry *next; //In case of cillision
};
typedef struct DiseaseHashTableEntry DiseaseHTentry;

typedef struct DiseaseHashTableEntry **DiseaseHT;    //Hashtable is an array of HTentries pointers

//Create and return a hashtable
DiseaseHT create_DiseaseHashtable(int size){
    DiseaseHT ht = malloc(size *sizeof(DiseaseHTentry *));
    for(int i=0;i<size;i++){
        ht[i]=NULL;
    }
    return ht;
}

//DiseaseHashTable hash function
int DiseaseHashFunction(char *c,int size){
    int i=0;
    int sum=0;
    while (c[i]!='\0')
    {
        sum = sum + c[i];
        i++;
    }
    return sum % size;
}

void DiseaseHashTableInsert(DiseaseHT table,struct patient_record *record,int size){
    int index = DiseaseHashFunction(record->disease,size);   //Get the index to insert
    if(table[index]==NULL){ //No entry with this key so we create one
        table[index] = malloc(sizeof(DiseaseHTentry));
        strcpy(table[index]->diseaseID,record->disease);    //Copy the diseaseID
        table[index]->root=NULL;    //Just to avoid any error
        table[index]->root=add_RecordDatetree_node(table[index]->root,record);  //Insert the record in the tree
        table[index]->next=NULL;
        return;
    }
    //Check if an entry with this key already exists
    DiseaseHTentry *temp = table[index];    //to don't mess up the ptrs
    while(temp!=NULL){
        if(strcmp(temp->diseaseID,record->disease)==0){     //An entry with this key exists
            temp->root=add_RecordDatetree_node(temp->root,record);  //Insert the record in the tree
            return;
        }
        temp=temp->next;    //Go to next entry of the chain
    }
    //If we come here there is no entry with this key so we create one
    temp=table[index];  //Temp points again to the first entry of the chain
    DiseaseHTentry *newentry = malloc(sizeof(DiseaseHTentry));
    strcpy(newentry->diseaseID,record->disease);
    newentry->root=NULL;
    newentry->root = add_RecordDatetree_node(newentry->root,record);
    newentry->next=temp;    //Newentry now is the first entry of the chain
    return;
}

//Get the root of the tree with key disease "c"
RecordTreeptr getDiseaseHTvalue(DiseaseHT table,char *c,int size){
    int index = DiseaseHashFunction(c,size);   //Get the index to insert
    DiseaseHTentry *temp = table[index];
    //Find the entry with key "c"
    while(temp!=NULL){
        if(strcmp(temp->diseaseID,c)==0){   //Key found
            return temp->root;
        }
        temp=temp->next;
    }
    return NULL;
}

void freeDiseaseHT(DiseaseHT table,int size){
    DiseaseHTentry *current,*next_entry;
    for(int i=0;i<size;i++){
        current=table[i];
        while(current!=NULL){
            next_entry=current->next;
            free(current);
            current=next_entry;
        }
    }
}
///////////////////////////////////////////////////////////////////

//List to keep statistics
struct statsListNode{
    File_Stats stats;
    struct statsListNode *next;
};
typedef struct statsListNode *statsListPtr;

//Update statistics
void statsListUpdate(statsListPtr *ptraddr,struct patient_record *record,char *c){
    //Search if a stat structure for that disease already exists
    while(*ptraddr!=NULL){
        if(strcmp((*ptraddr)->stats.Disease,record->disease)==0){
            //Stats for this disease exists so we just updated it
            if(record->age<=20){
                (*ptraddr)->stats.Age_counter[0]++;
            }
            else if(record->age>20 && record->age<=40){
                (*ptraddr)->stats.Age_counter[1]++;
            }
            else if(record->age>40 && record->age<=60){
                (*ptraddr)->stats.Age_counter[2]++;
            }
            else{
                (*ptraddr)->stats.Age_counter[3]++;
            }
            return;
        }
        ptraddr = &((*ptraddr)->next);
    }
    //If we come here there is no stat for this disease so we create one
    *ptraddr=malloc(sizeof(struct statsListNode));
    //Initialize the stats
    strcpy((*ptraddr)->stats.Country,c);
    strcpy((*ptraddr)->stats.Disease,record->disease);
    (*ptraddr)->stats.file_date = record->filedate;
    (*ptraddr)->next=NULL;
    for(int i=0;i<4;i++){
        (*ptraddr)->stats.Age_counter[i]=0;
    }
    ///
    //Update the stats
    if(record->age<=20){
        (*ptraddr)->stats.Age_counter[0]++;
    }
    else if(record->age>20 && record->age<=40){
        (*ptraddr)->stats.Age_counter[1]++;
    }
    else if(record->age>40 && record->age<=60){
        (*ptraddr)->stats.Age_counter[2]++;
    }
    else{
        (*ptraddr)->stats.Age_counter[3]++;
    }
}

//Remove the first stat from the statList
//But first copy it to stats variable
int statsListPop(statsListPtr *ptraddr,File_Stats *stats){
    if((*ptraddr)==NULL){
        return 0;
    }
    statsListPtr temp = *ptraddr;//Save the address of the node
    //Copy the values
    strcpy(stats->Country,(*ptraddr)->stats.Country);
    strcpy(stats->Disease,(*ptraddr)->stats.Disease);
    stats->file_date= (*ptraddr)->stats.file_date;
    for(int i=0;i<4;i++){
        stats->Age_counter[i]=(*ptraddr)->stats.Age_counter[i];
    }
    ///////////////////////////////////////////////////////
    *ptraddr = (*ptraddr)->next;        //get new head of the queue
    free(temp);                 //free the old head of the queue
    return 1;
}

//////////////////////////////////////

//Directory List
struct DirListNode{
    char CountryDir[100];
    struct DirListNode *next;
};
typedef struct DirListNode *DirListPtr;

void DirListInsert(DirListPtr *ptraddr,char *c){
    while(*ptraddr!=NULL){
        ptraddr = &((*ptraddr)->next);
    }
    *ptraddr=malloc(sizeof(struct DirListNode));
    strcpy((*ptraddr)->CountryDir,c);
    (*ptraddr)->next = NULL;
}

void DirListPrint(DirListPtr list){
    while(list!=NULL){
        printf("Directory:%s\n",list->CountryDir);
        list=list->next;
    }
}

void freeDirList(DirListPtr *list){
    DirListPtr current=*list;
    DirListPtr next_node;
    while (current!=NULL)
    {
        next_node=current->next;
        free(current);
        current=next_node;
    }
    
}
///////////////////////////////////
struct ServerInfo{
    char server_addr[100];
    int server_port;
};

//A structure that keeps all the pointers the worker needs to keep
struct WorkersDataStructs{
    RecordTreeptr InPatients;   //Binary tree to keep "ENTER" Patients
    RecordTreeptr OutPatients;  //Binary tree to keep "EXIT" Patients
    FileTreeptr Filenames;  //Binary tree to keep the files we have read
    DiseaseHT DiseaseHashTable; //it will help for the queries
    int hashtablesize;
    DirListPtr directories; // a list to keep the directories the worker is responsible
    struct requestStats rStats; //Keep count of the requests
    struct ServerInfo Server_Info;  //Keep the address and port of WhoServer
};
#endif /* WORKER_DATA_STRUCTURES_H_ */