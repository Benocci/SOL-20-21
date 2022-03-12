/*******************************************************************************************
CODATASK.h: header dei metodi della struttura dati che gestisce una coda dei task

AUTORE: FRANCESCO BENOCCI

NOTE: usata per mantenere i task in ordine di arrivo con politica FIFO

*********************************************************************************************/
#ifndef CODATASK_H
#define CODATASK_H

#include "utils.h"

//struct nodo che costituisce un nodo della coda,
//mantiene 'task' contenente il file descriptor e 'next' puntatore all'elemento successivo
typedef struct _nodoTask{
    int task;
    void* next;
}nodoTask;

//struct Queue che contiene un puntatore alla testa e uno alla coda
//della coda ed il numero di elementi al suo interno
typedef struct _queueTask{
    int queue_size;
    int queue_maxsize;
    pthread_mutex_t mtx;
    pthread_cond_t full;
    pthread_cond_t empty;
    nodoTask *head;
    nodoTask *tail;
}QueueTask;

/* EFFETTO: alloca ed inizializza una coda vuota
 *
 * RITORNA: NULL  -> errore nella creazione
 *          queue -> creazione avvenuta con successo 
 */
QueueTask* createQueueTask(int maxsize);

/* 
 * EFFETTO: inserisce in coda a 'q' un nuovo 'task'
 */
void enqueueTask(QueueTask *q, int task);

/* EFFETTO: rimuove il task in testa alla coda 'q'
 *
 * RITORNA: NULL       -> errore nell'estrazione
 *          to_dequeue -> estrazione avvenuta con successo
 */
int dequeueTask(QueueTask *q);

/*
 * EFFETTO: libera la memoria della coda 'q'
 */
void destroyQueueTask(QueueTask *q);

/*
 * EFFETTO: metodo ricorsivo che stampa su stdout la coda 'q'
 */
void printQueueTask(nodoTask *q);

#endif