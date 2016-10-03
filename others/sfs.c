#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "disk_emu.c"


// system settings
#define FILE_DESCRIPTOR_TABLE_LENGTH 1000
#define MY_BLOCK_SIZE 512
#define MY_NUM_BLOCK 1000
#define MY_FIELD_SIZE sizeof(int)
#define MY_MAX_FILE_BLOCK (12 + MY_NUM_BLOCK / MY_FIELD_SIZE)

// partition settings
#define SUPER_BLOCK_LENGTH 1
#define INODE_TABLE_LENGTH 200
#define DIRECTORY_TABLE_LENGTH 100
#define DATA_BLOCK_LENGTH 600
#define FREE_BITMAP_LENGTH 99

// starting index of each partition
#define SUPER_BLOCK_INDEX 0
#define INODE_TABLE_INDEX (SUPER_BLOCK_INDEX + SUPER_BLOCK_LENGTH)
#define DIRECTORY_TABLE_INDEX (INODE_TABLE_INDEX + INODE_TABLE_LENGTH)
#define DATA_BLOCK_INDEX (DIRECTORY_TABLE_INDEX + DIRECTORY_TABLE_LENGTH)
#define FREE_BITMAP_INDEX (DATA_BLOCK_INDEX + DATA_BLOCK_LENGTH)


// === Remainder ===
// data blocks are indexed by their absolute position within the entire system
// inode blocks are indexed by their relative position to i-node table's starting index
// the "offset" in sfs_fseek() must be a multiple of MY_BLOCK_SIZE because disk_emu API's write_blocks()
// can only access by blocks, and not by bytes.



// define super block
typedef struct super_block {
    int magic;
    int block_size;
    int file_system_size;
    int i_node_table_length;
} super_block;

// define directory
typedef struct filename_to_inode {
    int isInitialized;
    char *filename;
    int inode_index;
} filename_to_inode;

// define inode
typedef struct inode inode;
struct inode {
    int isInitialized;
    int size;
    int direct_block_used;
    int indirect_block_used;
    int direct_block[12];
    int indirect_block;
};

// single file descriptor, this is 1 per process
typedef struct file_descriptor {
    int isInitialized;
    int inode_index;
    int read_pointer;
    int write_pointer;
} file_descriptor;

// create file descriptor table that tracks all currently opened files
file_descriptor file_descriptor_table[FILE_DESCRIPTOR_TABLE_LENGTH];

// cached inode table and directory table from disk to memory
inode* cached_inode_table;
filename_to_inode* cached_directory_table;


// create file system
int mksfs(int fresh){

    // allocate disk space
    int result;
    char *filename = "Simple File System";
    int block_size = MY_BLOCK_SIZE;
    int num_blocks = MY_NUM_BLOCK;

    if (fresh == 1) {
        result = init_fresh_disk(filename, block_size, num_blocks);
    } else {
        result = init_disk(filename, block_size, num_blocks);
    }

    // create partition : super block
    super_block* sb = malloc(1 * sizeof(super_block));
    sb->magic = 0;
    sb->block_size = block_size;
    sb->file_system_size = num_blocks * block_size;
    sb->i_node_table_length = INODE_TABLE_LENGTH;
    write_blocks(SUPER_BLOCK_INDEX, SUPER_BLOCK_LENGTH, sb);
    printf("create partition (%i blocks): super block.\n", SUPER_BLOCK_LENGTH);

    // create partition : i-node table
    inode* it_buffer = malloc(INODE_TABLE_LENGTH * sizeof(inode));
    write_blocks(INODE_TABLE_INDEX, INODE_TABLE_LENGTH, it_buffer);
    printf("create partition (%i blocks): i-node table.\n", INODE_TABLE_LENGTH);

    // create partition : directory table, it stores filename->inode pairs
    filename_to_inode* dt_buffer = malloc(INODE_TABLE_LENGTH * sizeof(inode));
    write_blocks(DIRECTORY_TABLE_INDEX, DIRECTORY_TABLE_LENGTH, dt_buffer);
    printf("create partition (%i blocks): directory table.\n", DIRECTORY_TABLE_LENGTH);

    // create partition : data blocks
    printf("create partition (%i blocks): data blocks.\n", DATA_BLOCK_LENGTH);

    // rest : free bitmap
    printf("create partition (%i blocks): free bitmap.\n", FREE_BITMAP_LENGTH);

    // cache inode table to memory
    cached_inode_table = malloc(INODE_TABLE_LENGTH * block_size);
    read_blocks(INODE_TABLE_INDEX, INODE_TABLE_LENGTH, cached_inode_table);
    printf("caching inode table (%i blocks) to memory.\n", INODE_TABLE_LENGTH);

    // cache directory table into memory
    cached_directory_table = malloc(DIRECTORY_TABLE_LENGTH * block_size);
    read_blocks(DIRECTORY_TABLE_INDEX, DIRECTORY_TABLE_LENGTH, cached_directory_table);
    printf("caching directory table (%i blocks) to memory.\n", DIRECTORY_TABLE_LENGTH);

    return result;
}



