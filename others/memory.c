#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


// error message
char my_malloc_error[40];

// starting and ending address of the memory we are managing
void *memory_start, *memory_end;

// malloc initialized, default to 0 (false)
int initialized;

// malloc search policy
int policy;
#define FIRST_FIT 0
#define BEST_FIT 1

// starting address of free list (head of doubly linked list)
void *free_list_start;
int free_list_initialized;

// Called at the very first time of malloc, setting up important parameters
void init(){

	// Address at which our heap implementation starts from
    memory_start = sbrk(0);

    // We don't have any memory to manage atm, so put it as same as start
    memory_end = memory_start;

    // free list: empty doubly linked list
    free_list_start = memory_start;

    // Mark setup as finished
    initialized = 1;
}


// Memory control block
typedef struct block {
	void *prev;
	void *next;
	int size;
    int is_free;
} Block;


// choose malloc policy
void my_mallopt(int p) {
	policy = p;
	printf("Policy chosen: %s\n", p == 0 ? "First-fit" : "Best-fit");
}


void *my_malloc(int size) {

	// prevent trolling
    if (size <= 0) {
    	strcpy(my_malloc_error, "Error! Size can not be negative.");
    	printf("%s.\n", my_malloc_error);
    	return NULL;
    }

    // initialize if haven't done so
    if (initialized != 1) init();

    // create our memory control block
    Block *block;

    // the memory needed is not simply the content, but actually includes its associated control block
    int byte_needed = size + sizeof(Block);

    // return value, containing the address of the available block if found, else 0
    void *available_address = 0;

    // track current address we are at during search
    void *current_address;

    // search in the free list (FIRST-FIT method)
    if (policy == FIRST_FIT){

    	// starting from the head of free list
    	current_address = free_list_start;

    	// if found an available block
	    while ( free_list_initialized == 1 && (block = (Block*) current_address) && block->is_free == 1 && block->size >= byte_needed) {

    		// if it exactly fits, mark it as no longer available
    		block->is_free = 0;

    		// if there are surplus, split it into 2 blocks: a in-use block and a free block
    		int new_free_block_size = block->size - byte_needed * 2;
    		if (new_free_block_size > 0){
    			block->size = byte_needed;

    			// adjust pointer to put new free block in the middle of linked list
    			void *new_free_block_address = current_address + byte_needed;
    			Block *new_free_block = (Block*) new_free_block_address;

    			// double link with previous block
    			((Block*) (block->prev))->next = new_free_block_address;
    			new_free_block->prev = (void *)(block->prev);

    			// double link with next block
    			((Block*) (block->next))->prev = new_free_block_address;
    			new_free_block->next = (void *)(block->next);

    			// set parameters
    			new_free_block->size = new_free_block_size;
    			new_free_block->is_free = 1;

    			printf("Block split into an occupied block of %i bytes and a free block of %i bytes", byte_needed, new_free_block_size);
    		}

	    	// take it like a bawss
    		available_address = current_address;

    		printf("Free block of size %i found at address %p. New content takes up %i bytes.\n", block->size, available_address, byte_needed);
    		break;

	    	current_address = block->next;
	    }
	}

	// search in the free list (BEST-FIT method)
	else {
		// starting from the head of free list
    	current_address = free_list_start;

    	void *best_block_address;
    	int best_block_size = 32767; // max value for an int

    	// if found an available block
	    while ( free_list_initialized == 1 && (block = (Block*) current_address) && block->is_free == 1 && block->size >= byte_needed) {

	    	// choose the smallest block
	    	if (block->size < best_block_size){
	    		best_block_address = current_address;
	    		best_block_size = block->size;
	    	}

	    	current_address = block->next;
	    }

	    if (best_block_size != 32767){

		    // take the best fit block
		    block = (Block*) best_block_address;
		    current_address = best_block_address;

		    // take it like a bawss
	    	available_address = current_address;
	    	printf("Free block of size %i found at address %p. New content takes up %i bytes.\n", block->size, available_address, byte_needed);
	    		

		    // if it exactly fits, mark it as no longer available
			block->is_free = 0;

			// if there are surplus, split it into 2 blocks: a in-use block and a free block
			int new_free_block_size = block->size - byte_needed * 2;
			if (new_free_block_size > 0){
				block->size = byte_needed;

				// adjust pointer to put new free block in the middle of linked list
				void *new_free_block_address = current_address + byte_needed;
				Block *new_free_block = (Block*) new_free_block_address;

				// double link with previous block
				((Block*) (block->prev))->next = new_free_block_address;
				new_free_block->prev = (void *)(block->prev);

				// double link with next block
				((Block*) (block->next))->prev = new_free_block_address;
				new_free_block->next = (void *)(block->next);

				// set parameters
				new_free_block->size = new_free_block_size;
				new_free_block->is_free = 1;

				printf("Block split into an occupied block of %i bytes and a free block of %i bytes", byte_needed, new_free_block_size);
			}

		}


	}

    // if there's no available block in free list, we have to allocate memory on heap
    if (available_address == 0) {

    	// increase program break to fit our new block
    	sbrk(byte_needed);

    	// the starting address of our new block is the previous end of the heap
    	available_address = memory_end;

    	// take it like a bawss
		block = available_address;

		// mark it as no longer available
		block->is_free = 0;
		block->size = byte_needed;

    	// update the end of heap
    	memory_end += byte_needed;

    	printf("No free block available. Allocated new heap space at address %p for %i bytes.\n", available_address, byte_needed);
    }

    // the address of the block we just allocated points to our Block section
    // right after this section will be the address we store actual data on, we
    // want to return that

    // printf("Data section starts at address %p.\n", available_address + sizeof(Block));
    return available_address + sizeof(Block);


}

