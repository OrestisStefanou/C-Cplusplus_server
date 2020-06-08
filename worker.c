#include"pipe.h"
#include"worker_functions.h"
#include"request.h"
#include<ctype.h>

char client_fifo[256];  //Client fifo pipe name
char server_fifo[256];  //Server fifo pipe name
queuenode *request_queue = NULL;    //queue of requests from the server
struct WorkersDataStructs myData;   //Here we will have all the data structures we need

void SearchNewFiles(int sig){
    //printf("Caught USR1 signal\n");
    myData.rStats.totalRequests++;
    DirListPtr temp=myData.directories;
    DIR *dr;
    struct dirent *de;//Pointer to directory entry
    dateListptr dateList=NULL;
    char path[100];
    char country_name[25];
    FILE *fp;   //for fopen
    struct patient_record *record;  //To read the records from the files
    char buf[100];  //Buffer to read the records from the file
    struct date_list_node dates;    //Struct to get numeric and string value of Dates(filenames)
    statsListPtr filestats; //Pointer to stats List

    //Open the server pipe to send the stats
    int server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
    if (server_fifo_fd==-1)
    {
        fprintf(stderr,"No server\n");
        exit(EXIT_FAILURE);
    }

    while(temp!=NULL){  //Search in the directories to find new files
        dateList=NULL;
        dr = opendir(temp->CountryDir);
        getCountryFromDir(temp->CountryDir,country_name);
        if(dr==NULL){
            printf("Could not open directory\n");
            return;
        }
        //Read the directory
        while((de=readdir(dr))!=NULL){
            if(de->d_name[0]!='.'){
                memset(path,0,100);
                strcpy(path,temp->CountryDir);
                strcat(path,"/");
                strcat(path,de->d_name);
                //Search if file exists in the tree we keep the files we opened
                if(FileTreesearch(myData.Filenames,path)==0){   //File not found
                    //printf("New file found:%s\n",path);
                    date_list_insert(&dateList,de->d_name);
                }
            }
        }
        while(dateListPop(&dateList,&dates)){
            filestats=NULL; //Initialize the pointer to the filestats list
            memset(path,0,100);
            strcpy(path,temp->CountryDir);
            strcat(path,"/");
            strcat(path,dates.stringDate); //Create the path to open the file
            myData.Filenames=add_filetree_node(myData.Filenames,path);    //Add the path to visited Filenames
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
                    if(RecordTreesearch(myData.InPatients,record)==0){//Record not found in the "ENTER" patients tree
                        printf("Patient not in 'ENTER' records\n");
                        continue;
                    }
                    //If not found insert it in the "EXIT" patients tree
                    myData.OutPatients=add_Recordtree_node(myData.OutPatients,record);
                }else   //Patient has "ENTER" status
                {
                    myData.InPatients=add_Recordtree_node(myData.InPatients,record);//Insert the record in the "ENTER" patients tree
                    DiseaseHashTableInsert(myData.DiseaseHashTable,record,myData.hashtablesize);  //Update the hashtable
                    statsListUpdate(&filestats,record,country_name);    //Update the stats
                }
            }
            fclose(fp);
            //Get the stats and send them to the server
            File_Stats stats_to_send;
            while(statsListPop(&filestats,&stats_to_send)){
                //printf("Sending stats:\n");
                //File_Stats_Print(&stats_to_send);
                write(server_fifo_fd, &stats_to_send, sizeof(stats_to_send));    
            }
        }
        temp=temp->next;
    }
    closedir(dr);
    close(server_fifo_fd);
    myData.rStats.successRequests++;    
}

