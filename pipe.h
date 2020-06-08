#ifndef PIPE_H_
#define PIPE_H_
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<limits.h>
#include<sys/types.h>
#include<sys/stat.h>
#include"mydate.h"
#include<signal.h>
#include<dirent.h>

#define SERVER_FIFO_NAME "/tmp/serv_%d_fifo"
#define CLIENT_FIFO_NAME "/tmp/cli_%d_fifo"

#define BUFFER_SIZE 20

//The statistics of the file that the Worker sends to parent proccess
struct File_Statistics
{
    Date file_date;
    char Country[25];
    char Disease[25];
    int Age_counter[4];    //[0]->0-20,[1]->21-40,[2]->41-60,[3]->60+
};
typedef struct File_Statistics File_Stats;

void File_Stats_Print(File_Stats *stats){
    print_date(&stats->file_date);
    printf("Country:%s,Disease:%s\n",stats->Country,stats->Disease);
    printf("Patients between age 0-20:%d\n",stats->Age_counter[0]);
    printf("Patients between age 21-40:%d\n",stats->Age_counter[1]);
    printf("Patients between age 41-60:%d\n",stats->Age_counter[2]);
    printf("Patients between age 60+:%d\n",stats->Age_counter[3]);
}

///////////////////////////////////////////////////////////////////////////////

struct requestStats
{
    int totalRequests;
    int successRequests;
    int failedRequests;
};

#endif /* PIPE_H_ */