void my_free(void *data_address) {

	// prevent trolling
	if (data_address == NULL) return;

	free_list_initialized = 1;

	// point to the memory control section of the block (which is before its data section)
	void *block_address = data_address - sizeof(Block);
	Block *block = (Block*) block_address;

	int merge_performed = 0;

	// merge previous block if necessary
	Block *prev_block = (Block*) block->prev;
	if ( prev_block && prev_block->is_free == 1 ){
		prev_block->size = block->size + prev_block->size;
		block_address = block->prev; // need to update starting address of merged block
		block = (Block*) block_address;
		merge_performed = 1;
	}

	// merge next block if necessary
	Block *next_block = (Block*) block->next;
	if ( next_block && next_block->is_free == 1 ){
		next_block->size = block->size + next_block->size; // no need to update starting address since its after
		merge_performed = 1;
	}

	// if no merge is performed, we need to manually free it and put it on free list
	if (merge_performed == 0){
		// mark it as available
		block->is_free = 1;

		// put it on free list (at its head position for minimum computation)
		void *old_head = free_list_start;
		if (((Block*) old_head)->is_free == 1){
			block->next = old_head;
			((Block*) old_head)->prev = block;
		}
		free_list_start = block_address;

		// avoid self-loop
		if (block->next == block_address) block->next = NULL;
	}

	printf("Memory block %p is freed, %i bytes released, head of free list is now at %p.\n", block, block->size, free_list_start);

}


void my_mallinfo() {

	int total_bytes_allocated = 0;
	int total_free_space = 0;
	int largest_free_space = 0;

	// start from the beginning of heap
    void *current_address = memory_start;

	// loop through each memory block
	while (current_address != memory_end) {

    	Block *block = (Block*) current_address; // type cast to Block to view its content

    	// count total bytes allocated
    	if (block->is_free != 1) total_bytes_allocated += block->size;

    	// count free space
    	if (block->is_free == 1) total_free_space += block->size;

    	// largest contiguous occupied space
    	if (block->is_free == 1 && block->size > largest_free_space) largest_free_space = block->size;

    	current_address += block->size;
    }

    printf("====== Memory allocation statistics ======\n");
    printf("Total bytes allocated: %i\n", total_bytes_allocated);
    printf("Total free space: %i\n", total_free_space);
    printf("Largest contiguous free space: %i\n", largest_free_space);
}


void print_free_list() {

	Block *block;
	void* current_address = free_list_start;
    printf("FREE LIST: ", current_address);
    while ( (block = (Block*) current_address) && block->is_free == 1) {
    	printf(" ->%p", current_address);
    	current_address = block->next;
    }
    printf("\n");
}


int main(){

	// ===== Below are test for our malloc API. Logs will appear on console. =====

	// choose search policy
	my_mallopt(FIRST_FIT); // or BEST_FIT

	// malloc some string
    void *a = my_malloc(1);
   	void *b = my_malloc(2);
   	char *c = (char *) my_malloc(6);

   	// demonstrate malloc works, should display "hello"
   	strcpy(c, "hello");
   	printf("Malloc stored in c contains string: %s\n", c);

   	// error message
   	void *d = my_malloc(-3);

   	my_free(a);
   	my_free(b);
   	my_free(c);

   	// get memory statistics
   	my_mallinfo();

   	// display free list
	print_free_list();

	// first-fit will chooce c, and first-fit will chooses b
	my_malloc(2);

    return 0;

}