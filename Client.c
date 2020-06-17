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

int started=0,thread_number,thread_counter=0;

int main(int argc, char const *argv[])
{   
    int err;
    char buf[100];
    pthread_attr_t pthread_attr;
    pthread_t *pthreads;
    pthread_t pthread;

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

    requests_array_size = thread_number;
    init_requests_array();

    FILE *fp = fopen(argv[2],"r");

    /* Initialise pthread attribute to create detached threads. */
    if (pthread_attr_init(&pthread_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }
    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }

    int flag=0;
    while(1)
    {
        for(int i=0;i<thread_number;i++){
                //printf("Buf is %s",buf);
                //Lock mutex
                if(err=pthread_mutex_lock(&mtx)){
                    perror2("phtread_mutex_lock",err);
                    exit(EXIT_FAILURE);
                }
                if(fgets(buf,100,fp)){                
                    /* Create thread to serve connection to client. */
                    if (pthread_create(&pthreads[i],NULL, thread_function,NULL) != 0) {
                        perror("pthread_create");
                        continue;
                    }
                    thread_counter++;
                    //pthread_cond_wait(&cvar,&mtx);
                    printf("Main inserting\n");
                    err = requests_array_insert(buf);
                    if(err==-1){
                        printf("Something went wrong during insert\n");
                    }
                    //Unlock mutex
                    if (err=pthread_mutex_unlock(&mtx))
                    {
                        perror2("pthread mutex unlock\n",err);
                        exit(EXIT_FAILURE);
                    }
                }   
            else{
                flag=1;
                break;
            }    
        }

        if (thread_counter==0)
        {
            break;
        }

        //Lock mutex
        if(err=pthread_mutex_lock(&mtx)){
            perror2("phtread_mutex_lock",err);
            exit(EXIT_FAILURE);
        }


        printf("Thread counter is %d\n",thread_counter);
        while(started!=thread_counter){
            pthread_cond_wait(&cvar,&mtx);
        }
        thread_counter=0;
        
        pthread_cond_broadcast(&cvar2);
        printf("Broadcasting and waiting\n");
        while(requests_array_empty()==0 || started!=0){
            pthread_cond_wait(&cvar,&mtx);
        }
        
        //Unlock mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }
        //for(int i=0;i<flag;i++){
        //    pthread_join(pthreads[i],NULL);
        //}
        if(flag>0){
            break;
        }
    }

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
    
    //Lock mutex
    if(err=pthread_mutex_lock(&mtx)){
        perror2("phtread_mutex_lock",err);
        exit(EXIT_FAILURE);
    }

    //printf("Request is %s\n",request);

    //pthread_cond_signal(&cvar);
    printf("Thread waiting\n");
    started++;
    printf("Started:%d\n",started);
    if(started==thread_counter)
        pthread_cond_signal(&cvar);
    //Unlock mutex
    if (err=pthread_mutex_unlock(&mtx))
    {
        perror2("pthread mutex unlock\n",err);
        exit(EXIT_FAILURE);
    }
    //Lock mutex
    if(err=pthread_mutex_lock(&mtx)){
        perror2("phtread_mutex_lock",err);
        exit(EXIT_FAILURE);
    }
    pthread_cond_wait(&cvar2,&mtx);

    err = requests_array_get(request);
    if(err==-1){
        printf("Something went wrong during get\n");
    }
    printf("Thread got request %s",request);
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
        
        //Lock mutex
        if(err=pthread_mutex_lock(&mtx)){
            perror2("phtread_mutex_lock",err);
            exit(EXIT_FAILURE);
        }
        // Connect to socket with server address.
        if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1) {
            perror("connect");
            exit(1);
        }
        write(socket_fd,request,strlen(request));
        printf("Request is %s\n",request);
        //Get the response
        char c;
        while (read(socket_fd,&c,1))
        {
            printf("%c",c);
        }

        started--;
        printf("Started:%d\n",started);
        if(started==0)
            pthread_cond_signal(&cvar);

        pthread_cond_signal(&cvar2);    //TESTING

        //Unlock mutex
        if (err=pthread_mutex_unlock(&mtx))
        {
            perror2("pthread mutex unlock\n",err);
            exit(EXIT_FAILURE);
        }

        close(socket_fd);
    }
    printf("Thread exiting\n");
    return NULL;
}