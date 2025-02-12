#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "logger.h"
#include <semaphore.h>
#include <stdbool.h>
#include <iso646.h>

static sig_atomic_t is_init = false; // is logger initialized
volatile static sig_atomic_t is_logger_active = true;
volatile static sig_atomic_t current_logger_detail = LOG_MIN;

char* message_to_dump;

static pthread_t dumb_thread;
static sem_t dumb_sem;
static pthread_mutex_t log_mutex;
static pthread_mutex_t data_mutex;

static pthread_mutex_t terminate_mutex;
static sig_atomic_t terminate_thread = false;

void handle_change_logger_active(int signo, siginfo_t* info, void *other) {
    is_logger_active = !is_logger_active;
}

void handle_dump(int signo, siginfo_t* info, void *other) {
    sem_post(&dumb_sem);
}

void handle_level(int signo, siginfo_t* info, void *other) {
    const int new_detail_level = info->si_value.sival_int;
    if (new_detail_level < LOG_MIN or new_detail_level > LOG_ALL) return;
    current_logger_detail = new_detail_level;
}

void* dump_message(void* arg) {
    while (true) {
        if (sem_wait(&dumb_sem)) {

        }
        pthread_mutex_lock(&terminate_mutex);
        if (terminate_thread) {
            pthread_mutex_unlock(&terminate_mutex);
            break;
        }
        pthread_mutex_unlock(&terminate_mutex);

        char filename[99];
        char date[50];
        time_t curr_time = time(NULL);
        strftime(date, sizeof(date), "%Y-%m-%d_%H:%M:%S", gmtime(&curr_time));
        sprintf(filename, "log_%s.log",date);

        FILE* fptr = fopen(filename, "w+");
        if (!fptr) {
            //err
            continue;
        }

        pthread_mutex_lock(&data_mutex);
        if (!message_to_dump) {
            fprintf(fptr, "NO DATA\n");
        }
        else {
            fprintf(fptr, "%s\n", message_to_dump);
            free(message_to_dump);
        }
        pthread_mutex_unlock(&data_mutex);
        fclose(fptr);
    }
}

void save_log(logger_level_t level, const char* message, ...) {

}

int set_message(char* message) {
    if (!message) {
        errno = EINVAL;
        return -1;
    }

    char* msg = calloc(sizeof(message), sizeof(char));
    if (!msg) {
        //err
        return -1;
    }
    message_to_dump = msg;
    return 1;
}

void init_logger(void) {
    if (is_init) {
        return;
    }

    message_to_dump = NULL;

    sem_init(&dumb_sem, 0, 0);
    pthread_mutex_init(&log_mutex, NULL);
    pthread_mutex_init(&data_mutex, NULL);
    pthread_mutex_init(&terminate_mutex, NULL);

    struct sigaction action;

    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = handle_change_logger_active;
    sigaction(SIGRTMIN, &action, NULL);

    sigfillset(&action.sa_mask);
    action.sa_sigaction = handle_dump;
    sigaction(SIGRTMIN+1, &action, NULL);

    sigfillset(&action.sa_mask);
    action.sa_sigaction = handle_level;
    sigaction(SIGRTMIN+2, &action, NULL);

    pthread_create(&dumb_thread, NULL, dump_message, NULL);

    is_init = true;
}

void destroy_logger(void) {
    pthread_mutex_lock(&terminate_mutex);
    terminate_thread = true;
    pthread_mutex_unlock(&terminate_mutex);
    sem_post(&dumb_sem);
    pthread_join(dumb_thread, NULL);
    sem_destroy(&dumb_sem);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&data_mutex);
    pthread_mutex_destroy(&terminate_mutex);

    if (message_to_dump) {
        free(message_to_dump);
    }

    terminate_thread = false;
    is_init = false;
}