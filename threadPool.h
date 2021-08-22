// Liron Vaitzman 206505588

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct thread_pool
{
 pthread_t* threads;
 int num_threads;
 OSQueue* queue;
 bool destroy;
 bool shouldWait;
 pthread_mutex_t mutex;
 pthread_cond_t cond;
}ThreadPool;

typedef struct Node
{
 void (*computeFunc) (void *);
 void* param;
}Node;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