// find the first available slot in a given table
int find_next_free_slot(file_descriptor *table) {
    int i;
    int n = FILE_DESCRIPTOR_TABLE_LENGTH;
    int free_position;
    for (i=0; i<n; i++){
        if (table[i].isInitialized != 1){
            free_position = i;
            return free_position;
        }
    }
    printf("File descriptor table is full.\n");
    return -1; // no available spot in table
}

// create an entry in file descriptor table
void make_file_desc_entry(int file_id, int inode_index, int read_pointer, int write_pointer) {
    file_descriptor_table[file_id].isInitialized = 1;
    file_descriptor_table[file_id].inode_index = inode_index;
    file_descriptor_table[file_id].read_pointer = read_pointer;
    file_descriptor_table[file_id].write_pointer = write_pointer;
    printf("File opened with ID %i. Read pointer: %i, write pointer: %i\n", file_id, read_pointer, write_pointer);
}

// create an inode and return its index in the inode table
int create_inode(){
    int i;
    for (i=0; i<INODE_TABLE_LENGTH; i++){
        if (cached_inode_table[i].isInitialized != 1){ // found an available spot in inode table
            cached_inode_table[i].isInitialized = 1;
            cached_inode_table[i].size = 0;
            cached_inode_table[i].direct_block_used = 0;
            cached_inode_table[i].indirect_block_used = 0;
            // printf("Created i-node at index %i.\n", i);
            return i;
        }
    }
    printf("i-node table is full.\n");
    return -1;
}

// return the inode's index given a filename
int get_inode(char *filename){
    int i = 0;
    filename_to_inode file;
    while ( (file = cached_directory_table[i]).filename != NULL ){
        if (strcmp(file.filename, filename) == 0 && file.isInitialized == 1) {
            printf("Found file \"%s\" with i-Node %i.\n", file.filename, file.inode_index);
            return file.inode_index;
        }
        i++;
    }
    printf("File \"%s\" does not exist.\n", filename);
    return -1; // no inode exists with such filename
}

int create_file(char *filename){

    // allocate and initialize an i-Node
    int inode_index = create_inode();
    if (inode_index == -1) return -1;

    // Write the mapping between the i-Node and file name in the root directory.
    int i;
    for (i=0; i<DIRECTORY_TABLE_LENGTH; i++){
        if (cached_directory_table[i].isInitialized != 1){
            cached_directory_table[i].isInitialized = 1;
            cached_directory_table[i].filename = filename;
            cached_directory_table[i].inode_index = inode_index;
            printf("Created file \"%s\" at root at inode %i.\n", cached_directory_table[i].filename, cached_directory_table[i].inode_index);

            // sync with the inode and directory table on disk
            write_blocks(INODE_TABLE_INDEX, INODE_TABLE_LENGTH, cached_inode_table);
            write_blocks(DIRECTORY_TABLE_INDEX, DIRECTORY_TABLE_LENGTH, cached_directory_table);

            return inode_index;
        }
    }
    printf("Directory table is full.\n");
    return -1;

}


int sfs_fopen(char *filename){
    // check if file exist
    int inode_index = get_inode(filename);
    if (inode_index == -1) { // if file doesn't exist, create new file
        inode_index = create_file(filename);
    }
    if (inode_index == -1) return -1;

    // if file is already opened, return its index in file descriptor table
    int i;
    for (i=0; i<FILE_DESCRIPTOR_TABLE_LENGTH; i++){
        if (file_descriptor_table[i].isInitialized == 1 && file_descriptor_table[i].inode_index == inode_index){
            printf("File already opened.\n");
            return i;
        }
    }
    int file_id = find_next_free_slot(file_descriptor_table);
    if (file_id == -1) return -1;
    int read_pointer  = 0;
    int write_pointer = cached_inode_table[inode_index].size;
    make_file_desc_entry(file_id, inode_index, read_pointer, write_pointer);
    return file_id;
}

int sfs_fclose(int file_id){
    file_descriptor_table[file_id].isInitialized = 0;
    file_descriptor_table[file_id].read_pointer = 0;
    printf("Closed file with ID %i.\n", file_id);
    return file_id;
}


