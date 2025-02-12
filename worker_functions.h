#ifndef WORKER_FUNCTIONS_H_
#define WORKER_FUNCTIONS_H_
#include"pipe.h"
#include"Worker_Data_Structures.h"
#include"request.h"
#include"network.h"

//A queue with the requests from the server
struct request_queue_node
{
    char request[100];
    struct request_queue_node *next;
};
typedef struct request_queue_node queuenode;

//Add item to the end of the queue
void add_item(queuenode **qnode,char *r){
    while (*qnode!=NULL)
    {
        qnode = &((*qnode)->next);
    }
    *qnode = (queuenode *)malloc(sizeof(queuenode));
    strcpy((*qnode)->request,r);
    (*qnode)->next = NULL;
}

//Get item from the queue
int get_item(queuenode **qnode,char *r){
    if((*qnode)==NULL){
        return 0;
    }
    queuenode *temp = *qnode;//Save the address of the node
    strcpy(r,(*qnode)->request);   //get the request to return
    *qnode = (*qnode)->next;        //get new head of the queue
    free(temp);                 //free the old head of the queue
    return 1;
}

void print_queue(queuenode *qnode){
    queuenode *temp = qnode;
    while (temp!=NULL)
    {
        printf("Request is %s\n",temp->request);
        temp = temp->next;
    }
    
}

//Get country name from the directory name
void getCountryFromDir(char *dir,char *c){
    int i=0;
    int j=0;
    while(dir[i]!='/'){
        i++;
    }
    i++;
    while(dir[i]!='/'){
        i++;
    }
    i++;
    while(dir[i]!='\0'){
        c[j] = dir[i];
        i++;
        j++;
    }
    c[j] = '\0';
}

//Get WhoServerInfo
void get_WhoServerInfo(char *server_fifo,queuenode *requests,struct WorkersDataStructs *myData){
    char info[100];
    get_item(&requests,info);  //Get the server info
    sscanf(info,"%s %d",myData->Server_Info.server_addr,&myData->Server_Info.server_port);
    //Let the master know that we received the message
    int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
    if (server_fifo_fd==-1)
    {
        fprintf(stderr,"No server\n");
        exit(EXIT_FAILURE);
    }
    close(server_fifo_fd);    
}

//Connect to whoServer to send stats and port
void WhoServerConnect(struct WorkersDataStructs *myData,File_Stats stats,int p){
    //Connect to the server
    int server_port,socket_fd;
    struct hostent *server_host;
    struct sockaddr_in server_address;

    server_port = 27123 ;
    char server_name[100];
    strcpy(server_name,"127.0.0.1");
    server_host=gethostbyname(server_name);
    // Initialise IPv4 server address with server host. 
    memset(&server_address, 0, sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr, server_host->h_length);

    // Create TCP socket.
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Connect to socket with server address.
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1) {
		perror("connect");
        exit(1);
	}
    char msg[300];
    strcpy(msg,"Stats\n");
    write(socket_fd,msg,strlen("Stats\n"));
    //File_Stats_Print(&stats);
    //Create the message to send
    sprintf(msg,"%d-%d-%d %s %s %d %d %d %d %d",stats.file_date.day,stats.file_date.month,stats.file_date.year,
    stats.Country,stats.Disease,stats.Age_counter[0],stats.Age_counter[1],stats.Age_counter[2],
    stats.Age_counter[3],p);
    int msg_len = strlen(msg);
    //printf("Message len is %d\n",msg_len);
    //First send the length of the message
    int res = write(socket_fd,&msg_len,sizeof(msg_len));    //Send port number and stats
    if(res<sizeof(msg_len)){
        printf("Something went wrong when sending size\n");
    }
    //Send the message
    res = write(socket_fd,msg,msg_len+1);
    if(res<msg_len+1){
        printf("Something went wrong when sending the message\n");
    }
    close(socket_fd);
}

