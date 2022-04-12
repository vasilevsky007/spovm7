#include<stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/stat.h>
#include <string.h>
#include <stdio_ext.h>
#include <errno.h>
#include <sys/mman.h>
#include "mess_q.h"
#include <fcntl.h>
#include <signal.h>


int main(int argc, char *argv[], char *envp[]) {
    shm_unlink("/q_str");
    shm_unlink("/q_buf");
    int md1=shm_open("/q_str",O_CREAT|O_RDWR,S_IRWXG);
    if(md1==-1){
        perror("unable to create shared memory for queue structure: ");
        return -1;
    }
    int md2=shm_open("/q_buf",O_CREAT|O_RDWR,S_IRWXG);
    if(md2==-1){
        shm_unlink("/q_str");
        perror("unable to create shared memory for queue buffer: ");
        return -1;
    }


     message_q* queue= addq(8192,md1,md2);
    if(queue==NULL){
        shm_unlink("/q_str");
        shm_unlink("/q_buf");
        return -2;
    }


    sem_t* zapoln= sem_open("/zapoln",O_CREAT,S_IRWXG,1);
    if(zapoln==SEM_FAILED){
        perror("unable to create zapoln semaphore: ");
        shm_unlink("/q_str");
        shm_unlink("/q_buf");
        delq(queue,8192);
    }
    sem_t* izvl= sem_open("/izvl",O_CREAT,S_IRWXG,1);
    if(izvl==SEM_FAILED){
        perror("unable to create izvl semaphore: ");
        shm_unlink("/q_str");
        shm_unlink("/q_buf");
        sem_unlink("/zapoln");
        delq(queue,8192);
    }
    sem_t* qaccess= sem_open("/qaccess", O_CREAT, S_IRWXG, 1);
    if(izvl==SEM_FAILED){
        perror("unable to create qaccess semaphore: ");
        shm_unlink("/q_str");
        shm_unlink("/q_buf");
        sem_unlink("/zapoln");
        sem_unlink("/izvl");
        delq(queue,8192);
    }

    pid_t pid, ppid, chpid;
    int n = 0;

    while(1){
        char childname[9]="CHILD_\0";
        char chnum[3];
        int number=-1;
        sprintf(chnum, "%02d", n);
        chnum[2]='\0';
        strcat(childname,chnum);
        __fpurge(stdin);
        char input[100];
        scanf("%s",input);
        char* newargv[]={childname,NULL};
        switch(input[0]){
            case '+':
                chpid= fork();
                pid = getpid();
                ppid = getppid();
                if (chpid==0) {
                    printf("PRODUCER: My pid = %d, my ppid = %d \n", (int)pid, (int)ppid);
                    trywrite(queue, zapoln, qaccess);
                    return 0;
                }
                else {
                    //waitpid(chpid,NULL,0);
                    printf("PARENT: My pid = %d, my ppid = %d my child[%d] pid = %d\n", (int)pid, (int)ppid,n,(int)chpid);
                    n++;
                }
                break;
            case '-':
                chpid= fork();
                pid = getpid();
                ppid = getppid();
                if (chpid==0) {
                    printf("PRODUCER: My pid = %d, my ppid = %d \n", (int)pid, (int)ppid);
                    tryread(queue, izvl, qaccess);
                    return 0;
                }
                else {
                    //waitpid(chpid,NULL,0);
                    printf("PARENT: My pid = %d, my ppid = %d my child[%d] pid = %d\n", (int)pid, (int)ppid,n,(int)chpid);
                    n++;
                }
                break;

            case 'q':
                signal(SIGTERM,SIG_IGN);
                kill(0,SIGTERM);
                signal(SIGTERM,SIG_IGN);
                signal(SIGTERM,SIG_DFL);
                for(int i=0;i<n;i++) chpid=0;
                printf("PARENT: My pid = %d, my ppid = %d\nAll childs were killed,quiting...\n", (int)pid, (int)ppid);
                return 0;
                break;
            default:
                break;
        }
    }

}