#ifndef PARENT_FUNCTIONS_H_
#define PARENT_FUNCTIONS_H_
#include<stdio.h>
#include<string.h>
#include"Parent_Data_Structures.h"
#include"request.h"

//Get the request code of a user request
int get_request_code(char *buf){
    char request[30];
    int i=0;
    while(buf[i]!=' ' && buf[i]!='\n'){
        request[i] = buf[i];
        i++;
    }
    request[i]='\0';
    if (strcmp(request,"/exit")==0){
        return 7;
    }
    if (strcmp(request,"/listCountries")==0){
        return 1;
    }
    if (strcmp(request,"/diseaseFrequency")==0){
        return 2;
    }
    if (strcmp(request,"/topk-AgeRanges")==0){
        return 3;
    }
    if (strcmp(request,"/searchPatientRecord")==0){
        return 4;
    }
    if (strcmp(request,"/numPatientAdmissions")==0){
        return 5;
    }
    if (strcmp(request,"/numPatientDischarges")==0){
        return 6;
    }
    if (strcmp(request,"/sendSIGUSR1")==0){
        return 8;
    }
    if (strcmp(request,"help")==0){
        return 9;
    }
    return -1;  //In case of invalid request
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

int topkRanges(char *buf){
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
    int index = getHashtable_index(country);    //Get the index of the country in the HT
    FileStatsTreePtr root = Hashtable[index].StatsTree; //Get the root of the tree where the stats are
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
        ageRangePrint(array[i]);
    }
}


int getPatientAdmissions(char *buf){
    int i=0,j=0;
    char virus[25];
    char country[25];
    Date entryDate;
    Date exitDate;
    char temp_date[5];

    //Skip request command
    while(buf[i]!=' ' && buf[i]!='\n'){
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;

    //Get disease
    while(buf[i]!=' ' && buf[i]!='\n'){
        virus[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;
    virus[j]='\0';
    
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

    if(buf[i]=='\n'){   //Country not given
        int sum;
        FileStatsTreePtr root;
        //Go through all the trees in the Hashtable
        for(int i=0;i<hashtable_size;i++){
            root = Hashtable[i].StatsTree;  //Get the root of the tree
            sum = countAdmissionPatients(root,virus,entryDate,exitDate);    //CountAdmissionPatients of this country
            if(sum>0){
                printf("%s %d\n",Hashtable[i].country,sum);
            }
        }
        return 0;
    }

    //Get country
    i++;
    j=0;
    while(buf[i]!=' ' && buf[i]!='\n'){
        country[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]==' '){
        return -1;
    }
    i++;
    country[j]='\0';
    int index = getHashtable_index(country);    //Get the index of the country in the HT
    if(index==-1){
        printf("There are no data for this country\n");
        return 0;
    }
    FileStatsTreePtr root=Hashtable[index].StatsTree;   //Get the root of the tree
    int sum=countAdmissionPatients(root,virus,entryDate,exitDate);  //Count admission Patients
    printf("%s %d\n",country,sum);
    return 0;  
}

#endif /* PARENT_FUNCTIONS_H_ */