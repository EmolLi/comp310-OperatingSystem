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
#define SLOTSIZE 20


typedef struct {
    sem_t binary;
    sem_t empty;
    sem_t full;

    int Job[1000];
    int front;
    int rear;

    int capacity;
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

#endif //_INCLUDE_COMMON_H_

