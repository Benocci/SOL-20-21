/*******************************************************************************************
CODATASK.c: implementazione della struttura dati utilizzata per mantenere i taks
        
AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include "codaTask.h"
#include "utils.h"

QueueTask* createQueueTask(int maxsize){
    if(maxsize <= 0){
        fprintf(stderr, "SIZE INCORRETTA.\n");
        return NULL;
    }

    QueueTask* _queue = smalloc(sizeof(QueueTask));
    _queue->head = _queue->tail = NULL;//inizializzo testa e coda a NULL
    
    pthread_mutex_t mtx_coda_task;        //dichiaro e inizializzo la mutex della coda
    pthread_cond_t cond_coda_task_empty;  //dichiaro e inizializzo la condition variables per la coda vuota
    pthread_cond_t cond_coda_task_full;   //dichiaro e inizializzo la condition variables per la coda piena

    pthread_mutex_init(&mtx_coda_task, NULL);

    if(pthread_cond_init(&cond_coda_task_empty, NULL) == 1){
        fprintf(stderr, "ERRORE: inizializzazione cond della coda task.\n");
        return NULL;
    }

    if(pthread_cond_init(&cond_coda_task_full, NULL) == 1){
        fprintf(stderr, "ERRORE: inizializzazione cond della coda task.\n");
        return NULL;
    }

    //inserisco i valori nei campi della struttura coda:
    _queue->mtx = mtx_coda_task;
    _queue->empty = cond_coda_task_empty;
    _queue->full = cond_coda_task_full;
    _queue->queue_size = 0;                
    _queue->queue_maxsize = maxsize;
    
    return _queue;
}

void enqueueTask(QueueTask *q, int task){
    if(q == NULL){//se l'elemento è vuoto o la coda è vuota ritorno
        return;
    }
    //alloco e inizializzo il nuovo nodo da aggiungere:
    nodoTask *new = (nodoTask *)smalloc(sizeof(nodoTask));
    new->task = task;
    new->next = NULL;

    LOCK("mtx_coda_task", q->mtx);

    while(q->queue_size == q->queue_maxsize){ // attendo se la coda è piena
        printf("CODA PIENA, cercavo di inserire %d.\n", task);
        CHECK_FUNCTION("pthread_cond_wait", pthread_cond_wait(&(q->full), &(q->mtx)), !=, 0);
    }

    //se la coda 'q' è vuota il nuovo nodo diventa sia testa che coda
    if(q->head == NULL || q->queue_size == 0){
        q->head = new;
        q->tail = new;
    }
    else{//altrimenti
        nodoTask *tmp = q->tail;
        tmp->next = new;//metto in coda il nuovo elemento
        q->tail = new;  //imposto come nuova coda il nuovo elemento
        
    }
    q->queue_size++;//incremento la size

    if(q->queue_size == 1){ // invio il segnale se la coda era vuota
        CHECK_FUNCTION("pthread_cond_broadcast", pthread_cond_broadcast(&(q->empty)), !=, 0);
    }

    // printf("PROVA: inserito %d, CODA di SIZE %d.\n", task, q->queue_size);

    UNLOCK("mtx_coda_task", q->mtx);
}

int dequeueTask(QueueTask *q){
    LOCK("mtx_coda_task", q->mtx);

    while(q->queue_size == 0){ //se la coda è vuota atttendo
        CHECK_FUNCTION("pthread_cond_wait", pthread_cond_wait(&(q->empty), &(q->mtx)), !=, 0);
    }
    
    nodoTask *to_dequeue = q->head;//prendo un puntatore all'elemento in testa da rimuovere
    q->head = q->head->next;       //pongo come nuova testa l'elemento successivo
    q->queue_size--;               //decremento la siza
    if(q->queue_size == 0){        //se la coda è vuota imposto come coda NULL;
        q->tail = NULL;
    }

    int task_todequeue = to_dequeue->task;
    free(to_dequeue);

    if(q->queue_size + 1 == q->queue_maxsize){ // invio il segnale se la coda sarà piena
        CHECK_FUNCTION("pthread_cond_broadcast", pthread_cond_broadcast(&(q->full)), !=, 0);
    }

    // printf("PROVA: rimuovo %d, CODA di SIZE %d.\n", task_todequeue, q->queue_size);
    
    UNLOCK("mtx_coda_task", q->mtx);
    return task_todequeue;         //ritorno il nodo rimosso

}

void destroyQueueTask(QueueTask *q){
    LOCK("mtx_coda_task", q->mtx);
    while(q->head != NULL){     //finchè la coda non è vuota
        nodoTask *tmp = q->head;    //prendo il nodo in testa
        q->head = q->head->next;//lo separo dalla coda
        tmp->next = NULL;
        free(tmp);             //libero la memoria del nodo
        q->queue_size--;       //decremento la size
    }
    UNLOCK("mtx_coda_task", q->mtx);
    
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->empty);
    pthread_cond_destroy(&q->full);

    free(q);//libero la memoria della coda
}


void printQueueTask(nodoTask *q){
    if(q == NULL){//se la coda è vuota stampo NULL e ritorno
        printf("NULL\n");
        return;
    }
    printf("'%d'->", q->task);//stampo l'elemento
    printQueueTask(q->next);             //e chiamo ricorsivamente "printQueue" sul nodo successivo
}