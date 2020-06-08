#ifndef MYDATE_H_
#define MYDATE_H_
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

struct myDate{
    int day;
    int month;
    int year;
};

typedef struct myDate Date;

Date get_date(char *date_in_char){//Converts string date to myDate struct 
    Date date_in_int;
    int i=0;
    int j=0;
    char temp[5];
    //Get day
    while(date_in_char[i]!='-'){
        temp[j]=date_in_char[i];
        i++;
        j++;
    }
    temp[j]='\0';
    date_in_int.day=atoi(temp);
    //Get month
    j=0;
    i++;
    while(date_in_char[i]!='-'){
        temp[j]=date_in_char[i];
        i++;
        j++;
    }
    temp[j]='\0';
    date_in_int.month=atoi(temp);
    //Get year
    j=0;
    i++;
    while(date_in_char[i]!='\0'){
        temp[j]=date_in_char[i];
        i++;
        j++;
    }
    temp[j]='\0';
    date_in_int.year=atoi(temp);
    return date_in_int;    
}

void set_date(Date *date_to_set,int d,int m,int y){//FILL Date structure
    date_to_set->day=d;
    date_to_set->month=m;
    date_to_set->year=y;
}

int check_dates(Date date1,Date date2){//Date1->Entry date,Date2->exit date
    //if date1 is smaller than date 2 return 0,else return 1
    if(date1.year>date2.year){
        return 1;
    }
    if((date1.year == date2.year) && (date1.month>date2.month)){
        return 1;
    }
    if((date1.year == date2.year) && (date1.month == date2.month) && (date1.day>date2.day)){
        return 1;
    }
    return 0;
}

void print_date(Date *date_to_print){
    if(date_to_print->day==100){
        printf("Exit date not given\n");
        return;
    }
    if(date_to_print->day==0){
        printf("Entry date not given\n");
        return;
    }
    printf("%d-%d-%d\n",date_to_print->day,date_to_print->month,date_to_print->year);
}

//////////DATA STRUCTURE TO SORT DATES//////////////////////
struct date_list_node
{
    char stringDate[15];
    Date numericDate;
    struct date_list_node *next;
    struct date_list_node *prev;
};
typedef struct date_list_node *dateListptr;

//Insert a date in the right position in the list 
void date_list_insert(dateListptr *ptraddr,char *filedate){
    dateListptr temp_ptr;
    Date newDate = get_date(filedate);
    while((*ptraddr)!=NULL){
        if(check_dates(newDate,(*ptraddr)->numericDate)){
            temp_ptr=*ptraddr;
            ptraddr=&((*ptraddr)->next);
        }
        else
        {   
            temp_ptr = *ptraddr;
            *ptraddr = malloc(sizeof(struct date_list_node));
            (*ptraddr)->numericDate = newDate;
            strcpy((*ptraddr)->stringDate,filedate);
            (*ptraddr)->next=temp_ptr;
            (*ptraddr)->prev = temp_ptr->prev;
            if(temp_ptr->prev!=NULL){
                temp_ptr->prev->next = *ptraddr;
            }
            temp_ptr->prev = *ptraddr;
            return;
        }
        
    }
    //We reached the end of the list
    *ptraddr = malloc(sizeof(struct date_list_node));
    (*ptraddr)->numericDate=newDate;
    strcpy((*ptraddr)->stringDate,filedate);
    (*ptraddr)->next=NULL;
    if(temp_ptr!=NULL){
        temp_ptr->next=*ptraddr;
        (*ptraddr)->prev=temp_ptr;
    }
    else    //First node of the list
    {
        (*ptraddr)->prev=NULL;
    }
    
}

//Get item from Datelist
int dateListPop(dateListptr *ptraddr,dateListptr returnDate){
    if((*ptraddr)==NULL){
        return 0;
    }
    dateListptr temp = *ptraddr;//Save the address of the node
    strcpy(returnDate->stringDate,(*ptraddr)->stringDate);
    returnDate->numericDate = (*ptraddr)->numericDate;
    *ptraddr = (*ptraddr)->next;        //get new head of the queue
    free(temp);                 //free the old head of the queue
    return 1;
}

void datelistPrint(dateListptr listptr){
    dateListptr temp = listptr;
    while(temp!=NULL){
        printf("%s\n",temp->stringDate);
        temp=temp->next;
    }
}

#endif /* MYDATE_H_ */