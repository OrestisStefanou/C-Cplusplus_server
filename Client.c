#include"network.h"
#include"Client_functions.h"

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
    if (argc > 1) {
        strncpy(server_name, argv[1], SERVER_NAME_LEN_MAX);
    } else {
        printf("Enter Server Name: ");
        scanf("%s", server_name);
    }

    // Get server port from command line arguments or stdin. 
    server_port = argc > 2 ? atoi(argv[2]) : 0;
    if (!server_port) {
        printf("Enter Port: ");
        scanf("%d", &server_port);
    }

    thread_number = 4;  //Get this from the command line
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

    strcpy(buf,"/diseaseFrequency H1N1 01-01-2000 01-01-2030\n");
    for (int i = 0; i < 10; i++)
    {
        //Lock mutex
        if(err=pthread_mutex_lock(&mtx)){
            perror2("phtread_mutex_lock",err);
            exit(EXIT_FAILURE);
        }

        requests_array_insert(buf);

        if(requests_array_full()){
            print_request_array();
            printf("Array is full... broadcasting\n");
            pthread_cond_broadcast(&cvar);
            printf("Main waiting for signal\n");
            while(requests_array_empty()==0){
                pthread_cond_wait(&cvar2,&mtx);
            }
            printf("Main got the signal\n");
        }

        //Unlock mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }

    }

    printf("Main broadcasting\n");
    pthread_cond_broadcast(&cvar);
    printf("Main waiting for signal\n");
    while(requests_array_empty()==0){
        pthread_cond_wait(&cvar2,&mtx);
    }
    printf("Main got the signal\n");

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

        printf("Thread waiting for signal\n");
        
        pthread_cond_wait(&cvar,&mtx);  //Wait for signal

        printf("Thread coming after wait\n");

        requests_array_get(request);
        printf("Request is %s\n",request);

        if(requests_array_empty()){
            printf("Array is empty.Sending signal to main\n");
            print_request_array();
            pthread_cond_signal(&cvar2);
        }
        //Unlock mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }
    }
/*    int socket_fd;
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


    char buffer[100];
    memset(buffer,0,100);
    strcpy(buffer,"/numPatientDischarges H1N1 01-01-1900 01-01-2050 USA\n");
    write(socket_fd,buffer,strlen(buffer));
    int response;
    //read(socket_fd,&response,sizeof(int));
    //printf("%d\n",response);
    while (read(socket_fd,&buffer[0],1))
    {
        printf("%c",buffer[0]);
    }
    
    close(socket_fd);*/
}