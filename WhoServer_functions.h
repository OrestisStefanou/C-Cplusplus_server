#ifndef WHOSERVER_FUNCTIONS_H_
#define WHOSERVER_FUNCTIONS_H_
#include<stdio.h>
#include<string.h>
#include"WhoServerDataStructs.h"
#include"request.h"

//Send a request to a Worker
void Connect_to_Worker(char *server_name,int port){
    int server_port,socket_fd;
    struct hostent *server_host;
    struct sockaddr_in server_address;

    server_port = port;

    /* Get server host from server name. */
    server_host = gethostbyname(server_name);

    /* Initialise IPv4 server address with server host. */
    memset(&server_address, 0, sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr, server_host->h_length);

    /* Create TCP socket. */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    /* Connect to socket with server address. */
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1) {
		perror("connect");
        exit(1);
	}

    /* TODO: Put server interaction code here. For example, use
     * write(socket_fd,,) and read(socket_fd,,) to send and receive messages
     * with the client.
     */
    char buffer[100];
    memset(buffer,0,100);
    strcpy(buffer,"/topk-AgeRanges 4 China H1N1 01-01-1900 01-01-2050\n");
    int res = write(socket_fd,buffer,strlen(buffer));
    if(res<strlen(buffer))
        printf("Soemthing went wrong during sending the message\n");
    close(socket_fd);
}

//Bubble sort function
void swap(struct ageRangeStats *x,struct ageRangeStats *y){
    struct ageRangeStats temp = *x;
    *x=*y;
    *y = temp;
}

void bubbleSort(struct ageRangeStats arr[],int n){
    int i,j;
    for(i=0;i<n-1;i++){
        for(j=0;j<n-i-1;j++){
            if(arr[j].number < arr[j+1].number){
                swap(&arr[j],&arr[j+1]);
            }
        }
    }
}

//fd is the file descriptor of client's socket
int topkRanges(char *buf,int fd){
    int k=0;
    char country[25];
    char disease[25];
    Date entryDate;
    Date exitDate;
    char temp_date[5];
    int i=0,j=0;

    //Skip request command
    while(buf[i]!=' ' && buf[i]!='\n'){
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;

    //Get k
    while(buf[i]!=' ' && buf[i]!='\n'){
        temp_date[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;
    temp_date[j]='\0';
    k=atoi(temp_date);

    //Get country
    j=0;
    while(buf[i]!=' ' && buf[i]!='\n'){
        country[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;
    country[j]='\0';

    //Get virus
    j=0;
    while(buf[i]!=' ' && buf[i]!='\n'){
        disease[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;
    disease[j]='\0';

    //Get enter date
    j=0;
    //get day
    while(buf[i]!='-' && buf[i]!='\n'){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    temp_date[j]='\0';
    entryDate.day=atoi(temp_date);
    i++;
    j=0;
    //get month
    while(buf[i]!='-' && buf[i]!='\n'){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    temp_date[j]='\0';
    entryDate.month=atoi(temp_date);
    i++;
    j=0;
    //get year
    while(buf[i]!=' ' && buf[i]!='\n'){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    temp_date[j]='\0';
    entryDate.year=atoi(temp_date);
    i++;

    //Get exit date
    j=0;
    //get day
    while(buf[i]!='-' && buf[i]!='\n'){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    temp_date[j]='\0';
    exitDate.day=atoi(temp_date);
    i++;
    j=0;
    //get month
    while(buf[i]!='-' && buf[i]!='\n'){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    temp_date[j]='\0';
    exitDate.month=atoi(temp_date);
    i++;
    j=0;
    //get year
    while(buf[i]!='\n' && buf[i]!=' '){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    temp_date[j]='\0';
    exitDate.year=atoi(temp_date);
    //printf("K=%d\n",k);
    //printf("Country:%s\n",country);
    //printf("Disease:%s\n",disease);
    //printf("Entry date:");
    //print_date(&entryDate);
    //printf("Exit date:");
    //print_date(&exitDate);

    struct topkAgeRangeData data;
    struct ageRangeStats array[4];
    ServerHT_Entry *ht_entry = ServerHT_get(country);    //Get the entry from the Hashtable
    if(ht_entry==NULL){
        printf("Something went wrong\n");
        return -1;
    }

    //TEST CONNECTION WITH CLIENT
    Connect_to_Worker(ht_entry->worker_address,ht_entry->worker_port);

    FileStatsTreePtr root = ht_entry->CountryStatsTree; //Get the root of the tree where the stats are
    data = topkAgeRangeCount(root,disease,entryDate,exitDate);
    //printf("Total patients %d\n",data.total_patients);
    for(i=0;i<4;i++){   //Insert the data in the array to sort them
        array[i].index=i;
        array[i].number=data.ages[i];
    }
    //Sort the array
    bubbleSort(array,4);
    //Transform the array to % format
    float num;
    for(int i=0;i<4;i++){
        num = data.total_patients/array[i].number;
        array[i].number = 100 / num; 
    }
    //Print the results
    for(int i=0;i<k;i++){
        if(i>=4){
            break;
        }
        ageRangePrint(array[i],fd);
    }
}
#endif /* WHOSERVER_FUNCTIONS_H_ */