// write buf characters into disk
int sfs_fwrite(int file_id, char *buffer, int length){

    // make sure the file is opened
    if (file_descriptor_table[file_id].isInitialized != 1) {
        printf("Write refused. File with ID %i is not opened yet.\n", file_id);
        return -1;
    }

    // make sure its inode exist
    int inode_index = file_descriptor_table[file_id].inode_index;
    if (inode_index < 0) return -1;

    // make sure its size is under the limit set by inode
    int blocks_needed = length / MY_BLOCK_SIZE + (length % MY_BLOCK_SIZE != 0) + 1; // take the ceiling, and + 1 for index block
    if (cached_inode_table[inode_index].direct_block_used + cached_inode_table[inode_index].indirect_block_used + blocks_needed - 1 > MY_MAX_FILE_BLOCK){
        printf("Write refused. Buffer exceeds max file size.\n");
        return -1;
    }

    // update file on disk
    // scan disk blocks one by one, searching for a contiguous space that's big enough to contain our data
    // (too lazy to take advantage of i-node's flexible data allocation, just gonna use contiguous for both direct block + index block)
    int block_address;
    int writable_block_num = 0;
    int blocks_written = -1;
    int start_index, end_index;
    for (block_address = DATA_BLOCK_INDEX; block_address < DATA_BLOCK_INDEX + DATA_BLOCK_LENGTH; block_address ++){
        char *block_data = malloc(1 * MY_BLOCK_SIZE);
        read_blocks(block_address, 1, block_data);
        if (strcmp(block_data, "") == 0){ // once we encounter a empty block, we start counting
            writable_block_num ++ ;
        } else { // if we encounter a non-empty block, we reset counter
            writable_block_num = 0;
        }
        if (writable_block_num == blocks_needed){ // once that block is big enough, we'll use that to store data. +1 for index bock
            start_index = block_address - blocks_needed + 1;
            end_index = block_address; // address of the index block
            blocks_written = write_blocks(start_index, blocks_needed - 1, buffer);
            break;
        }
    }

    if (blocks_written <= 0) {
        printf("No space left on disk.\n");
        return blocks_written;
    }

    // at this line, data blocks are written to disk. We need to link them to the file's i-node.
    printf("Data are written to disk from block %i to block %i.\n", start_index, start_index + blocks_needed - 1);

    // find which block field the we start and which field we end on
    int starting_field = file_descriptor_table[file_id].write_pointer / MY_BLOCK_SIZE + (file_descriptor_table[file_id].write_pointer % MY_BLOCK_SIZE != 0);
    int ending_field = starting_field + blocks_written - 1;

    // start linking
    int i;
    int block_index = start_index; // position of data block

    // direct blocks
    int direct_block_num = ending_field > 11 ? 11 : ending_field + 1;
    for (i=starting_field; i<=11 && i<=ending_field; i++){
        cached_inode_table[inode_index].direct_block[i] = block_index;
        block_index ++ ;
    }

    // link indirect block
    int indirect_block_num = ending_field > 11 ? ending_field - 11 : 0;
    int index_block = end_index; // index block is located at the end of all data blocks of this file
    if (indirect_block_num >= 1){
        cached_inode_table[inode_index].indirect_block = index_block;
        int indirect_blocks_addresses[indirect_block_num];
        for (i=0; i<indirect_block_num; i++){
            indirect_blocks_addresses[i] = block_index;
            block_index ++ ;
        }
        write_blocks(index_block, 1, indirect_blocks_addresses);
    }

    cached_inode_table[inode_index].direct_block_used = direct_block_num;
    cached_inode_table[inode_index].indirect_block_used = indirect_block_num;
    printf("File contains %i direct blocks and %i indirect blocks.\n", direct_block_num, indirect_block_num);

    // we update the file's write pointer
    file_descriptor_table[file_id].write_pointer = starting_field * MY_BLOCK_SIZE + length;

    // update file size
    int old_size = cached_inode_table[inode_index].size;
    int write_ptr = file_descriptor_table[file_id].write_pointer;
    int new_size = old_size > write_ptr ? old_size : write_ptr;
    cached_inode_table[inode_index].size = new_size;
    printf("New file size: %i, old file size: %i.\n", new_size, old_size);

    // save modified i-node to disk
    write_blocks(INODE_TABLE_INDEX, INODE_TABLE_LENGTH, cached_inode_table);

    return blocks_written;
}


// read characters from disk into buffer
int sfs_fread(int file_id, char *buffer, int length){

    // if file is closed, forbid access
    if (file_descriptor_table[file_id].isInitialized != 1) return -1;

    // analyze inode's data block allocation and reconsistute original data
    int inode_index = file_descriptor_table[file_id].inode_index;
    int i;
    for (i=0; i<12; i++){ // direct blocks
        int address = cached_inode_table[inode_index].direct_block[i];
        char *tmp_buffer = malloc(1 * MY_BLOCK_SIZE);
        if (address != 0){
            read_blocks(address, 1, tmp_buffer); // fetch data
            strcat(buffer, tmp_buffer); // append it to buffer
            file_descriptor_table[file_id].read_pointer += strlen(tmp_buffer); // update read pointer
        }
        else {
            break;
        }
    }
    return file_descriptor_table[file_id].read_pointer;

}

