#ifndef PATIENT_H_
#define PATIENT_H_
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include"mydate.h"

struct patient_record
{
    char recordID[10];
    char name[25];
    char surname[25];
    char status[10];
    char disease[25];
    char country[25];
    int age;
    Date filedate;
};

void print_record_info(struct patient_record *record){
    printf("-------------------------\n");
    printf("RecordID:%s\n",record->recordID);
    printf("FirstName:%s\n",record->name);
    printf("LastName:%s\n",record->surname);
    printf("DiseaseID:%s\n",record->disease);
    printf("Country:%s\n",record->country);
    printf("Status:%s\n",record->status);
    printf("Age:%d\n",record->age);
    printf("Date:");
    print_date(&record->filedate);
    printf("-------------------------\n");
}

void read_record(struct patient_record *record,char *buf){//Fill patient_record structure
    int i=0;//buf index variable
    
    //GET RECORD_ID
    while(buf[i]!=32){
        record->recordID[i] = buf[i];
        i++;
    }
    record->recordID[i] = '\0';
    i++;

    //GET STATUS
    int j=0;
    while(buf[i]!=32){
        record->status[j] = buf[i];
        i++;
        j++;
    }
    i++;
    record->status[j] = '\0';

    //GET FIRSTNAME
    j=0;
    while(buf[i]!=32){
        record->name[j] = buf[i];
        i++;
        j++;
    }
    i++;
    record->name[j] = '\0';

    //GET LASTNAME
    j=0;
    while(buf[i]!=32){
        record->surname[j] = buf[i];
        i++;
        j++;
    }
    i++;
    record->surname[j] = '\0';

    //GET DISEASE
    j=0;
    while(buf[i]!=32){
        record->disease[j] = buf[i];
        i++;
        j++;
    }
    i++;
    record->disease[j] = '\0';

    //GET AGE
    char age[4];
    j=0;
    while(buf[i]!='\n'){
        age[j] = buf[i];
        i++;
        j++;
    }
    i++;
    age[j] = '\0';
    record->age=atoi(age);
}


#endif /* PATIENT_H_ */