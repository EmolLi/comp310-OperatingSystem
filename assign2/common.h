#ifndef _INCLUDE_COMMON_H_
#define _INCLUDE_COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// from `man shm_open`
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <signal.h>
#include <semaphore.h>

#define MY_SHM "/JIT"


typedef struct {
    sem_t binary;
    sem_t empty;
    sem_t full;

    sem_t idUpdate;
    

    int Job[1000];
    int front;
    int rear;

    int capacity;
    int running;
    int nextClientID;
    int nextPrinterID;
} Shared;


void Enqueue(int numOfpage, Shared* shared_mem){
	(shared_mem->rear) = (shared_mem->rear)%(shared_mem->capacity);
	int index = shared_mem->rear;
	(shared_mem->Job)[index] = numOfpage;
	shared_mem->rear += 1;


}

//return the first job in the queue
int Dequeue(Shared* shared_mem){
	int front = shared_mem->front;
	int page = (shared_mem->Job)[front];
	front += 1;
	front = front % (shared_mem->capacity);
	shared_mem->front = front;
	return page;
}


void release_shared_mem(Shared* shared_mem){
	//just unmap, for last printer, also unlink
	shared_mem->running -= 1;
	int running = shared_mem->running;
	if(munmap(shared_mem, sizeof(Shared))!=0){
	        printf("munmap() failed\n");
	        exit(1);
	    }

	if (running <= 0){
		if(shm_unlink(MY_SHM)!=0){
			printf("shm_unlink failed\n");
			exit(1);
		}
	}
}
#endif //_INCLUDE_COMMON_H_