// the offset MUST be multiple of MY_BLOCK_SIZE
int sfs_fseek(int file_id, int offset){
    file_descriptor_table[file_id].read_pointer += offset;
    file_descriptor_table[file_id].write_pointer += offset;
    int new_pointer = file_descriptor_table[file_id].write_pointer;
    printf("Moved file (ID: %i)'s read pointer and write pointer to %i.\n", file_id, new_pointer);
    return new_pointer;
}

int sfs_remove(char *filename){

    // remove the file from the directory entry
    int i;
    for (i=0; i<DIRECTORY_TABLE_LENGTH; i++){
        if (cached_directory_table[i].filename != NULL && strcmp(cached_directory_table[i].filename, filename) == 0 && cached_directory_table[i].isInitialized == 1){
            cached_directory_table[i].isInitialized = 0;
            cached_directory_table[i].filename = NULL;
            write_blocks(DIRECTORY_TABLE_INDEX, DIRECTORY_TABLE_LENGTH, cached_directory_table);

            // release the data blocks and overwrite them with 0's
            int inode_index = cached_directory_table[i].inode_index;
            int total_block_used = cached_inode_table[inode_index].direct_block_used + cached_inode_table[inode_index].indirect_block_used;
            if (total_block_used >= 1){
                // we do not need to check for each individual data block address because they are continugous
                // as we've defined it in sfs_fwrite()
                int data_block_address = cached_inode_table[inode_index].direct_block[0];
                for (i=0; i<total_block_used + 1; i++){ // +1 for index block
                    char *stream_of_zeros = calloc(1, MY_BLOCK_SIZE);
                    write_blocks(data_block_address, 1, stream_of_zeros);
                    data_block_address ++ ;
                }
            }

            // reset i-node
            cached_inode_table[inode_index].isInitialized = 0;
            cached_inode_table[inode_index].size = 0;
            cached_inode_table[inode_index].direct_block_used = 0;
            cached_inode_table[inode_index].indirect_block_used = 0;
            for (i=0; i<12; i++) cached_inode_table[inode_index].direct_block[i] = 0;
            cached_inode_table[inode_index].indirect_block = 0;
            write_blocks(INODE_TABLE_INDEX, INODE_TABLE_LENGTH, cached_inode_table);

            printf("Deleted file \"%s\".\n", filename);
            return 1;
        }
    }

    printf("Unable to find file \"%s\".\n", filename);
    return 0;
}


int position_in_root;
int sfs_get_next_filename(char* filename){
    int i;
    for (i=position_in_root; i<DIRECTORY_TABLE_LENGTH; i++){
        if (cached_directory_table[i].filename != NULL && cached_directory_table[i].isInitialized == 1){
            strcpy(filename, cached_directory_table[i].filename);
            position_in_root ++ ;
            printf("Next file in the directory is %s.\n", filename);
            return 1;
        }
    }
    printf("No more file in this directory.\n");
    return 0;
}


// since our simple file system has only 1 directory, which is the root, then it's useless
// to input a path such as "/root/filename", just put a normal filename instead
int sfs_GetFileSize(char* filename){
    int inode_index = get_inode(filename);
    if (inode_index < 0) return -1;
    int size = cached_inode_table[inode_index].size;
    printf("File \"%s\" has size %i.\n", filename, size);
    return cached_inode_table[inode_index].size;
}



int main(){

    // below are test for our file system API. Logs will appear on console.

    printf("\n\n ==== Create file system ==== \n");
    mksfs(1);

    printf("\n\n ==== Open a non-existing file called 'a'==== \n");
    int id = sfs_fopen("a");

    printf("\n\n ==== Write some data on our file 'a' ==== \n");
    sfs_fwrite(id, "IM NOT WEARING ANY PANTS", 24);

    printf("\n\n ==== Read the content of file 'a' from disk ==== \n");
    char *b = malloc(10 * MY_BLOCK_SIZE);
    sfs_fread(id, b, 10);
    printf("data is: %s\n", b);

    printf("\n\n ==== Get size of file 'a' ==== \n");
    sfs_GetFileSize("a");

    printf("\n\n ==== Close file 'a' ==== \n");
    sfs_fopen("a");

    printf("\n\n ==== To show get_next_filename() works, we create 3 other files, for a total of 4 files ====\n ==== in the directory now, then we call the function 5 times, so the last one should indicate invalid ==== \n");
    sfs_fopen("b");
    sfs_fopen("c");
    sfs_fopen("d");
    char *f = malloc(1 * MY_BLOCK_SIZE);
    sfs_get_next_filename(f);
    sfs_get_next_filename(f);
    sfs_get_next_filename(f);
    sfs_get_next_filename(f);
    sfs_get_next_filename(f);

    printf("\n\n ==== Remove file 'a' ==== \n");
    sfs_remove("a");

    return 0;
}


