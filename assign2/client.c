#include "common.h"

int fd;
Shared* shared_mem;
int clientID;

int setup_shared_memory(){
    fd = shm_open(MY_SHM, O_RDWR, S_IRWXU);
    if(fd == -1){
        printf("shm_open() failed\n");
        exit(1);
    }
    return 0;
}

int attach_shared_memory(){
    shared_mem = (Shared*) mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shared_mem == MAP_FAILED){
        printf("mmap() failed\n");
        exit(1);
    }


    return 0;
}

void setUpClient(){
	clientID = shared_mem->nextClientID;
	shared_mem->nextClientID += 1;
	shared_mem->running += 1;
	printf("Client %d is running now.\n", clientID);

}

int get_job(){
	//get job params
	char str[10];
	printf("Enter the number of pages you want to print:  ");
	fgets(str, 10, stdin);
	return atoi(str);

}

void put_a_job(int numOfPage){
	sem_wait(&shared_mem->empty);
	sem_wait(&shared_mem->binary);
	Enqueue(numOfPage, shared_mem);
	sem_post(&shared_mem->binary);
	sem_post(&shared_mem->full);
}


void handler(int signo){
    int temp;
    sem_getvalue(&shared_mem->binary, &temp);
    if(temp != 1)
        sem_post(&shared_mem->binary);
    /**
    sem_getvalue(&shared_mem->resource, &temp);
    if(temp != 1)
        sem_post(&shared_mem->resource);
        **/
    exit(0);
}
/**
void release_shared_mem(){
	//just unmap, for last printer, also unlink
	shared_mem->running -= 1;
	if(munmap(shared_mem, sizeof(Shared))!=0){
	        printf("munmap() failed\n");
	        exit(1);
	    }

	if (shared_mem->running == 0){
		if(shm_unlink(MY_SHM)!=0){
			printf("shm_unlink failed\n");
			exit(1);
		}
	}
}
**/
int main() {

	if(signal(SIGINT, handler) == SIG_ERR)
        printf("Signal Handler Failure ..\n");

    setup_shared_memory();
    attach_shared_memory();
    Shared* shared_mem2 = shared_mem;
    setUpClient();

    int numOfPage = get_job();
    put_a_job(numOfPage);
    release_shared_mem(shared_mem);



    return 0;
}

