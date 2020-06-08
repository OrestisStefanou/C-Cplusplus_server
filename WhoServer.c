#include"network.h"
#include"buffer.h"

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
                printf("New connection from:%s\n",remoteIP);
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
    int err;
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
        printf("Thread handling the connection\n");
        //Get the info of the client to serve
        clientInfo = buffer_get();
        //Unlock the mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }
        char buffer[100];   //Buffer to get info of the incoming request
        read(clientInfo.fd,buffer,sizeof(buffer));
        if(strcmp(buffer,"port")==0){
            int worker_port;
            read(clientInfo.fd,&worker_port,sizeof(int));
            printf("Worker is listening on port %d\n",worker_port);
        }else
        {
            printf("Got message %s\n",buffer);
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
    close(query_socket);
    close(stats_socket);
    exit(0);
}