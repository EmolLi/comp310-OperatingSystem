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

//queue implemented by linked list
typedef struct{
	//int printerID;
	int clientID;
	int jobID;
	int pageToPrint;
	struct Job* next;
} Job;



typedef struct {
    //sem_t resource;
    sem_t binary;
    sem_t empty;
    sem_t full;

    //for queue
    Job* front;
    Job* rear;

    //int capacity;
    int nextJobID;
    int nextClientID;
    int nextPrinterID;
    //int data;
    //int readcount;
} Shared;


void Enqueue(int numOfpage, Shared* shared_mem, int clientID){
	Job* temp = (Job*) malloc(sizeof(Job));

	temp->clientID = clientID;
	temp->jobID = shared_mem->nextJobID;
	shared_mem->nextJobID += 1;
	temp->pageToPrint = numOfpage;
	temp->next = NULL;

	Job* front = (Job*) (shared_mem->front);
	Job* rear = (Job*) (shared_mem->rear);

	if(front == NULL && rear == NULL){
		shared_mem->front = temp;
		shared_mem->rear = temp;
		return;
	}

	rear->next = (struct Job*)temp;
	shared_mem->rear = (Job*)temp;
	return;


}

//return the first job in the queue
Job* Dequeue(Shared* shared_mem){
	Job* temp = shared_mem->front;

	//This may be unnecessary
	if (temp == NULL){
		return NULL;
	}

	if(shared_mem->front == shared_mem->rear){
		shared_mem->front = NULL;
		shared_mem->rear = NULL;
	}
	else{
		shared_mem->front = (Job*)(temp->next);
	}
	return temp;
}

#endif //_INCLUDE_COMMON_H_

