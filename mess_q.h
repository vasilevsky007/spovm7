//
// Created by alex on 11.04.22.
//

#ifndef LAB7_MESS_Q_H
#define LAB7_MESS_Q_H
#include<fcntl.h>
#include<sys/stat.h>
#include<semaphore.h>
typedef struct {
    unsigned char type;
    unsigned short hash;
    unsigned char size;
    unsigned char* data;
}message;

unsigned char* mes_to_char(message* mes){
    unsigned char* converted;
    converted=(unsigned char*)malloc(5+mes->size);
    converted[0]=mes->type;
    converted[1]=mes->hash;
    converted[2]=mes->hash>>8;
    converted[3]=mes->size;
    for(int i=4;i<=((mes->size)+4);i++){
        converted[i]=mes->data[i-4];
    }
    return converted;
}

message* char_to_mes(unsigned char* converted){
    message* mes;
    mes=(message*)malloc(sizeof(message));
    converted=(unsigned char*)malloc(5+mes->size);
    mes->type=converted[0];
    mes->hash=converted[2]<<8;
    mes->hash=converted[1]|mes->hash;
    mes->size=converted[3];
    mes->data=malloc((mes->size)+1);
    for(int i=4;i<=((mes->size)+4);i++){
        mes->data[i-4]=converted[i];
    }
}

void printmes(message* mes){
    printf("\nmessage \n");
    printf("%hhu %hu %hhu %s ",mes->type,mes->hash,mes->size,mes->data);
}

typedef struct {
    unsigned char* start;
    unsigned char* end;
    volatile unsigned char* readptr;
    volatile unsigned char* writeptr;
    unsigned int _capacity;
    int counter_added;
    int counter_consumed;
    unsigned char* buffer;
}message_q;

message_q* addq(unsigned int length,int md1,int md2){
    message_q* newq=(message_q*)mmap(NULL, sizeof(message_q),PROT_READ|PROT_WRITE,MAP_SHARED,md1,0);
    if(newq==MAP_FAILED){
        perror("unable to create nmap for queue: ");
        return NULL;
    }

    newq->buffer= (unsigned char *)mmap(NULL,length,PROT_READ|PROT_WRITE,MAP_SHARED,md2,0);
    if(newq->buffer==MAP_FAILED){
        perror("unable to create nmap for queue buffer");
        munmap(newq, sizeof(message_q));
        return NULL;
    }
    newq->_capacity=length;
    newq->start = newq->buffer;
    newq->end = newq->buffer;
    newq->counter_added=0;
    newq->counter_consumed=0;
    return newq;
}

void delq(message_q* mq,unsigned int capacity){
    if(mq==NULL)return;
    munmap(mq->buffer,capacity);
    munmap(mq,sizeof (message_q));
}

int trywrite(message_q * q,sem_t* zapoln,sem_t*qaccess) {
    message mes;
    short r1;
    unsigned char r2;
    r1=(short)(rand()%257);
    if (r1==256){
        mes.type=1;
        mes.type=mes.type<<7;
    }else mes.size=(unsigned char)r1;
    mes.data=malloc((r1+1)* sizeof(char));
    for (int i=0; i<r1;i++){
        r2=(unsigned char)((rand()%95)+32);
        mes.data[i]=r2;
    }mes.data[r1]='\0';
    printmes(&mes);
    printf("\ngenerated, trying to add to queue\n");
    unsigned char* temp= mes_to_char(&mes);
    int flag=1;
    while(flag){
        sem_wait(qaccess);
        unsigned int capacity;
        capacity=q->_capacity;
        sem_post(qaccess);
        if(capacity>=(mes.size+5)){
            sem_wait(zapoln);
            sem_wait(qaccess);
            q->_capacity-=(mes.size+5);
            sem_post(qaccess);
            for(int i=0;i<mes.size+5;i++){
                *(q->writeptr)=temp[i];
                if(q->writeptr==q->end)q->writeptr=q->start;
                else q->writeptr+= 1;
            }
            sem_post(zapoln);
            printmes(&mes);
            printf("\nadded to queue");
            flag=0;
        }
    }
    free(mes.data);
    return 0;
}

int tryread(message_q*q, sem_t* izvl, sem_t* qaccess){
    message* mes;
    sem_wait(qaccess);
    if(q->readptr==q->writeptr){
        printf("\nunable to consume item, no items in queue\n");
        sem_post(qaccess);
        return 1;
    }
    sem_post(qaccess);
    sem_wait(izvl);
    mes= char_to_mes((unsigned char *) q->readptr);
    sem_wait(qaccess);
    for(int i=0;i<(mes->size)+5;i++){
        if (q->readptr==q->end) q->readptr=q->start;
        else q->readptr+=1;
    }
    sem_post(qaccess);
    sem_post(izvl);
    printmes(mes);
    printf("\nconsumed\n");
    free(mes->data);
    free(mes);
    return 0;
}
#endif //LAB7_MESS_Q_H
