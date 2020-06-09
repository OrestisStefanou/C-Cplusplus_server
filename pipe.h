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
#include"request.h"

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

//StatsTree
struct FileStatsTreeNode{
    struct File_Statistics fileStats;
    struct FileStatsTreeNode *right;
    struct FileStatsTreeNode *left;
};
typedef struct FileStatsTreeNode *FileStatsTreePtr;

//Insert FileStats in the tree(Sorted by date)
FileStatsTreePtr add_FileStatsTree_node(FileStatsTreePtr p,struct File_Statistics fStats){
    if(p==NULL){//Tree is empty
        p = (struct FileStatsTreeNode *)malloc(sizeof(struct FileStatsTreeNode));
        p->fileStats.file_date=fStats.file_date;
        strcpy(p->fileStats.Country,fStats.Country);
        strcpy(p->fileStats.Disease,fStats.Disease);
        for(int i=0;i<4;i++){
            p->fileStats.Age_counter[i]=fStats.Age_counter[i];
        }
        p->left=NULL;
        p->right=NULL;
    }
    else if(check_dates(fStats.file_date,p->fileStats.file_date)){
        p->right=add_FileStatsTree_node(p->right,fStats);
    }
    else if(check_dates(fStats.file_date,p->fileStats.file_date)==0){
        p->left=add_FileStatsTree_node(p->left,fStats);
    }
    return p;
}

void FileStatsTreePrint(FileStatsTreePtr p){
    if(p!=NULL){
        File_Stats_Print(&p->fileStats);
        FileStatsTreePrint(p->left);
        FileStatsTreePrint(p->right);
    }
}

void freeFileStatsTree(FileStatsTreePtr p){
    if(p==NULL){
        return;
    }
    freeFileStatsTree(p->left);
    freeFileStatsTree(p->right);
    free(p);
}
//Use it in topk-AgeRange query
struct topkAgeRangeData topkAgeRangeCount(FileStatsTreePtr p,char *d,Date date1,Date date2){
    struct topkAgeRangeData data,data2;
    for(int i=0;i<4;i++){
        data.ages[i]=0;
    }
    data.total_patients=0;
    if(p!=NULL){
        if(check_dates(p->fileStats.file_date,date1)==1 && check_dates(date2,p->fileStats.file_date)==1 && strcmp(p->fileStats.Disease,d)==0){
            for(int i=0;i<4;i++){
                data.ages[i] = data.ages[i] + p->fileStats.Age_counter[i];
                data.total_patients+=p->fileStats.Age_counter[i];
            }
        }
        if(check_dates(date1,p->fileStats.file_date)){
            data2=topkAgeRangeCount(p->right,d,date1,date2);
            for(int i=0;i<4;i++){
                data.ages[i]+=data2.ages[i];
                data.total_patients+=data2.ages[i];
            }
        }
        else
        {
            data2=topkAgeRangeCount(p->right,d,date1,date2);
            for(int i=0;i<4;i++){
                data.ages[i]+=data2.ages[i];
                data.total_patients+=data2.ages[i];
            }
            data2=topkAgeRangeCount(p->left,d,date1,date2);
            for(int i=0;i<4;i++){
                data.ages[i]+=data2.ages[i];
                data.total_patients+=data2.ages[i];
            }
        }
        
    }
    return data;
}

//Use it for numPatientAdmissions
int countAdmissionPatients(FileStatsTreePtr p,char *d,Date date1,Date date2){
    int count=0;
    if(p!=NULL){
        if(check_dates(p->fileStats.file_date,date1)==1 && check_dates(date2,p->fileStats.file_date)==1 && strcmp(p->fileStats.Disease,d)==0){
            for(int i=0;i<4;i++){
                count=count + p->fileStats.Age_counter[i];
            }
        }
        if(check_dates(date1,p->fileStats.file_date)){
            count+=countAdmissionPatients(p->right,d,date1,date2);
        }
        else
        {
            count+=countAdmissionPatients(p->right,d,date1,date2);
            count+=countAdmissionPatients(p->left,d,date1,date2);
        }
        
    }
    return count;
}
///////////////////////////////////////////////////////////////////////////////

struct requestStats
{
    int totalRequests;
    int successRequests;
    int failedRequests;
};

#endif /* PIPE_H_ */