//Send file statistics to the server
void send_file_stats(char *server_fifo,queuenode *requests,struct WorkersDataStructs *myData,int p){
    File_Stats stats_data;
    char directory[100];
    char path[100];
    char country_name[25];
    get_item(&requests,directory);  //Get the directory to handle
    //printf("Directory to handle is %s\n",directory);
    DirListInsert(&myData->directories,directory);  //Insert the directory in the worker's Dir list
    getCountryFromDir(directory,country_name);
    //printf("Country name is %s\n",country_name);
    //Read the directory and insert the files(dates) in the dateslist to sort them
    struct dirent *de;//Pointer to directory entry
    dateListptr dateList=NULL;
    DIR *dr = opendir(directory);
    if(dr==NULL){   //In case of error update the parent and abort
        printf("Could not open the directory\n");
        int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
        if (server_fifo_fd==-1)
        {
            fprintf(stderr,"No server\n");
            exit(EXIT_FAILURE);
        }
        printf("Error during opening directory\n");
        //Send the stats(empty stats)
        write(server_fifo_fd, &stats_data, sizeof(stats_data));
        close(server_fifo_fd);
        return;
    }
    //READING THE DIRECTORY AND SORTING THE DATES
    while((de=readdir(dr))!=NULL){
        if(de->d_name[0]!='.'){
            date_list_insert(&dateList,de->d_name);     //Insert the filename(date) in the list
        }
    }
    //Open the server pipe to send the stats
    int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
    if (server_fifo_fd==-1)
    {
        fprintf(stderr,"No server\n");
        exit(EXIT_FAILURE);
    }
    //OPEN THE FILES OF THE DIRECTORY FILL THE DATA STRUCTURES AND CREATE THE STATS TO SEND
    FILE *fp;   //for fopen
    struct patient_record *record;  //To read the records from the files
    char buf[100];  //Buffer to read the records from the file
    struct date_list_node dates;    //Struct to get numeric and string value of Dates(filenames)
    statsListPtr filestats; //Pointer to stats List
    while(dateListPop(&dateList,&dates)){
        //filestats=NULL; //Initialize the pointer to the filestats list
        memset(path,0,100);
        strcpy(path,directory);
        strcat(path,"/");
        strcat(path,dates.stringDate); //Create the path to open the file
        myData->Filenames=add_filetree_node(myData->Filenames,path);    //Add the path to visited Filenames
        //printf("File to open is %s\n",path);
        fp = fopen(path,"r");
        if(fp==NULL){
            printf("Error during opening file\n");
            continue;
        }
        while(fgets(buf,100,fp)!=NULL){ //Read the file line by line
            record=malloc(sizeof(struct patient_record));
            read_record(record,buf);    //Fill the patient record structure
            record->filedate=dates.numericDate; //Patient Record date is the name of the file
            strcpy(record->country,country_name);   //Patient Country is the name of the folder
            if(strcmp(record->status,"EXIT")==0){
                //Check for error
                if(RecordTreesearch(myData->InPatients,record)==0){//Record not found in the "ENTER" patients tree
                    printf("ERROR\n");
                    continue;
                }
                //If not found insert it in the "EXIT" patients tree
                myData->OutPatients=add_Recordtree_node(myData->OutPatients,record);
            }else   //Patient has "ENTER" status
            {
                myData->InPatients=add_Recordtree_node(myData->InPatients,record);//Insert the record in the "ENTER" patients tree
                DiseaseHashTableInsert(myData->DiseaseHashTable,record,myData->hashtablesize);  //Update the hashtable
                statsListUpdate(&filestats,record,country_name);    //Update the stats
            }
        }
        fclose(fp);
        //Get the stats and send them to the server
        File_Stats stats_to_send;
        while(statsListPop(&filestats,&stats_to_send)){
            write(server_fifo_fd, &stats_to_send, sizeof(stats_to_send));
            WhoServerConnect(myData,stats_to_send,p);
        }
    }
    closedir(dr);
    close(server_fifo_fd);
    return;
}

