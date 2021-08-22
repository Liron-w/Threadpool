// Liron Vaitzman 206505588

#include "threadPool.h"
#include "osqueue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void* do_task(void *tp)
{
    //printf("in do task\n");
    ThreadPool *pool = (ThreadPool *)tp;
    // locked mutex becose there is access to common variablrs
    pthread_mutex_lock(&pool->mutex);
    // if tpDestroy was not call yet - can do task
    while (!(pool->destroy))
    {
        if (!osIsQueueEmpty(pool->queue))
        {
            Node *node;
            node = osDequeue(pool->queue);
            pthread_mutex_unlock(&pool->mutex);
            //printf("before function\n");
            void (*computeFunc)(void *) = node->computeFunc;
            //printf("after function\n");
            computeFunc(node->param);
            free(node);
            pthread_mutex_lock(&pool->mutex);
        }
        else
        {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
    }

    // if we need to do all the task that in the queue
    if (pool->shouldWait)
    {

        while (!osIsQueueEmpty(pool->queue))
        {
            Node *node;
            node = osDequeue(pool->queue);
            pthread_mutex_unlock(&pool->mutex);
            void (*computeFunc)(void *) = node->computeFunc;
            computeFunc(node->param);
            free(node);
            pthread_mutex_lock(&pool->mutex);
        }
    }
    pthread_mutex_unlock(&pool->mutex);

    return NULL;
}

ThreadPool *tpCreate(int numOfThreads)
{
    // allocation space to the thread-pool struct
    ThreadPool *thread_pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (thread_pool == NULL)
    {
        perror("Error in malloc");
        free(thread_pool);
        exit(-1);
    }
    // allocation space to the threads array
    thread_pool->threads = (pthread_t *)malloc((sizeof(pthread_t *)) * numOfThreads);
    if (thread_pool->threads == NULL)
    {
        perror("Error in malloc");
        free(thread_pool->threads);
        free(thread_pool);
        exit(-1);
    }
    // allocation space to the tasks queue
    thread_pool->queue = osCreateQueue();
    if (thread_pool->queue == NULL)
    {
        perror("Error in malloc");
        osDestroyQueue(thread_pool->queue);
        free(thread_pool->threads);
        free(thread_pool);
        exit(-1);
    }
    // init mutex
    pthread_mutex_init(&thread_pool->mutex, NULL);
    pthread_cond_init(&thread_pool->cond, NULL);

    thread_pool->destroy = false;
    thread_pool->num_threads = numOfThreads;
    thread_pool->shouldWait = false;

    // create the threads
    int i;
    for (i = 0; i < numOfThreads; i++)
    {
        int thread = pthread_create(&(thread_pool->threads)[i], NULL, &do_task, (void *)thread_pool);
        //  printf(" pid %u tid %u\n", (unsigned int)getpid(), (unsigned int)(thread_pool->threads)[i]);
        if (thread != 0)
        {
            perror("Error in pthread_create");
        }
    }
    //printf("after for loop\n");
    return thread_pool;
}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param)
{
    pthread_mutex_lock(&threadPool->mutex);
    if (!threadPool->destroy)
    {
        // create task
        Node *node = (Node *)malloc(sizeof(Node));
        if (node == NULL)
        {
            perror("Error in malloc");
            free(node);
            free(threadPool->threads);
            free(threadPool);
            exit(-1);
        }
        node->param = param;
        node->computeFunc = computeFunc;
        // push the task to the queue
        osEnqueue(threadPool->queue, node);
        pthread_mutex_unlock(&threadPool->mutex);
        // signaling that there is a task that needs to be performed
        pthread_cond_signal(&threadPool->cond);
        //printf("after signal\n");
        return 0;
    }
    pthread_mutex_unlock(&threadPool->mutex);
    return -1;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks)
{
    pthread_mutex_lock(&threadPool->mutex);
    threadPool->destroy = true;
    if (shouldWaitForTasks)
    {
        threadPool->shouldWait = true;
    }
    pthread_mutex_unlock(&threadPool->mutex);

    pthread_cond_broadcast(&threadPool->cond);
    int i;
    for (i = 0; i < threadPool->num_threads; i++)
    {
        pthread_join(threadPool->threads[i], NULL);
    }
    //printf("after all join\n");

    while (!osIsQueueEmpty(threadPool->queue))
    {
        Node *node = (Node *)osDequeue(threadPool->queue);
        if (node != NULL)
        {
            free(node);
        }
    }

    osDestroyQueue(threadPool->queue);
    //printf("here\n");

    free(threadPool->threads);
    free(threadPool);

    //printf("end!\n");
    pthread_mutex_destroy(&threadPool->mutex);
    pthread_cond_destroy(&threadPool->cond);
}
