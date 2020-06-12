#include"pipe.h"
#include"worker_functions.h"
#include<ctype.h>
#include"Parent_Data_Structures.h"
#include"parent_functions.h"
#include"request.h"

int time_to_exit;
pid_t *pids;    //Array with the pids of the workers
int num_of_workers; //Number of workers we created
char CountriesDir[30];    //Country Directory name

void terminate(int sig){
    time_to_exit=1;
    (void) signal(SIGINT, SIG_DFL);
}

void replace_worker(int sig){
    //Find wich worker exited
    //Send the usr2 signal to the workers.The one that fails is the one that died
    printf("A worker died..Trying to replace it...\n");
    int error;
    int old_pid;    //pid of worker that terminated
    int new_pid;    //pid of the worker to replace it
    int ret;    //for error checking
    char *args[]={"./worker",NULL}; //For exec()
    char server_fifo[256];  //To save the server fifo name
    char client_fifo[256];  //To save the client fifo name
    char request[100];
    char dir_name[100];
    int read_res;
    File_Stats stats_data;

    for(int i=0;i<hashtable_size;i++){
        error = kill(Hashtable[i].worker_pid,SIGUSR2);
        if(error==-1){  //We found the worker that terminated
            old_pid = Hashtable[i].worker_pid;
            printf("Worker with pid:%d died\n",old_pid);
            new_pid = fork();
            switch (new_pid)
            {
            case -1:
                perror("fork failed");
                exit(1);
            //This is the child
            case 0:
                ret = execvp(args[0],args);//Start worker process
                if(ret==-1){
                    perror("execvp");
                    exit(1);
                }
            //this is the parent
            default:
                //printf("I created a worker with pid:%d\n",pid);
                //Replace the old pid with the new one
                for(int i=0;i<num_of_workers;i++){
                    if(pids[i]==old_pid){
                        pids[i]=new_pid;
                        break;
                    }
                }
                sprintf(server_fifo,SERVER_FIFO_NAME,new_pid);
                mkfifo(server_fifo,0777);   //Create the pipe that we read from the worker
                break;
            }
            break;
        }
    }
    int client_fifo_fd,server_fifo_fd;

    for(int i=0;i<hashtable_size;i++){
        if(Hashtable[i].worker_pid==old_pid){
            sprintf(client_fifo,CLIENT_FIFO_NAME,new_pid);  //Create worker's pipe name
            client_fifo_fd = open(client_fifo,O_WRONLY);    //Open the worker's pipe to send the directories
            if (client_fifo_fd!=-1)
            {
                strcpy(request,"Send me the stats\n");
                write(client_fifo_fd,request,sizeof(request));//Send the request
                sprintf(dir_name,"%s/%s",CountriesDir,Hashtable[i].country);//Create the directory to send the worker
                write(client_fifo_fd,dir_name,sizeof(dir_name));//Send the directory name
                close(client_fifo_fd);
                sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);
                server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
                while(read_res = read(server_fifo_fd,&stats_data,sizeof(stats_data))>0){//Get the stats from the worker
                    //Ignore them because we already have them
                }
                close(server_fifo_fd);
                Hashtable[i].worker_pid=new_pid;  //Save the worker's pid in the hashtable
            }
        }
    }
    printf("Worker replaced\n");
}

void print_help(){
    printf("All commands:\n");
    printf("/listCountries\n");
    printf("/diseaseFrequency virusName date1 date2 [country]\n");
    printf("/topk-AgeRanges k country disease date1 date2\n");
    printf("/searchPatientRecord recordID\n");
    printf("/numPatientAdmissions disease date1 date2 [country]\n");
    printf("/numPatientDischarges disease date1 date2 [country]\n");
    printf("/exit\n");
}

void print_usage(){
    printf("Usage is:\n./master –w numWorkers -b bufferSize -s serverIP -p serverPort -i input_dir\n");
}

