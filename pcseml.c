#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "eventbuf.c"

struct eventbuf *event_buffer;

sem_t *producer_sem;
sem_t *consumer_sem;
sem_t *event_buffer_sem;

int events_per_prod_count;


sem_t *sem_open_temp(const char *name, int value) {
    sem_t *sem;

    if ((sem = sem_open(name, O_CREAT, 0600, value)) == SEM_FAILED)
        return SEM_FAILED;
        
    if (sem_unlink(name) == -1) {
        sem_close(sem);
        return SEM_FAILED;
    }
    return sem;
}

void *producer_func(int *pr_thread_id) {
    for (int i = 0; i < events_per_prod_count; i++) {
        int event_added = 100 * *pr_thread_id + i;
        sem_wait(producer_sem);
        sem_wait(event_buffer_sem);

        printf("P%d: adding event %d\n", *pr_thread_id, event_added);
        eventbuf_add(event_buffer, event_added);

        sem_post(event_buffer_sem);
        sem_post(consumer_sem);
    }
    printf("P%d: exiting\n", *pr_thread_id);
}

void *consumer_func(int *c_thread_id) {
    for (int i = 0; i < 50; i++) { // TODO potentially remove the i<50 case, that's just to prevent infinite loop
        sem_wait(consumer_sem);
        sem_wait(event_buffer_sem);

        if (eventbuf_empty(event_buffer)) {
            sem_post(event_buffer_sem);
            printf("C%d: exiting\n", *c_thread_id);
            return;
        }
        int event_gotten = eventbuf_get(event_buffer);
        printf("C%d: got event %d\n", *c_thread_id, event_gotten);

        sem_post(event_buffer_sem);
        sem_post(producer_sem);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("error: needs 4 int args -- producer_count, consumer_count, events_per_prod_count, queue_lim\n");
        exit(0);
    }
    int producer_count = atoi(argv[1]);
    int consumer_count = atoi(argv[2]);
    events_per_prod_count = atoi(argv[3]);
    int queue_lim = atoi(argv[4]);

    event_buffer = eventbuf_create();

    producer_sem = sem_open_temp("producer_semaphore", queue_lim);
    consumer_sem = sem_open_temp("consumer_semaphore", 0);
    event_buffer_sem = sem_open_temp("event_buffer_semaphore", 1);

    pthread_t *pr_thread = calloc(producer_count, sizeof *pr_thread);
    int *pr_thread_id = calloc(producer_count, sizeof *pr_thread_id);

    for (int i = 0; i < producer_count; i++) {
        pr_thread_id[i] = i;
        pthread_create(pr_thread + i, NULL, producer_func, pr_thread_id + i);
    }

    pthread_t *c_thread = calloc(consumer_count, sizeof *c_thread);
    int *c_thread_id = calloc(consumer_count, sizeof *c_thread_id);

    for (int i = 0; i < consumer_count; i++) {
        c_thread_id[i] = i;
        pthread_create(c_thread + i, NULL, consumer_func, c_thread_id + i);
    }

    for (int i = 0; i < producer_count; i++)
        pthread_join(pr_thread[i], NULL);

    // TODO: sem_unlink?
    
    for (int i = 0; i < consumer_count; i++)
        sem_post(consumer_sem);

    for (int i = 0; i < consumer_count; i++)
        pthread_join(c_thread[i], NULL);

    eventbuf_free(event_buffer);
}