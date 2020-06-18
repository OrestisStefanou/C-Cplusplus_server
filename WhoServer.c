#include"network.h"
#include"buffer.h"
#include"pipe.h"
#include"WhoServer_functions.h"

#define BACKLOG 10
#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

//Thread function to handle connections from client
void *serve_client(void *arg);

//Siganl handler for SIGTERM AND SIGINT signals
void signal_handler(int signal_number);

int number_of_threads;  //Get this from command line arg

//Network variables
int query_socket,stats_socket;

//Poll set 
struct pollfd *pfds;

int main(int argc, char const *argv[])
{   
    //Initialize Hashtable
    ServerHT_init(15);

    //Network variables
    int query_port,new_socket_fd,err;
    struct sockaddr_in address;
    struct sockaddr_storage client_address;
    socklen_t client_address_len;
    char remoteIP[INET6_ADDRSTRLEN];

    //Poll set varibles
    int fd_count = 0;
    int fd_size = 2;
    pfds = malloc(sizeof *pfds * fd_size); //Create set to monitor the 2 listening sockets    

    //Check command line arguments
    if (argc != 9)
    {
        printf("Usage is:./whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize\n");
        exit(EXIT_FAILURE);
    }

    number_of_threads = atoi(argv[6]);

    pthreads = malloc(number_of_threads * sizeof(pthread_t));   //Create the array of thread ids
    
    //Create file descriptor buffer
    buffer_size = atoi(argv[8]);
    buffer_init();  //Initialize the buffer

    //Initialize condition variable
    pthread_cond_init(&cvar,NULL);
    pthread_cond_init(&cvar2,NULL);         

    query_port = atoi(argv[2]);   //Port number to listen
    int stats_port = atoi(argv[4]);

    //Initialize IPv4 address
    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(query_port);
    address.sin_addr.s_addr = INADDR_ANY;

    //Create the threads
    for(int i=0;i<number_of_threads;i++){
        if(err=pthread_create(&pthreads[i],NULL,serve_client,NULL)){
            perror2("pthread_create",err);
            exit(EXIT_FAILURE);
        }
    }

    //Create the query listening socket
    if((query_socket = socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    //Bind address to socket
    if(bind(query_socket,(struct sockaddr *)&address,sizeof(address))==-1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    //Listen on socket
    if(listen(query_socket,BACKLOG)==-1){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //Create stats listening socket
    //Initialize IPv4 address
    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(stats_port);   //CHANGE THIS TO THE PORT GIVEN FROM CMD LINE
    address.sin_addr.s_addr = INADDR_ANY;

    //Create the stats listening socket
    if((stats_socket = socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    //Bind address to socket
    if(bind(stats_socket,(struct sockaddr *)&address,sizeof(address))==-1){
        perror("bind");
        exit(EXIT_FAILURE);
    }
    //Listen on socket
    if(listen(stats_socket,BACKLOG)==-1){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //Add the two listening sockets to set
    pfds[0].fd = query_socket;
    pfds[0].events = POLLIN;    //Report ready to read on incoming connection
    pfds[1].fd = stats_socket;
    pfds[1].events = POLLIN;
    fd_count=2; //For the 2 listening sockets

    /* Assign signal handlers to signals. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    printf("Waiting for connections\n");
    while (1)
    {
        int poll_count = poll(pfds,fd_count,-1);

        if (poll_count == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        //Run through the existing connections looking for data to read
        for(int i=0;i<fd_count;i++){
            //Check if someone is ready to read
            if(pfds[i].revents & POLLIN){
                //Handle new connection
                //Accept connection from client
                client_address_len = sizeof(struct sockaddr_in);
                new_socket_fd = accept(pfds[i].fd,(struct sockaddr *)&client_address,&client_address_len);
                if(new_socket_fd==-1){
                    perror("accept");
                    continue;
                }
                //Convert IP address from binary to text form
                inet_ntop(client_address.ss_family,get_in_addr((struct sockaddr*)&client_address),remoteIP, INET6_ADDRSTRLEN);
                //printf("New connection from:%s\n",remoteIP);
                //Lock mutex
                if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                    perror2("pthread mutex lock",err);
                    exit(EXIT_FAILURE);
                }
                while (buffer_is_full())    //If buffer is full wait
                {
                    pthread_cond_wait(&cvar2,&mtx);  //wait for signal
                }
                //Entering new socket from connection in the buffer
                buffer_insert(new_socket_fd,remoteIP);
                pthread_cond_signal(&cvar); //Wake one thread to handle the connection
                //Unlock the mutex
                if (err=pthread_mutex_unlock(&mtx))
                {
                    perror2("pthread mutex unlock\n",err);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    
    return 0;
}


void *serve_client(void *arg){
    int err,res,i;
    BufferEntry clientInfo;
    printf("Thread started\n");
    while (1)
    {
        if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
            perror2("pthread mutex lock",err);
            exit(EXIT_FAILURE);
        }
        //printf("Serve client thread locked the mutex\n");
        while (buffer_is_empty())
        {
            pthread_cond_wait(&cvar,&mtx);  //If buffer is empty wait for signal
        }
        //printf("Thread handling the connection\n");
        //Get the info of the client to serve
        clientInfo = buffer_get();
        //Unlock the mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }
        char buffer[100];   //Buffer to get info of the incoming request
        memset(buffer,0,100);
        i=0;
        //Read the request
        int nbytes=0;
        while(1){
            nbytes = read(clientInfo.fd,&buffer[i],1);
            if(nbytes==0){
                continue;
            }
            if(buffer[i]=='\n')
                break;
            i++;
        }
        buffer[++i] = '\0';

        if(strcmp(buffer,"Stats\n")==0){    //Get the stats and port from the workers
            File_Stats stats;
            int worker_port;
            int msg_len;
            res = read(clientInfo.fd,&msg_len,sizeof(int)); //Get the length of the message
            if(res<sizeof(int)){
                printf("Something went wrong\n");
            }
            char msg[300];
            memset(msg,0,300);
            int bytes_read=0;
            //printf("Message len is %d\n",msg_len);
            //Read until we get the whole message
            while (bytes_read<msg_len)
            {
                res = read(clientInfo.fd,&msg[bytes_read],msg_len-bytes_read);
                bytes_read+=res; 
            }
            sscanf(msg,"%d-%d-%d %s %s %d %d %d %d %d",&stats.file_date.day,&stats.file_date.month,&stats.file_date.year,
            stats.Country,stats.Disease,&stats.Age_counter[0],&stats.Age_counter[1],&stats.Age_counter[2],
            &stats.Age_counter[3],&worker_port);

            if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                perror2("pthread mutex lock",err);
                exit(EXIT_FAILURE);
            }
            //Insert the stats in the Hashtable
            ServerHT_insert(clientInfo.address,worker_port,stats);
            //Unlock the mutex
            if (err=pthread_mutex_unlock(&mtx))
            {
                perror2("pthread mutex unlock\n",err);
                exit(EXIT_FAILURE);
            }
        }        
        else
        {
            //printf("Got message %s\n",buffer);
            int request_code = get_request_code(buffer);
            if(request_code==3){    //Topk-AgeRanges
                int error = topkRanges(buffer,clientInfo.fd);
                if(error==-1){
                    printf("Wrong usage\n");
                }
            }

            if(request_code == 2){//DiseaseFrequency
                struct dfData info;
                int result=0,sum=0,nbytes; //result from the worker,sum->final result
                int error = fill_dfData(buffer,&info);
                if(error==-1){
                    write(clientInfo.fd,"Wrong Usage\n",13);    //Send error message
                    printf("Wrong usage\n");
                    close(clientInfo.fd);
                    memset(clientInfo.address,0,100);
                    continue;
                }

                //Check the dates
                if(check_dates(info.entry_date,info.exit_date)){
                    write(clientInfo.fd,"Wrong Usage\n",13);    //Send error message
                    printf("Wrong usage\n");
                    close(clientInfo.fd);
                    memset(clientInfo.address,0,100);
                    continue;                    
                }

                //if country not given send the request to all the workers
                if(info.country[0]==0){
                    strcpy(info.country,"000000000");   //So the worker knows country is not given
                    //Sent the request and sum the results
                    Addr_List_Node *temp = addr_list_head;
                    //print_addr_list();
                    while(temp!=NULL){
                        //Send the request to the worker
                        //Create the message with the information needed
                        char msg[300];
                        sprintf(msg,"%s %s %d-%d-%d %d-%d-%d %s %s\n","df",info.virusName,info.entry_date.day,
                        info.entry_date.month,info.entry_date.year,info.exit_date.day,info.exit_date.month,
                        info.exit_date.year,info.country,"000");
                        //Send the information to the worker
                        int sock_fd = send_message(temp->addr,temp->port,msg,1);
                        if(sock_fd == -1){
                            printf("Something went wrong during sending the message\n");
                        }
                        //Read the response from the worker
                        nbytes = read(sock_fd,&result,sizeof(int));
                        if(nbytes<4){
                            printf("Something went wrong\n");
                        }else{
                            sum+=result;
                        }
                        close(sock_fd);
                        temp = temp->next;                        
                    }
                    //Lock the mutex
                    if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                        perror2("pthread mutex lock",err);
                        exit(EXIT_FAILURE);
                    }
                    printf("%d\n",sum);
                    //Unlock the mutex
                    if (err=pthread_mutex_unlock(&mtx))
                    {
                        perror2("pthread mutex unlock\n",err);
                        exit(EXIT_FAILURE);
                    }
                    char response_message[50];
                    sprintf(response_message,"%d\n",sum);
                    write(clientInfo.fd,response_message,strlen(response_message));
                }else
                {
                    //Country given.Send the request to the responsible worker
                    ServerHT_Entry *ht_entry = ServerHT_get(info.country);
                    //Send the request to the worker
                    //Create the message with the information needed
                    char msg[300];
                    sprintf(msg,"%s %s %d-%d-%d %d-%d-%d %s %s\n","df",info.virusName,info.entry_date.day,
                    info.entry_date.month,info.entry_date.year,info.exit_date.day,info.exit_date.month,
                    info.exit_date.year,info.country,"000");
                    //Send the information to the worker
                    int sock_fd = send_message(ht_entry->worker_address,ht_entry->worker_port,msg,1);
                    if(sock_fd == -1){
                        printf("Something went wrong during sending the message\n");
                    }
                    //Read the response from the worker
                    nbytes = read(sock_fd,&result,sizeof(int));
                    if(nbytes<4){
                        printf("Something went wrong\n");
                    }else{
                        //Lock the mutex
                        if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                            perror2("pthread mutex lock",err);
                            exit(EXIT_FAILURE);
                        }
                        printf("%d\n",result);
                        //Unlock the mutex
                        if (err=pthread_mutex_unlock(&mtx))
                        {
                            perror2("pthread mutex unlock\n",err);
                            exit(EXIT_FAILURE);
                        }
                        char response_message[50];
                        sprintf(response_message,"%d\n",result);
                        write(clientInfo.fd,&response_message,strlen(response_message));
                    }
                    close(sock_fd);
                }
                
            }

            if (request_code==4)    //searchPatientRecord
            {
                char rID[10];
                int error = getSearchPatientRecordId(buffer,rID);
                int flag=0,nbytes;
                if(error==-1){
                    write(clientInfo.fd,"Wrong Usage\n",13);    //Send error message
                    printf("Wrong usage\n");
                    close(clientInfo.fd);
                    memset(clientInfo.address,0,100);
                    continue;
                }
                //Sent the request to all the workers
                Addr_List_Node *temp = addr_list_head;
                while(temp!=NULL){
                    //Send the request to the worker
                    //Create the message with the information needed
                    char msg[300];
                    sprintf(msg,"%s %s %d-%d-%d %d-%d-%d %s %s\n","spr","0",0,0,0,0,0,0,"0",rID);
                    //Send the information to the worker
                    int sock_fd = send_message(temp->addr,temp->port,msg,1);
                    if(sock_fd == -1){
                        printf("Something went wrong during sending the message\n");
                    }
                    //Read the response from the worker
                    struct searchPatientData response_info;
                    memset(msg,0,300);
                    read_request(sock_fd,msg);  //Read reponse
                    //Get the information from the response
                    sscanf(msg,"%s %s %s %s %d %d-%d-%d %d-%d-%d\n",response_info.id,response_info.patientName,
                    response_info.patientLastName,response_info.patientDisease,&response_info.patientAge,
                    &response_info.patientEntryDate.day,&response_info.patientEntryDate.month,&response_info.patientEntryDate.year
                    ,&response_info.patientExitDate.day,&response_info.patientExitDate.month,&response_info.patientExitDate.year);
                    
                    if(response_info.patientAge!=-1){
                        //Record found
                        if(response_info.patientExitDate.day!=-1){   //There is an exit date
                            //Lock the mutex
                            printf("%s %s %s %s %d %d-%d-%d %d-%d-%d\n",response_info.id,response_info.patientName,response_info.patientLastName,response_info.patientDisease,response_info.patientAge,response_info.patientEntryDate.day,response_info.patientEntryDate.month,response_info.patientEntryDate.year,response_info.patientExitDate.day,response_info.patientExitDate.month,response_info.patientExitDate.year);
                            //Unlock the mutex
                            char response_msg[300];
                            //Create response to send the client
                            sprintf(response_msg,"%s %s %s %s %d %d-%d-%d %d-%d-%d\n",response_info.id,response_info.patientName,response_info.patientLastName,response_info.patientDisease,response_info.patientAge,response_info.patientEntryDate.day,response_info.patientEntryDate.month,response_info.patientEntryDate.year,response_info.patientExitDate.day,response_info.patientExitDate.month,response_info.patientExitDate.year);
                            write(clientInfo.fd,response_msg,strlen(response_msg));
                        }
                        else    //There is no exit date
                        {
                            //Lock the mutex
                            if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                                perror2("pthread mutex lock",err);
                                exit(EXIT_FAILURE);
                            }
                            printf("%s %s %s %s %d %d-%d-%d --\n",response_info.id,response_info.patientName,response_info.patientLastName,response_info.patientDisease,response_info.patientAge,response_info.patientEntryDate.day,response_info.patientEntryDate.month,response_info.patientEntryDate.year);
                            //Unlock the mutex
                            if (err=pthread_mutex_unlock(&mtx))
                            {
                                perror2("pthread mutex unlock\n",err);
                                exit(EXIT_FAILURE);
                            }
                            char response_msg[300];
                            //Create response message to send the client
                            sprintf(response_msg,"%s %s %s %s %d %d-%d-%d --\n",response_info.id,response_info.patientName,response_info.patientLastName,response_info.patientDisease,response_info.patientAge,response_info.patientEntryDate.day,response_info.patientEntryDate.month,response_info.patientEntryDate.year);
                            write(clientInfo.fd,response_msg,strlen(response_msg));
                        }
                        flag=1;
                        close(sock_fd);
                        break;
                    }
                    close(sock_fd);
                    temp = temp->next;                        
                }
                if(flag==0){
                    //Lock the mutex
                    if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                        perror2("pthread mutex lock",err);
                        exit(EXIT_FAILURE);
                    }
                    printf("Record not found\n");
                    //Unlock the mutex
                    if (err=pthread_mutex_unlock(&mtx))
                    {
                        perror2("pthread mutex unlock\n",err);
                        exit(EXIT_FAILURE);
                    }
                    //Send response message to the client
                    write(clientInfo.fd,"Record not found\n",strlen("Record not found\n"));
                }
            }

            if (request_code==5)    //NumPatientAdmissions
            {
                    int error = numtPatientAdmissions(buffer,clientInfo.fd);
                    if (error==-1)
                    {
                        printf("Error during executing the command(Wrong Usage or no Data\n");
                        write(clientInfo.fd,"Error\n",strlen("Error\n"));
                    }
            }
            
            if(request_code==6){    //NumPatientDischarges
                char response[100];
                struct PatientDischargesData info;
                int nbytes;
                int error = fillPatientDischargesData(buffer,&info);

                if(error==-1){  //Wrong usage
                    printf("Wrong Usage\n");
                    write(clientInfo.fd,"Wrong Usage\n",strlen("Wrong usage\n"));   //Send client error message
                    close(clientInfo.fd);
                    memset(clientInfo.address,0,100);
                    continue;
                }

                //Check the dates
                if(check_dates(info.entry_date,info.exit_date)){
                    write(clientInfo.fd,"Wrong Usage\n",strlen("Wrong usage\n"));    //Send client error message
                    printf("Wrong usage\n");
                    close(clientInfo.fd);
                    memset(clientInfo.address,0,100);
                    continue;                    
                }

                //If Country not given send the request to all the workers
                if(info.countryName[0]==0){
                    //Sent the request and sum the results
                    ServerHT_Entry *ht_entry;
                    //Lock the mutex
                    if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                        perror2("pthread mutex lock",err);
                        exit(EXIT_FAILURE);
                    }
                    for(int i=0;i<ServerHT_size;i++){
                        ht_entry = ServerHT[i];
                        while(ht_entry!=NULL){
                            //Send the request to the worker
                            //Create the message with the information needed
                            char msg[300];
                            strcpy(info.countryName,ht_entry->country);
                            sprintf(msg,"%s %s %d-%d-%d %d-%d-%d %s %s\n","npd",info.virusName,info.entry_date.day,
                            info.entry_date.month,info.entry_date.year,info.exit_date.day,info.exit_date.month,
                            info.exit_date.year,info.countryName,"000");
                            //Send the information to the worker
                            int sock_fd = send_message(ht_entry->worker_address,ht_entry->worker_port,msg,1);
                            if(sock_fd == -1){
                                printf("Something went wrong during sending the message\n");
                            }
                            //Read the response from the worker
                            memset(response,0,100);
                            error = read_request(sock_fd,response);
                            if(error==-1){
                                printf("Something went wrong\n");
                            }else{
                                printf("%s",response);  //Print the result
                                nbytes = write(clientInfo.fd,response,strlen(response)); //Send the result to the client
                                if(nbytes<strlen(response)){
                                    printf("Something went wrong during sending the response\n");
                                }
                            }
                            close(sock_fd);
                            ht_entry = ht_entry->next;
                        }                        
                    }
                    //Unlock the mutex
                    if (err=pthread_mutex_unlock(&mtx))
                    {
                        perror2("pthread mutex unlock\n",err);
                        exit(EXIT_FAILURE);
                    }
                }else
                {
                    //Country given.Send the request to the responsible worker
                    ServerHT_Entry *ht_entry = ServerHT_get(info.countryName);
                    //Send the request to the worker
                    //Create the message with the information needed
                    char msg[300];
                    sprintf(msg,"%s %s %d-%d-%d %d-%d-%d %s %s\n","npd",info.virusName,info.entry_date.day,
                    info.entry_date.month,info.entry_date.year,info.exit_date.day,info.exit_date.month,
                    info.exit_date.year,info.countryName,"000");
                    //Send the information to the worker
                    int sock_fd = send_message(ht_entry->worker_address,ht_entry->worker_port,msg,1);
                    if(sock_fd == -1){
                        printf("Something went wrong during sending the message\n");
                    }
                    //Read the response from the worker
                    memset(response,0,100);
                    error = read_request(sock_fd,response);
                    if(error==-1){
                        printf("Something went wrong\n");
                    }else{
                        //Lock the mutex
                        if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
                            perror2("pthread mutex lock",err);
                            exit(EXIT_FAILURE);
                        }
                        printf("%s",response);
                        nbytes = write(clientInfo.fd,response,strlen(response)); //Send the result to the client
                        if(nbytes<strlen(response)){
                            printf("Something went wrong during sending the response\n");
                        }
                        //Unlock the mutex
                        if (err=pthread_mutex_unlock(&mtx))
                        {
                            perror2("pthread mutex unlock\n",err);
                            exit(EXIT_FAILURE);
                        }
                    }
                    close(sock_fd);
                }                
            }

            if (request_code==-1)   //Invalid request
            {
                write(clientInfo.fd,"Invalid request\n",strlen("Invalid request\n"));
            }
            
        }
        close(clientInfo.fd);
        memset(clientInfo.address,0,100);
        //Lock the mutex
        if(err=pthread_mutex_lock(&mtx)){   //Lock mutex
            perror2("pthread mutex lock",err);
             exit(EXIT_FAILURE);
        }
        pthread_cond_signal(&cvar2);
        //Unlock the mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }
    }
    
}

void signal_handler(int signal_number) {
    int err;
    printf("Caught the signal\n");
    //Terminate all threads
    for(int i=0;i<number_of_threads;i++)
        pthread_kill(pthreads[i],SIGKILL);
    free(pthreads);
    //Destroy condition variable
    if(err = pthread_cond_destroy(&cvar)){
        perror2("pthread_cond_destroy",err);
        exit(EXIT_FAILURE);
    }
    if(err = pthread_cond_destroy(&cvar2)){
        perror2("pthread_cond_destroy",err);
        exit(EXIT_FAILURE);
    }
    free(pfds);
    //printf("Coming here\n");
    ServerHT_free();
    close(query_socket);
    close(stats_socket);
    exit(0);
}