//Send results to the master
void sendDiseaseFrequencyResult(char *server_fifo,queuenode *requests,struct WorkersDataStructs *myData){
    struct dfData info;
    int result=0;
    char request[100];
    get_item(&requests,request);
    fill_dfData(request,&info);
    RecordTreeptr root = getDiseaseHTvalue(myData->DiseaseHashTable,info.virusName,myData->hashtablesize);  //Get the root of the tree from the diseaseHT
    if(root==NULL){ //No data for this disease
        myData->rStats.failedRequests++;
        int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
        if(server_fifo_fd==-1){
            myData->rStats.failedRequests++;
            return;
        }
        //Send the result to the server
        write(server_fifo_fd,&result,sizeof(result));
        close(server_fifo_fd);
        return;
    }
    
    if(info.country[0]==0){//Country not given
        result = RecordTreeCountWithDates(root,info.entry_date,info.exit_date); //Count the nodes that are between the dates given
    }
    else    //Country given
    {
        result = DiseaseFrequencyCount(root,info.entry_date,info.exit_date,info.country);
    }
    myData->rStats.successRequests++;
    int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
    if(server_fifo_fd==-1){
        myData->rStats.failedRequests++;
        return;
    }
    //Send the result to the server
    write(server_fifo_fd,&result,sizeof(result));
    close(server_fifo_fd);   
}

//Send df results to WhoServer
void send_df_results(int fd,struct dfData info,struct WorkersDataStructs *myData){
    int result = 0,nbytes;
    //printf("Country:%s\n",info.country);
    //printf("Disease:%s\n",info.virusName);
    //print_date(&info.entry_date);
    //print_date(&info.exit_date);
    RecordTreeptr root = getDiseaseHTvalue(myData->DiseaseHashTable,info.virusName,myData->hashtablesize);  //Get the root of the tree from the diseaseHT
   
    if(root==NULL){
        //No data for this disease
        result = -1;
        nbytes = write(fd,&result,sizeof(int));  //Send -1 to the server
        if(nbytes<sizeof(int)){
            printf("Something went wrong\n");
            return;
        }
    }

    if(info.country[0]=='0'){//Country not given
        //printf("Entering here\n");
        result = RecordTreeCountWithDates(root,info.entry_date,info.exit_date); //Count the nodes that are between the dates given
    }
    else    //Country given
    {
        result = DiseaseFrequencyCount(root,info.entry_date,info.exit_date,info.country);
    }

    //Send the result to the server
    //printf("Result is %d\n",result);
    nbytes = write(fd,&result,sizeof(result));
    if(nbytes<sizeof(int)){
        printf("Something went wrong\n");
    }
}

//Send results to master
void sendSearchPatientResult(char *server_fifo,queuenode *requests,struct WorkersDataStructs *myData){
    struct searchPatientData data_to_send;
    data_to_send.patientAge=-1; //To know if the record found
    data_to_send.patientExitDate.day=-1;    //To know if patient exited 
    char request[100];
    get_item(&requests,request);
    char record_id[10];
    getSearchPatientRecordId(request,record_id);
    //Search for this id in the trees
    //First search in the "IN" Tree
    RecordTreesearchPatientId(myData->InPatients,record_id,&data_to_send,1);
    if(data_to_send.patientAge==-1){    //Record not found
        myData->rStats.successRequests++;
        int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
        if(server_fifo_fd==-1){
            myData->rStats.failedRequests++;
            return;
        }   
        //Send the result to the server
        write(server_fifo_fd,&data_to_send,sizeof(data_to_send));
        close(server_fifo_fd);
        return;   
    }
    //Check if Patient Exited
    RecordTreesearchPatientId(myData->OutPatients,record_id,&data_to_send,0);
    //printf("Exit date is:");
    //print_date(&data_to_send.patientExitDate);
    myData->rStats.successRequests++;
    int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
    if(server_fifo_fd==-1){
        myData->rStats.failedRequests++;
        return;
    }
    //Send the result to the server
    write(server_fifo_fd,&data_to_send,sizeof(data_to_send));
    close(server_fifo_fd);
}

