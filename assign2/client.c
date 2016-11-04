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
    if (shared_mem->nextClientID == 0){
        //shared_mem not initialized with proper value now.
        //may happen when the user opens a client before he enters the buffer size for the first printer
        printf("Buffer initialization not finished yet!\nPlease first enter the buffer size in the first printer.\n");
        printf("Exiting.\n");
        if(munmap(shared_mem, sizeof(Shared))!=0){
            printf("munmap() failed\n");
            exit(1);
        }
        exit(0);

    }

    sem_wait(&shared_mem->idUpdate);
	clientID = shared_mem->nextClientID;
	shared_mem->nextClientID += 1;
	shared_mem->running += 1;
    sem_post(&shared_mem->idUpdate);
	printf("Client %d is running now.\n", clientID);

}


void put_a_job(int numOfPage){
	int a;
	sem_getvalue(&shared_mem->empty, &a);
	sem_wait(&shared_mem->empty);
	sem_wait(&shared_mem->binary);
	Enqueue(numOfPage, shared_mem);
	sem_post(&shared_mem->binary);
	sem_post(&shared_mem->full);
	printf("Client %d has %d pages to print, puts request in Buffer.\n", clientID, numOfPage);
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



int main(int argc, char *argv[]) {

	if(signal(SIGINT, handler) == SIG_ERR)
        printf("Signal Handler Failure ..\n");

	if(argc == 1){
		printf("Argument missing. \n");
		printf("Please enter something like: ./client 3");
		printf("It means you want print 3 pages");
		exit(1);
	}
	if(argc>2){
		printf("Too many arguments. \n");
		printf("Please enter something like: ./client 3");
		printf("It means you want print 3 pages");
		exit(1);
	}

    setup_shared_memory();
    attach_shared_memory();
    Shared* shared_mem2 = shared_mem;
    setUpClient();

    int numOfPage = atoi(argv[1]);
    if (numOfPage<=0){
        printf("Invalid input!\nPage number should be a positive integer!\n");
        printf("Exiting\n");
        release_shared_mem(shared_mem);
        exit(1);
    }
    put_a_job(numOfPage);
    release_shared_mem(shared_mem);



    return 0;
}

