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

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;    //Initialize thread mutex

pthread_cond_t cvar;    //Condition variable
pthread_t *pthreads;    //Array with thread ids

int number_of_threads = 4;  //Get this from command line arg

//Network variables
int query_socket,stats_socket;

//Poll set 
struct pollfd *pfds;

int main(int argc, char const *argv[])
{   
    //Initialize Hashtable
    ServerHT_init(15);

    //Network variables
    int port,new_socket_fd,err;
    struct sockaddr_in address;
    struct sockaddr_storage client_address;
    socklen_t client_address_len;
    char remoteIP[INET6_ADDRSTRLEN];

    //Poll set varibles
    int fd_count = 0;
    int fd_size = 2;
    pfds = malloc(sizeof *pfds * fd_size); //Create set to monitor the 2 listening sockets    

    //Check command line arguments
    if (argc < 2)
    {
        printf("Please give port number and buffer size\n");
        exit(EXIT_FAILURE);
    }

    pthreads = malloc(number_of_threads * sizeof(pthread_t));   //Create the array of thread ids
    
    //Create file descriptor buffer
    buffer_size = atoi(argv[2]);
    buffer_init();  //Initialize the buffer

    //Initialize condition variable
    pthread_cond_init(&cvar,NULL);        

    port = atoi(argv[1]);   //Port number to listen

    //Initialize IPv4 address
    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
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
    address.sin_port = htons(port+1);   //CHANGE THIS TO THE PORT GIVEN FROM CMD LINE
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
                    pthread_cond_wait(&cvar,&mtx);  //wait for signal
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
        while(1){
            read(clientInfo.fd,&buffer[i],1);
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
            printf("Got message %s\n",buffer);
            int request_code = get_request_code(buffer);
            if(request_code==3){    //Topk-AgeRanges
                //Here i have to lock the mutex
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
                    while(temp!=NULL){
                        //Send the request to the worker
                        //Create the message with the information needed
                        char msg[300];
                        sprintf(msg,"%s %s %d-%d-%d %d-%d-%d %s %d\n","df",info.virusName,info.entry_date.day,
                        info.entry_date.month,info.entry_date.year,info.exit_date.day,info.exit_date.month,
                        info.exit_date.year,info.country,0);
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
                    //LOCK THE MUTEX HERE
                    printf("%d\n",sum);
                    //UNLCOK
                }else
                {
                    //Country given.Send the request to the responsible worker
                    ServerHT_Entry *ht_entry = ServerHT_get(info.country);
                    //Send the request to the worker
                    //Create the message with the information needed
                    char msg[300];
                    sprintf(msg,"%s %s %d-%d-%d %d-%d-%d %s %d\n","df",info.virusName,info.entry_date.day,
                    info.entry_date.month,info.entry_date.year,info.exit_date.day,info.exit_date.month,
                    info.exit_date.year,info.country,0);
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
                        //LOCK THE MUTEXT
                        printf("%d\n",result);
                        //UNLOCK THE MUTEX
                    }
                    close(sock_fd);
                }
                
            }
        }
        close(clientInfo.fd);
        memset(clientInfo.address,0,100);
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
    free(pfds);
    //printf("Coming here\n");
    ServerHT_free();
    close(query_socket);
    close(stats_socket);
    exit(0);
}