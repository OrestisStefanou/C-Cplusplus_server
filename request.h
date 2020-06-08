#ifndef REQUEST_H_
#define REQUEST_H_
#include"mydate.h"

//DiseaseFrequency data
struct dfData{
    char virusName[25];
    Date entry_date;
    Date exit_date;
    char country[25];
};
//topk-AgeRanges query struct
struct topkAgeRangeData{
    int total_patients;
    int ages[4];
};

//To sort topkAgeRangeCount records
struct ageRangeStats
{
    int index;
    float number;
};
//////////////////////////////
//To use in searchPatientRecord
struct searchPatientData{
    char id[10];
    char patientName[25];
    char patientLastName[25];
    char patientDisease[25];
    int patientAge;
    Date patientEntryDate;
    Date patientExitDate;
};
/////////////////////////////

//To use in PatientDischarges
struct PatientDischargesData{
    char virusName[25];
    char countryName[25];
    Date entry_date;
    Date exit_date;
};
/////////////////////////////

void ageRangePrint(struct ageRangeStats stats){
    if(stats.index==0){
        printf("0-20:%0.0f%c\n",stats.number,'%');
        return;
    }

    if(stats.index==1){
        printf("21-40:%0.0f%c\n",stats.number,'%');
        return;
    }

    if(stats.index==2){
        printf("41-60:%0.0f%c\n",stats.number,'%');
        return;
    }

    if(stats.index==3){
        printf("60+:%0.0f%c\n",stats.number,'%');
        return;
    }
}
////////////////////////////////////

//Get the input of the user and fill the dfData structure
//returns -1 in case of error
int fill_dfData(char *buf,struct dfData *data){
    int i=0,j=0;
    char temp_date[5];
    //Skip request command
    while(buf[i]!=' ' && buf[i]!='\n'){
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;

    //Get virus name
    while(buf[i]!=' ' && buf[i]!='\n'){
        data->virusName[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;
    data->virusName[j]='\0';

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
    data->entry_date.day=atoi(temp_date);
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
    data->entry_date.month=atoi(temp_date);
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
    data->entry_date.year=atoi(temp_date);
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
    data->exit_date.day=atoi(temp_date);
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
    data->exit_date.month=atoi(temp_date);
    i++;
    j=0;
    //get year
    while(buf[i]!=' ' && buf[i]!='\n'){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    temp_date[j]='\0';
    data->exit_date.year=atoi(temp_date);
    if(buf[i]=='\n'){   //No country given
        data->country[0]=0;
        return 0;
    }
    //Get country
    i++;
    j=0;
    while (buf[i]!='\n' && buf[i]!=' ')
    {
        data->country[j]=buf[i];
        i++;
        j++;
    }
    data->country[j]='\0';
    return 0;
}

//Use it for searchPatientRecord
//Returns the recordID of the patient to search
//-1 on error
int getSearchPatientRecordId(char *buf,char *recordId){
    int i=0,j=0;
    char id[10];
    //Skip request command
    while(buf[i]!=' ' && buf[i]!='\n'){
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;

    //Get id
    while(buf[i]!='\n' && buf[i]!=' '){
        id[j] = buf[i];
        j++;
        i++;
    }
    i++;
    id[j]='\0';
    strcpy(recordId,id);
    return 1;    
}

//Get user request and fill struct PatientDischargesData struct
int fillPatientDischargesData(char *buf,struct PatientDischargesData *data){
    int i=0,j=0;
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
        data->virusName[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;
    data->virusName[j]='\0';

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
    data->entry_date.day=atoi(temp_date);
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
    data->entry_date.month=atoi(temp_date);
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
    data->entry_date.year=atoi(temp_date);
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
    data->exit_date.day=atoi(temp_date);
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
    data->exit_date.month=atoi(temp_date);
    i++;
    j=0;
    //get year
    while(buf[i]!='\n' && buf[i]!=' '){
        temp_date[j]=buf[i];
        j++;
        i++;
    }
    temp_date[j]='\0';
    data->exit_date.year=atoi(temp_date);

    if(buf[i]=='\n'){   //Country not given
        data->countryName[0]=0;
        return 0;
    }
    //Get country
    i++;
    j=0;
    while(buf[i]!=' ' && buf[i]!='\n'){
        data->countryName[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]==' '){
        return -1;
    }
    i++;
    data->countryName[j]='\0';
    return 0;
}

int getSiganlCountry(char *buf,char *c){
    int i=0,j=0;
    //Skip request command
    while(buf[i]!=' ' && buf[i]!='\n'){
        i++;
    }
    if(buf[i]=='\n'){
        return -1;
    }
    i++;

    //Get country name
    while(buf[i]!=' ' && buf[i]!='\n'){
        c[j] = buf[i];
        j++;
        i++;
    }
    if(buf[i]==' '){
        return -1;
    }
    i++;
    c[j]='\0';
    return 0;   
}
#endif /* REQUEST_H_ */