int main(int argc, char const *argv[])
{
    if (argc!=11)
    {
        print_usage();
        exit(EXIT_FAILURE);
    }
    
    if(strcmp(argv[1],"-w")!=0){
        print_usage();
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[3],"-b")!=0){
        print_usage();
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[5],"-s")!=0){
        print_usage();
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[7],"-p")!=0){
        print_usage();
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[9],"-i")!=0){
        print_usage();
        exit(EXIT_FAILURE);
    }

    time_to_exit=0;
    int server_fifo_fd,client_fifo_fd;  //File descriptors of server and client pipes
    File_Stats stats_data;
    int read_res;           //For error checking
    char client_fifo[256];  //To save the client fifo name
    char server_fifo[256];  //To save the server fifo name
    pid_t pid;              //For fork()
    char *args[]={"./worker",NULL}; //For exec()
    int ret;                //For error checking

    num_of_workers=atoi(argv[2]);   //Number of workers
    pids = malloc(num_of_workers * sizeof(pid_t));

    strcpy(CountriesDir,argv[10]);

    int bufferSize=atoi(argv[4]);
    if(bufferSize<4){
        printf("BUffer size must be at lest 4 bytes\n");
        exit(1);
    }

    char WhoServerInfo[100];    //IP address and port of who server
    sprintf(WhoServerInfo,"%s %d",argv[6],atoi(argv[8]));

    struct requestStats stats;
    //Initialize the stats
    stats.failedRequests=0;
    stats.successRequests=0;
    stats.totalRequests=0;

    //Create the directory to save the logfiles
    ret=mkdir("./logfiles",0777);
    if(ret==-1){
        printf("Error during creating logfiles directory\n");
        exit(EXIT_FAILURE);
    }

    //Create workers
    for(int i=0;i<num_of_workers;i++){
        pid = fork();
        switch (pid)
        {
        case -1:
            perror("fork failed");
            exit(1);
        //This is the child
        case 0:
            ret = execvp(args[0],args);//Start worker process
            if(ret==-1){
                perror("execvp");
                exit(1);
            }
        //this is the parent
        default:
            //printf("I created a worker with pid:%d\n",pid);
            pids[i] = pid;
            sprintf(server_fifo,SERVER_FIFO_NAME,pid);
            mkfifo(server_fifo,0777);   //Create the pipe that we read from the worker
            break;
        }
    }
    //"Waiting for workers to create their pipes"
    sleep(2);

    //Get directories of Country folder
    struct dirent *de;  //Pointer to directory entry
    int dir_counter=0;
    queuenode *dir_queue=NULL;
    DIR *dr = opendir(CountriesDir);   //Here it is going to be the folder name given in cmdline arguments
    if(dr ==NULL){
        printf("Could not open the directory\n");
    }
    while((de = readdir(dr))!=NULL){
        if(de->d_name[0]!='.'){
            //add the directory name to a queue and increase the counter of the dirs
            add_item(&dir_queue,de->d_name);
            dir_counter++;
            //to use later at hashtable creation.Insert dirs in the hashtable
            //printf("%s\n",de->d_name);
        }
    }
    closedir(dr);
    //////////////////////////////////////

    //Fill the Hashtable
    Hashtable_init(dir_counter);
    char dir_name[100];
    while(get_item(&dir_queue,dir_name)){   //Get directories from the queue
        Hashtable_insert(dir_name,0);   //Insert directory(country) to the hashtable
    }
    //

    //Send WhoServer info to all the workers
    char request[100];
    for(int i=0;i<num_of_workers;i++){
        sprintf(client_fifo,CLIENT_FIFO_NAME,pids[i]);
        client_fifo_fd = open(client_fifo,O_WRONLY);
        if(client_fifo_fd != -1){
            strcpy(request,"Server Info\n");
            write(client_fifo_fd,request,sizeof(request));  //Send the request
            write(client_fifo_fd,WhoServerInfo,sizeof(WhoServerInfo));  //Send the server info
            close(client_fifo_fd);
            sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);
            server_fifo_fd = open(server_fifo,O_RDONLY);    //Wait until worker gets the message
            close(server_fifo_fd);
        }
    }

    //Send a request to all workers with the directories to handle
    dir_counter=0;
    while(dir_counter<hashtable_size){  //Loop until we send all directories
        for(int i=0;i<num_of_workers;i++){
            if (dir_counter>=hashtable_size)
            {
                break;
            }
            sprintf(client_fifo,CLIENT_FIFO_NAME,pids[i]);
            client_fifo_fd = open(client_fifo,O_WRONLY);
            if (client_fifo_fd!=-1)
            {
                strcpy(request,"Send me the stats\n");
                write(client_fifo_fd,request,sizeof(request));//Send the request
                sprintf(dir_name,"%s/%s",CountriesDir,Hashtable[dir_counter].country);//Create the directory to send the worker
                write(client_fifo_fd,dir_name,sizeof(request));//Send the directory name
                close(client_fifo_fd);
                sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);
                server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
                while(read_res = read(server_fifo_fd,&stats_data,sizeof(stats_data))>0){//Get the stats from the worker
                    Hashtable[dir_counter].StatsTree = add_FileStatsTree_node(Hashtable[dir_counter].StatsTree,stats_data);   //Insert them in the tree
                }
                close(server_fifo_fd);
                Hashtable[dir_counter].worker_pid=pids[i];  //Save the worker's pid in the hashtable
            }
            else    //Something went wrong
            {
                printf("Something went wrong with open(server)\n");
                for(int i=0;i<num_of_workers;i++){
                    sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);
                    unlink(server_fifo);
                    kill(pids[i],SIGINT);
                }
                Hashtable_Free();
                exit(EXIT_SUCCESS);
            }
            dir_counter++;
            memset(request,0,100);
            memset(dir_name,0,100);
        }
        if (dir_counter>=hashtable_size)
        {
            break;
        }
    }

    //Send the workers that stats are finished
    for(int i=0;i<num_of_workers;i++){
        sprintf(client_fifo,CLIENT_FIFO_NAME,pids[i]);
        client_fifo_fd = open(client_fifo,O_WRONLY);
        if(client_fifo_fd != -1){
            strcpy(request,"Stats Finished\n");
            write(client_fifo_fd,request,sizeof(request));  //Send the request
            write(client_fifo_fd,WhoServerInfo,sizeof(WhoServerInfo));  //Send the server info
            close(client_fifo_fd);
            sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);
            server_fifo_fd = open(server_fifo,O_RDONLY);    //Wait until worker gets the message
            close(server_fifo_fd);
        }
    }

    //Get user's input
    char user_request[100];
    int request_code=0; //Each request will have a code
    (void) signal(SIGINT,terminate);//Handle interrupt signal
    (void) signal(SIGQUIT,terminate);//Handle quit signal
    //(void) signal(SIGCHLD,replace_worker);//Handle child signal

    while(1){
        fgets(user_request,100,stdin);  //Get user request
        request_code = get_request_code(user_request);

        if(time_to_exit){   //Got the interrupt siganl
            printf("Caught a signal and exiting\n");
            break;
        }

        if(request_code==1){
            stats.totalRequests++;
            stats.successRequests++;
            for(int i=0;i<hashtable_size;i++){
                printf("Country:%s,pid of worker:%d\n",Hashtable[i].country,Hashtable[i].worker_pid);
            }
            continue;
        }

        if(request_code==2){    //DiseaseFrequency
            stats.totalRequests++;
            struct dfData info;
            int result=0; //To read from the worker
            int sum=0;    //Final result
            int error = fill_dfData(user_request,&info);//Get the info of the request
            if(error==-1){
                stats.failedRequests++;
                printf("Wrong usage(type help for info)\n");
                continue;
            }
            //compare dates
            if(check_dates(info.entry_date,info.exit_date)){
                stats.failedRequests++;
                printf("Wrong dates given\n");
                continue;
            }
            //If country not given send the request info to all the workers
            if(info.country[0]==0){
                for(int i=0;i<num_of_workers;i++){
                    strcpy(request,"/diseaseFrequency");    //Request to send the worker
                    sprintf(client_fifo,CLIENT_FIFO_NAME,pids[i]);  //Create the client pipe name
                    client_fifo_fd = open(client_fifo,O_WRONLY);    //Open worker's pipe to send the request and the info
                    if(client_fifo_fd==-1){
                        stats.failedRequests++;
                        printf("Error during opening client pipe\n");
                        continue;
                    }
                    write(client_fifo_fd,request,sizeof(request));  //Send the request
                    write(client_fifo_fd,user_request,sizeof(user_request));   //Send the user request
                    close(client_fifo_fd);
                    memset(request,0,100);
                    sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);  //Create parent pipe name
                    server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
                    if(server_fifo_fd==-1){
                        stats.failedRequests++;
                        printf("Error during opening server pipe\n");
                        continue;
                    }
                    read(server_fifo_fd,&result,sizeof(int));
                    sum = sum + result;
                    close(server_fifo_fd);
                }
                stats.successRequests++;
                printf("%d\n",sum);
            }else
            {
                int index = getHashtable_index(info.country);   //Get the index of the worker in the hashtable
                strcpy(request,"/diseaseFrequency");    //Request to send the worker
                sprintf(client_fifo,CLIENT_FIFO_NAME,Hashtable[index].worker_pid);  //Create the client pipe name
                client_fifo_fd = open(client_fifo,O_WRONLY);    //Open worker's pipe to send the request and the info
                if(client_fifo_fd==-1){
                    stats.failedRequests++;
                    printf("Error during opening client pipe\n");
                    continue;
                }
                write(client_fifo_fd,request,sizeof(request));  //Send the request
                write(client_fifo_fd,user_request,sizeof(user_request));   //Send the user request
                close(client_fifo_fd);
                memset(request,0,100);
                sprintf(server_fifo,SERVER_FIFO_NAME,Hashtable[index].worker_pid);  //Create parent pipe name
                server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
                if(server_fifo_fd==-1){
                    stats.failedRequests++;
                    printf("Error during opening server pipe\n");
                    continue;
                }
                read(server_fifo_fd,&result,sizeof(int));
                sum = result;
                close(server_fifo_fd);
                stats.successRequests++;
                printf("%d\n",sum);                
            } 
        }

        if(request_code==3){    ///topk-AgeRanges
            stats.totalRequests++;
            int error = topkRanges(user_request);
            if(error==-1){
                stats.failedRequests++;
                printf("Wrong usage(type help for info)\n");
                continue;
            }
            stats.successRequests++;
        }

        if(request_code==4){    //searchPatientRecord
            stats.totalRequests++;
            char rID[10];
            int error = getSearchPatientRecordId(user_request,rID);
            int flag=0;
            if(error==-1){
                stats.failedRequests++;
                printf("Wrong usage(type help for info)\n");
                continue;
            }
            //Send the request to all the workers
            for(int i=0;i<num_of_workers;i++){
                strcpy(request,"/searchPatient");    //Request to send the worker
                sprintf(client_fifo,CLIENT_FIFO_NAME,pids[i]);  //Create the client pipe name
                client_fifo_fd = open(client_fifo,O_WRONLY);    //Open worker's pipe to send the request and the info
                if(client_fifo_fd==-1){
                    stats.failedRequests++;
                    printf("Error during opening client pipe\n");
                    continue;
                }
                write(client_fifo_fd,request,sizeof(request));  //Send the request
                write(client_fifo_fd,user_request,sizeof(user_request));   //Send the user request
                close(client_fifo_fd);
                memset(request,0,100);
                sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);  //Create parent pipe name
                server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
                if(server_fifo_fd==-1){
                    stats.failedRequests++;
                    printf("Error during opening server pipe\n");
                    continue;
                }
                //Read the response
                struct searchPatientData response;
                read(server_fifo_fd,&response,sizeof(response));
                close(server_fifo_fd);
                if(response.patientAge!=-1){    //Record found
                    stats.successRequests++;
                    if(response.patientExitDate.day!=-1){   //There is an exit date
                        printf("%s %s %s %s %d %d-%d-%d %d-%d-%d\n",response.id,response.patientName,response.patientLastName,response.patientDisease,response.patientAge,response.patientEntryDate.day,response.patientEntryDate.month,response.patientEntryDate.year,response.patientExitDate.day,response.patientExitDate.month,response.patientExitDate.year);
                    }
                    else    //There is no exit date
                    {
                        printf("%s %s %s %s %d %d-%d-%d --\n",response.id,response.patientName,response.patientLastName,response.patientDisease,response.patientAge,response.patientEntryDate.day,response.patientEntryDate.month,response.patientEntryDate.year);
                    }
                    flag=1;
                    break;  //No need to ask other workers
                }
            }
            if(flag==0){    //record not found
                stats.successRequests++;
                printf("Record not found\n");
            }
        }

        if(request_code==5){    //numPatientAdmissions
            stats.totalRequests++;
            int error = getPatientAdmissions(user_request);
            if(error==-1){
                stats.failedRequests++;
                printf("Wrong usage(type help for info)\n");
                continue;
            }
            stats.successRequests++;
        }

        if(request_code==6){    //numPatientDischarges
            stats.totalRequests++;
            struct PatientDischargesData data;
            int error = fillPatientDischargesData(user_request,&data);
            char newRequest[200];
            if(error==-1){
                stats.failedRequests++;
                printf("Wrong usage(type help for info)\n");
                continue;
            }
            if(data.countryName[0]==0){ //Country not given
                //Send the request to all the workers
                for(int i=0;i<hashtable_size;i++){
                    user_request[strlen(user_request)-1]=' '; 
                    sprintf(newRequest,"%s%s\n",user_request,Hashtable[i].country);//Add the country to the request
                    //printf("Sending request:%s\n",newRequest);
                    strcpy(request,"/PatientDischarges");    //Request to send the worker
                    sprintf(client_fifo,CLIENT_FIFO_NAME,Hashtable[i].worker_pid);  //Create the client pipe name
                    client_fifo_fd = open(client_fifo,O_WRONLY);    //Open worker's pipe to send the request and the info
                    if(client_fifo_fd==-1){
                        stats.failedRequests++;
                        printf("Error during opening client pipe\n");
                        continue;
                    }
                    write(client_fifo_fd,request,sizeof(request));  //Send the request
                    write(client_fifo_fd,newRequest,sizeof(newRequest));   //Send the user request
                    close(client_fifo_fd);
                    memset(request,0,100);
                    sprintf(server_fifo,SERVER_FIFO_NAME,Hashtable[i].worker_pid);  //Create parent pipe name
                    server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
                    if(server_fifo_fd==-1){
                        stats.failedRequests++;
                        printf("Error during opening server pipe\n");
                        continue;
                    }
                    //Read the response
                    int response;
                    read(server_fifo_fd,&response,sizeof(response));
                    close(server_fifo_fd);
                    printf("%s %d\n",Hashtable[i].country,response);
                    stats.successRequests++;
                }                    
            }else   //Country given
            {
                int index = getHashtable_index(data.countryName);   //Get the worker's index in the HT
                if(index==-1){
                    stats.failedRequests++;
                    printf("There are no data for this country\n");
                    continue;
                }
                strcpy(request,"/PatientDischarges");    //Request to send the worker
                sprintf(client_fifo,CLIENT_FIFO_NAME,Hashtable[index].worker_pid);  //Create the client pipe name
                client_fifo_fd = open(client_fifo,O_WRONLY);    //Open worker's pipe to send the request and the info
                if(client_fifo_fd==-1){
                    stats.failedRequests++;
                    printf("Error during opening client pipe\n");
                    continue;
                }
                write(client_fifo_fd,request,sizeof(request));  //Send the request
                write(client_fifo_fd,user_request,sizeof(user_request));   //Send the user request
                close(client_fifo_fd);
                memset(request,0,100);
                sprintf(server_fifo,SERVER_FIFO_NAME,Hashtable[index].worker_pid);  //Create parent pipe name
                server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
                    if(server_fifo_fd==-1){
                    stats.failedRequests++;
                    printf("Error during opening server pipe\n");
                    continue;
                }
                //Read the response
                int response;
                read(server_fifo_fd,&response,sizeof(response));
                close(server_fifo_fd);
                printf("%s %d\n",Hashtable[index].country,response);
                stats.successRequests++;
            }
        }
        
        if(request_code==8){    //sendSIGUSR1 signal
            stats.totalRequests++;
            char country[25];
            int error=getSiganlCountry(user_request,country);
            if(error==-1){
                stats.failedRequests++;
                printf("Wrong usage(type help for info)\n");
                continue;
            }
            int index=getHashtable_index(country);
            kill(Hashtable[index].worker_pid,SIGUSR1);//Send signal to check the dirs
            sprintf(server_fifo,SERVER_FIFO_NAME,Hashtable[index].worker_pid);
            server_fifo_fd = open(server_fifo,O_RDONLY);    //Open the pipe to read from worker
            while(read_res = read(server_fifo_fd,&stats_data,sizeof(stats_data))>0){//Get the stats from the worker
                //File_Stats_Print(&stats_data);
                Hashtable[index].StatsTree = add_FileStatsTree_node(Hashtable[index].StatsTree,stats_data);   //Insert them in the tree
            }
            close(server_fifo_fd);
            stats.successRequests++;
        }

        if(request_code==7){//exit
            break;
        }

        if(request_code==9){
            print_help();
            continue;
        }

        if(request_code==-1){
            stats.failedRequests++;
            printf("Invalid request(type help for info)\n");
            continue;
        }
    }

    //Close file descriptors and delete the pipes,send interrupt signal to workers
    for(int i=0;i<num_of_workers;i++){
        sprintf(server_fifo,SERVER_FIFO_NAME,pids[i]);
        unlink(server_fifo);
        kill(pids[i],SIGINT);
    }
    //Create and open the log file    
    char logfile[50];   //To create the logfilename
    sprintf(logfile,"./logfiles/log_file.%d",getpid());
    FILE *fp=fopen(logfile,"w");    //Create and open the file
    //Write the counties in the logfile
    for(int i=0;i<hashtable_size;i++){
        fprintf(fp,"%s\n",Hashtable[i].country);
    }
    //Write the stats in the lofile
    fprintf(fp,"TOTAL %d\n",stats.totalRequests);
    fprintf(fp,"SUCCESS %d\n",stats.successRequests);
    fprintf(fp,"FAIL %d\n",stats.failedRequests);
    fclose(fp);

    //Free the memory
    free(pids);
    Hashtable_Free();
    exit(EXIT_SUCCESS);
    return 0;
}