//Send results to WhoServer
void SearchPatientResponse(int fd,char *record_id,struct WorkersDataStructs *myData){
    struct searchPatientData data_to_send;
    data_to_send.patientAge=-1; //To know if the record found 
    strcpy(data_to_send.id,"000");
    strcpy(data_to_send.patientName,"000");
    strcpy(data_to_send.patientLastName,"000");
    strcpy(data_to_send.patientDisease,"000");
    set_date(&data_to_send.patientEntryDate,0,0,0);
    set_date(&data_to_send.patientExitDate,-1,0,0); //To know if patient exited

    int nbytes;
    char msg[300];
    memset(msg,0,300);
    //Search for the id in the trees
    RecordTreesearchPatientId(myData->InPatients,record_id,&data_to_send,1);

    if(data_to_send.patientAge==-1){    //Record not found
        //Send the result to the server
        //Create response message
        sprintf(msg,"%s %s %s %s %d %d-%d-%d %d-%d-%d\n",data_to_send.id,data_to_send.patientName,
        data_to_send.patientLastName,data_to_send.patientDisease,data_to_send.patientAge,
        data_to_send.patientEntryDate.day,data_to_send.patientEntryDate.month,data_to_send.patientEntryDate.year
        ,data_to_send.patientExitDate.day,data_to_send.patientExitDate.month,data_to_send.patientExitDate.year);
        
        nbytes = write(fd,msg,strlen(msg));
        if(nbytes<strlen(msg)){
            printf("Something went wrong during sending the response\n");
        }
        return;   
    }

    //Check if Patient Exited
    RecordTreesearchPatientId(myData->OutPatients,record_id,&data_to_send,0);
    
    //Send the result to the server
    //Create response message
    sprintf(msg,"%s %s %s %s %d %d-%d-%d %d-%d-%d\n",data_to_send.id,data_to_send.patientName,
    data_to_send.patientLastName,data_to_send.patientDisease,data_to_send.patientAge,
    data_to_send.patientEntryDate.day,data_to_send.patientEntryDate.month,data_to_send.patientEntryDate.year
    ,data_to_send.patientExitDate.day,data_to_send.patientExitDate.month,data_to_send.patientExitDate.year);
    
    nbytes = write(fd,msg,strlen(msg));
    if(nbytes<strlen(msg)){
        printf("Something went wrong during sending the response\n");
    }
}

//Send to master
void sendPatientDischargesResult(char *server_fifo,queuenode *requests,struct WorkersDataStructs *myData){
    struct PatientDischargesData data_to_read;
    char request[100];
    get_item(&requests,request);
    fillPatientDischargesData(request,&data_to_read);
    //printf("Disease:%s\n",data_to_read.virusName);
    //printf("COuntry:%s\n",data_to_read.countryName);
    //p/rintf("Enter date:");
    //print_date(&data_to_read.entry_date);
    //printf("Exit date:");
    //print_date(&data_to_read.exit_date);
    
    //Count the patients
    int result=PatientDischargesCount(myData->OutPatients,data_to_read.entry_date,data_to_read.exit_date,data_to_read.countryName,data_to_read.virusName);
    myData->rStats.successRequests++;
    int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
    if(server_fifo_fd==-1){
        myData->rStats.failedRequests++;
        return;
    }
    //Send the result to the server
    write(server_fifo_fd,&result,sizeof(result));
    close(server_fifo_fd);
}

//Send result to Server
void NumPatientDischarges(int fd,struct PatientDischargesData data_to_read,struct WorkersDataStructs *myData){
    //Count the patients
    int result=PatientDischargesCount(myData->OutPatients,data_to_read.entry_date,data_to_read.exit_date,data_to_read.countryName,data_to_read.virusName);
    char response[100];
    //Create response message
    sprintf(response,"%s:%d\n",data_to_read.countryName,result);
    //Send the result to the server
    int nbytes = write(fd,response,strlen(response));
    if(nbytes<strlen(response)){
        printf("Something went wrong during sending the response\n");
    }    
}
#endif /* WORKER_FUNCTIONS_H_ */