void terminate(int sig){
    printf("Got the interrupt signal and exiting\n");
    unlink(client_fifo);
    (void) signal(SIGINT, SIG_DFL);
    char logfile[50];   //To create the logfilename
    char country[25];
    sprintf(logfile,"./logfiles/log_file.%d",getpid());
    FILE *fp=fopen(logfile,"w");    //Create and open the file
    //Write the countries in the logfile
    DirListPtr temp=myData.directories;
    while(temp!=NULL){
        getCountryFromDir(temp->CountryDir,country);
        fprintf(fp,"%s\n",country);
        memset(country,0,25);
        temp=temp->next;
    }
    ////////////////////////////////////
    //Write the stats in the logfile
    fprintf(fp,"TOTAL %d\n",myData.rStats.totalRequests);
    fprintf(fp,"SUCCESS %d\n",myData.rStats.successRequests);
    fprintf(fp,"FAIL %d\n",myData.rStats.failedRequests);
    fclose(fp);
    //FREE THE MEMORY
    freeRecordTree(myData.InPatients);
    freeRecordTree(myData.OutPatients);
    freeFilesTree(myData.Filenames);
    freeDiseaseHT(myData.DiseaseHashTable,myData.hashtablesize);
    freeDirList(&myData.directories);
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{   
    (void) signal(SIGINT,terminate);//Handle interrupt signal from parent
    (void) signal(SIGQUIT,terminate);

    //printf("Worker %d running\n",getpid());
    int server_fifo_fd,client_fifo_fd;
    char request[100];
    //INITIALIZE DATA STRUCTURES
    myData.Filenames=NULL;
    myData.InPatients=NULL;
    myData.OutPatients=NULL;
    myData.DiseaseHashTable = create_DiseaseHashtable(15);
    myData.hashtablesize=15;
    myData.directories=NULL;
    myData.rStats.failedRequests=0;
    myData.rStats.successRequests=0;
    myData.rStats.totalRequests=0;
    memset(myData.Server_Info.server_addr,0,100);
    myData.Server_Info.server_port = 0;
    //////////////////////////////////////////////
    sprintf(server_fifo,SERVER_FIFO_NAME,getpid()); //Create server pipe name
    sprintf(client_fifo,CLIENT_FIFO_NAME,getpid());//Create pipe to read from the server
    if (mkfifo(client_fifo,0777)==-1)
    {
        fprintf(stderr,"Error during mkfifo from client\n");
        exit(EXIT_FAILURE);
    }
    (void) signal(SIGUSR1,SearchNewFiles);
    //Main loop
    while(1){
        request_queue=NULL;
        client_fifo_fd = open(client_fifo, O_RDONLY);//Wait for server to open it and to send a request
        if (client_fifo_fd!=-1)
        {   //Read request from the server
            while(read(client_fifo_fd,request,sizeof(request))>0){
                //printf("Server request is %s",request);
                add_item(&request_queue,request);   //Add the request to the queue
                memset(request,0,100);  //Empty the buffer to read next request
            }
            close(client_fifo_fd);
        }
        else    //Error
        {
            printf("Something went wrong with open(client)\n");
            unlink(client_fifo);
            exit(EXIT_FAILURE);
        }

        get_item(&request_queue,request);   //Get request from the queue
        //printf("Request is %s\n",request);
        if(strcmp(request,"Send me the stats\n")==0){
            send_file_stats(server_fifo,request_queue,&myData);//request_queue has the name of the folders to handle
        }

        if(strcmp(request,"Server Info\n")==0){
            get_WhoServerInfo(server_fifo,request_queue,&myData);//request_queue has the info of WhoServer
        }

        if(strcmp(request,"Stats Finished\n")==0){
            //Let master know that we got the message
            server_fifo_fd = open(server_fifo,O_WRONLY);//Open server pipe
            if (server_fifo_fd==-1)
            {
                fprintf(stderr,"No master\n");
                exit(EXIT_FAILURE);
            }
            close(server_fifo_fd);    
            break;
        }

        if(strcmp(request,"/diseaseFrequency")==0){
            myData.rStats.totalRequests++;
            sendDiseaseFrequencyResult(server_fifo,request_queue,&myData);//Request queue has the user's request
        }

        if(strcmp(request,"/searchPatient")==0){
            myData.rStats.totalRequests++;
            sendSearchPatientResult(server_fifo,request_queue,&myData); //Request queue has the user's request
        }

        if(strcmp(request,"/PatientDischarges")==0){
            myData.rStats.totalRequests++;
            sendPatientDischargesResult(server_fifo,request_queue,&myData);////Request queue has the user's request
        }
    }
    printf("Stats finished\n");
    printf("Server address:%s and port:%d\n",myData.Server_Info.server_addr,myData.Server_Info.server_port);
    unlink(client_fifo);
}
