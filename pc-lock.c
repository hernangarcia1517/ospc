#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"
#include <semaphore.h>

int max;
int loops;
int *buffer;

int use_ptr  = 0;
int fill_ptr = 0;
int num_full = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

int consumers = 1;
int verbose = 1;


void do_fill(int value) {
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % max;
    num_full++;
}

int do_get() {
    int tmp = buffer[use_ptr];
    use_ptr = (use_ptr + 1) % max;
    num_full--;
    return tmp;
}

void *producer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
        Mutex_lock(&m);            // p1
        if (num_full < max)    // p2
          do_fill(i);                // p4
        Mutex_unlock(&m);          // p6
    }

    // end case: put an end-of-production marker (-1)
    // into shared buffer, one per consumer
    for (i = 0; i < consumers; i++) {
        Mutex_lock(&m);
//       printf("Producer got lock\n");
        if (num_full < max)
          do_fill(-1);
        Mutex_unlock(&m);
    }

    return NULL;
}


void *consumer(void *arg) {
    int tmp = 0;
    long long int cons = (long long int) arg;
    // consumer: keep pulling data out of shared buffer
    // until you receive a -1 (end-of-production marker)
    while (tmp != -1) {
        Mutex_lock(&m);           // c1
//        printf("Consumer %lld got lock\n", cons);
        if (num_full > 0)     // c2
          tmp = do_get();           // c4
        Mutex_unlock(&m);         // c6
    }
    return NULL;
}

int
main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <buffersize> <loops> <consumers>\n", argv[0]);
        exit(1);
    }
    max = atoi(argv[1]);
    loops = atoi(argv[2]);
    consumers = atoi(argv[3]);

    buffer = (int *) malloc(max * sizeof(int));
    assert(buffer != NULL);

    int i;
    for (i = 0; i < max; i++) {
        buffer[i] = 0;
    }

    pthread_t pid, cid[consumers];
    Pthread_create(&pid, NULL, producer, NULL);
    for (i = 0; i < consumers; i++) {
        Pthread_create(&cid[i], NULL, consumer, (void *) (long long int) i);
    }
    Pthread_join(pid, NULL);
    for (i = 0; i < consumers; i++) {
        Pthread_join(cid[i], NULL);
    }
    return 0;
}

