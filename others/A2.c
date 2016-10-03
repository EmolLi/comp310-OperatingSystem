#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>


static sem_t rw_mutex;
static sem_t mutex;
static int read_count = 0;
static int glob = 0;


static double writer_wait_time[10];
static double reader_wait_time[100];
static int nth_writer = 0;
static int nth_reader = 0;


static void *writer_thread_Func(void *arg) {

  int loops = *((int *) arg);
  int loc, j;

  double time_spent = 0;

  for (j = 0; j < loops; j++) {

    clock_t begin, end;
    begin = clock();

    if (sem_wait(&rw_mutex) == -1) exit(2); // wait for the value to be 1 or more, then consumes 1

    end = clock();
    time_spent += (double)(end - begin) * 1000 / CLOCKS_PER_SEC;

    // === writing ===

    loc = glob;
    loc += 10;
    glob = loc;

    // === writing ===
    if (sem_post(&rw_mutex) == -1) exit(2); // increase sem by 1 immediately
  }

  writer_wait_time[nth_writer] = time_spent;
  nth_writer ++ ;
  return NULL;
}

static void *reader_thread_Func(void *arg) {

  int loops = *((int *) arg);
  int loc, j;

  double time_spent = 0;

  for (j = 0; j < loops; j++) {

    clock_t begin, end;
    begin = clock();

    // === reading ===

    sem_wait(&mutex);
    read_count++;
    if (read_count == 1) sem_wait(&rw_mutex);
    end = clock();
    time_spent += (double)(end - begin) * 1000 / CLOCKS_PER_SEC;
    sem_post(&mutex);

    // === reading ===
    sem_wait(&mutex);
    read_count--;
    if (read_count == 0) sem_post(&rw_mutex);
    sem_post(&mutex);

  }

  reader_wait_time[nth_reader] = time_spent;
  nth_reader ++ ;

  return NULL;
}


void analyze(char *type, double *arr, int size){
  printf("\n===== Analyzing %s threads =====\n", type);
  int i;
  double min = 0;
  double max = 0;
  double sum = 0;
  for (i=0; i<size; i++){
    // sum
    sum += arr[i];
    // max
    if (arr[i] >= max) max = arr[i];
    // min
    if (min == 0) min = arr[0];
    if (arr[i] <= min) min = arr[i];
  }
  double avg = sum / size;

  printf("Maximum waiting time: %f milliseconds.\n", max);
  printf("Minimum waiting time: %f milliseconds.\n", min);
  printf("Average waiting time: %f milliseconds.\n", avg);
}



int main(int argc, char *argv[]) {

  pthread_t writer_threads[10];
  pthread_t reader_threads[100];

  int loops = 5000;

  // initialize semaphores
  if (sem_init(&rw_mutex, 0, 1) == -1) {
    printf("Error, init semaphore\n");
    exit(1);
  }

  if (sem_init(&mutex, 0, 1) == -1) {
    printf("Error, init semaphore\n");
    exit(1);
  }

  // create threads
  int i;
  for (i=0; i<10; i++){
    if (pthread_create(&(writer_threads[i]), NULL, writer_thread_Func, &loops) != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  for (i=0; i<100; i++){
    if (pthread_create(&(reader_threads[i]), NULL, reader_thread_Func, &loops) != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }
  }

  // free up thread resouces when they are done
  for (i=0; i<10; i++){
    if (pthread_join(writer_threads[i], NULL) != 0) {
      printf("Error, joining threads\n");
      exit(1);
    }
  }

  for (i=0; i<100; i++){
    if (pthread_join(reader_threads[i], NULL) != 0) {
      printf("Error, joining threads\n");
      exit(1);
    } 
  }

  printf("glob value %d \n", glob);

  analyze("writer", writer_wait_time, 10);
  analyze("reader", reader_wait_time, 100);

  exit(0);

}





