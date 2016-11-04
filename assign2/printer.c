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

int init_shared_memory(int capacity) {

	shared_mem->nextClientID = 1;
	shared_mem->nextPrinterID = 1;
	shared_mem->rear = 0;
	shared_mem->front = 0;
	shared_mem->capacity = capacity;
	shared_mem->running = 0;
	sem_init(&(shared_mem->empty), 1, capacity);
    sem_init(&(shared_mem->full),1, 0);
    sem_init(&(shared_mem->binary), 1, 1);
    sem_init(&(shared_mem->idUpdate), 1, 1);

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
	int sleep_time = (int)(page * 0.2);
	sleep(sleep_time);
	printf("Printer %d finishes printing %d pages.\n\n", printerID, page);
	//free memory
}

void setup_printer(){
    
    if (shared_mem->nextPrinterID == 0){
        //shared_mem not initialized with proper value now.
        //may happen when the user opens a second printer before he enters the buffer size for the first printer
        printf("Buffer initialization not finished yet! \nPlease first enter the buffer size in the first printer.\n");
        printf("Exiting.\n");
        if(munmap(shared_mem, sizeof(Shared))!=0){
            printf("munmap() failed\n");
            exit(1);
        }
        exit(0);

    }
    sem_wait(&shared_mem->idUpdate);
	printerID = shared_mem->nextPrinterID;
	shared_mem->nextPrinterID += 1;
	shared_mem->running += 1;
    sem_post(&shared_mem->idUpdate);
    printf("printer %d is running now.\n", printerID);
}

void handler(int signo){
    /**
	int temp;
    sem_getvalue(&shared_mem->binary, &temp);
    if(temp != 1)
        sem_post(&shared_mem->binary);

    sem_getvalue(&shared_mem->resource, &temp);
    if(temp != 1)
        sem_post(&shared_mem->resource);
        **/
	release_shared_mem(shared_mem);
    exit(0);
}

int getSize(){
    int size;
    printf("Please enter the buffer size: \n");

    if (scanf("%d", &size)!= 1 || size<=0 ||size>1000){
        printf("Invalid input!\nPlease enter an integer between 1 and 1000!\n");
        printf("Exiting.\n");
        if(shm_unlink(MY_SHM)!=0){
            printf("shm_unlink failed\n");
            exit(1);
        }
        exit(0);
    }
    return size;
}

int main() {
	int first;
	if(signal(SIGINT, handler) == SIG_ERR)
	        printf("Signal Handler Failure ..\n");

	fd = shm_open(MY_SHM, O_RDWR, S_IRWXU);
	first = fd;
    if (first == -1) {
    	setup_shared_memory();
    }
    attach_shared_memory();
    Shared* shared2 = shared_mem;

    if (first == -1){
    	int capacity = getSize();
    	init_shared_memory(capacity);
    }
    setup_printer();
    int a;
    sem_getvalue(&shared_mem->empty, &a);



    while(1){

    	int pageToPrint = take_a_job();
    	print_a_message(pageToPrint);
    	go_sleep(pageToPrint);
    }

    return 0;
}

