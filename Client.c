#include"network.h"
#include"Client_functions.h"
#include"request.h"

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

#define SERVER_NAME_LEN_MAX 255

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;    //Thread mutex

pthread_cond_t cvar,cvar2;    //Condiotion variable

void *thread_function(void *);  //Declare thread function

char server_name[SERVER_NAME_LEN_MAX + 1] = { 0 };
int server_port;

int thread_counter=0,thread_number;

int main(int argc, char const *argv[])
{   
    int err;
    char buf[100];
    pthread_attr_t pthread_attr;
    pthread_t *pthreads;

    //Initialize condition variable
    pthread_cond_init(&cvar, NULL);
    pthread_cond_init(&cvar2, NULL);

    // Get server name from command line arguments or stdin. 
    if (argc == 9) {
        strncpy(server_name, argv[8], SERVER_NAME_LEN_MAX);
        server_port = atoi(argv[6]);
        thread_number = atoi(argv[4]);
    } else {
        printf("Usage is:./whoClient –q queryFile -w numThreads –sp servPort –sip servIP\n");
        exit(EXIT_FAILURE);
    }

    pthreads = malloc(thread_number * sizeof(pthread_t));    //Create the array with pthread's id
    //Create the threads
    for(int i=0;i<thread_number;i++){
        if(err=pthread_create(&pthreads[i],NULL,thread_function,NULL)){
            perror2("pthread_create",err);
            exit(EXIT_FAILURE);
        }
    }

    sleep(1);

    requests_array_size = thread_number;
    init_requests_array();

    FILE *fp = fopen(argv[2],"r");

    while(fgets(buf,100,fp))
    {
        //Lock mutex
        if(err=pthread_mutex_lock(&mtx)){
            perror2("phtread_mutex_lock",err);
            exit(EXIT_FAILURE);
        }

        while(requests_array_full()){
            pthread_cond_wait(&cvar2,&mtx);
        }

        requests_array_insert(buf);
        pthread_cond_signal(&cvar);

        //Unlock mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }

    }

    //Lock mutex
    if(err=pthread_mutex_lock(&mtx)){
        perror2("phtread_mutex_lock",err);
        exit(EXIT_FAILURE);
    }

    while (requests_array_empty()==0)
    {   
        pthread_cond_wait(&cvar2,&mtx);
    }
    
    //Unlock mutex
    if (err=pthread_mutex_unlock(&mtx))
    {
        perror2("pthread mutex unlock\n",err);
        exit(EXIT_FAILURE);
    }

    //Terminate all threads
    for(int i=0;i<thread_number;i++)
        pthread_kill(pthreads[i],SIGKILL);
    free(pthreads);

    //Destroy condition variable
    if(err=pthread_cond_destroy(&cvar)){
        perror2("pthread_cond_destroy",err);
        exit(EXIT_FAILURE);
    }

    //Destroy condition variable
    if(err=pthread_cond_destroy(&cvar2)){
        perror2("pthread_cond_destroy",err);
        exit(EXIT_FAILURE);
    }
    return 0;
}

void *thread_function(void *argp){
    int err;
    char request[100];

    while(1){
        //Lock mutex
        if(err=pthread_mutex_lock(&mtx)){
            perror2("phtread_mutex_lock",err);
            exit(EXIT_FAILURE);
        }

        //printf("Thread waiting for signal\n");
        while(requests_array_empty()){
            pthread_cond_wait(&cvar,&mtx);  //Wait for signal
        }

        //printf("Thread coming after wait\n");

        err = requests_array_get(request);
        if(err==-1){
            printf("Something went wrong durring geting request\n");
        }
    
        //Unlock mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }

        int request_code = get_request_code(request);
        if(request_code!=-1){
            //Send the request
            int socket_fd;
            struct hostent *server_host;
            struct sockaddr_in server_address;

            // Get server host from server name.
            server_host = gethostbyname(server_name);

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

            write(socket_fd,request,strlen(request));
        
            //Lock mutex
            if(err=pthread_mutex_lock(&mtx)){
                perror2("phtread_mutex_lock",err);
                exit(EXIT_FAILURE);
            }

            printf("Request is %s\n",request);
            //Get the response
            char c;
            while (read(socket_fd,&c,1))
            {
                printf("%c",c);
            }

            //Unlock mutex
            if (err=pthread_mutex_unlock(&mtx))
            {
                perror2("pthread mutex unlock\n",err);
                exit(EXIT_FAILURE);
            }

            close(socket_fd);
        }
        pthread_cond_signal(&cvar2);
    }
}