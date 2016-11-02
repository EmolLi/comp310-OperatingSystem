#include "common.h"
#include <unistd.h>

int fd;
int errno;
Shared* shared_mem;

int setup_shared_memory(){
    fd = shm_open(MY_SHM, O_CREAT | O_RDWR, S_IRWXU);
    if(fd == -1){
        printf("shm_open() failed\n");
        exit(1);
    }
    ftruncate(fd, sizeof(Shared));
    return 0;
}

int attach_shared_memory(){
    shared_mem = (Shared*)  mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shared_mem == MAP_FAILED){
        printf("mmap() failed\n");
        exit(1);
    }

    return 0;
}

int init_shared_memory() {

	shared_mem->nextJobID=1;
	shared_mem->rear=NULL;
	shared_mem->front=NULL;
	sem_init(&(shared_mem->empty), 1, SLOTSIZE);
    sem_init(&(shared_mem->full),1, 0);
    sem_init(&(shared_mem->binary), 1, 1);
	//sem_init(&(shared_mem->binary))
    return 0;
}

/**
 * get job from the job list.
 * if there is no job in the job list, the printer will go to sleep until there is a job available
 */

void take_a_job(Job** job){
	sem_wait(&shared_mem->full);
	sem_wait(&shared_mem->binary);
	*job = Dequeue(shared_mem);
	sem_post(&shared_mem->binary);
}

int main() {
	int sem;
	Job* job = (Job*)malloc(sizeof(Job));

    setup_shared_memory();
    attach_shared_memory();

    init_shared_memory();
    sem_getvalue(&(shared_mem->full), &sem);

    Enqueue(3,shared_mem,1);
    sem_post(&shared_mem->full);
    sem_getvalue(&(shared_mem->full), &sem);
    Enqueue(4,shared_mem,2);
    sem_post(&shared_mem->full);
    sem_getvalue(&(shared_mem->full), &sem);

    take_a_job(&job);
    sem_getvalue(&(shared_mem->full), &sem);
    take_a_job(&job);
    sem_getvalue(&(shared_mem->full), &sem);
    take_a_job(&job);
    sem_getvalue(&(shared_mem->full), &sem);


    //free Job*
    /**
    Job* a = Dequeue(shared_mem);
    Job* b = Dequeue(shared_mem);
    Job* c = Dequeue(shared_mem);
    /**
    while (1) {
        //sem_wait(&shared_mem->full);
        printf("Now data is %d\n", shared_mem->data);
        //shared_mem->data = shared_mem->data + 1;
        //sleep(1);
        //shared_mem->data = shared_mem->data * 2;
        //printf("to %d\n", shared_mem->data);
        //sem_post(&shared_mem->resource);
        wait(&shared_mem->binary);

        sleep(1);
    }**/

    return 0;
}

