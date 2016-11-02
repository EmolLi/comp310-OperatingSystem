#include "common.h"
#include <unistd.h>

int fd;
int errno;
int printerID;
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

	shared_mem->nextClientID = 1;
	shared_mem->nextPrinterID = 1;
	shared_mem->rear = 0;
	shared_mem->front = 0;
	shared_mem->capacity = SLOTSIZE;
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




int take_a_job(){
	sem_wait(&shared_mem->full);
	sem_wait(&shared_mem->binary);

	int pageToPrint = Dequeue(shared_mem);

	sem_post(&shared_mem->binary);
	sem_post(&shared_mem->empty);

	return pageToPrint;
}

void print_a_message(int page){
	printf("Printer %d starts printing %d pages.\n\n", printerID, page);
}

void go_sleep(int page){
	int sleep_time = (int)(page * 0.5);
	sleep(sleep_time);
	printf("Printer %d finishes printing %d pages.\n\n", printerID, page);
	//free memory
}


int main() {
	int sem;

    setup_shared_memory();
    attach_shared_memory();

    init_shared_memory();

    printerID = shared_mem->nextPrinterID;
    shared_mem->nextPrinterID+=1;

    sem_getvalue(&(shared_mem->full), &sem);


    //==========setting up test value==================
    Enqueue(3,shared_mem);
    sem_post(&shared_mem->full);
    sem_getvalue(&(shared_mem->full), &sem);
    Enqueue(4,shared_mem);
    sem_post(&shared_mem->full);
    sem_getvalue(&(shared_mem->full), &sem);

    //while(1){};
/**
    while(1){

    	take_a_job(&job);
    	sem_getvalue(&(shared_mem->full), &sem);


    	print_a_message(job);

    	go_sleep(job);
    }
**/
    int job1 = take_a_job();
    sem_getvalue(&(shared_mem->full), &sem);
    int job2 = take_a_job();
    sem_getvalue(&(shared_mem->full), &sem);


    return 0;
}

