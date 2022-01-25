/*******************************************************************************************
CODA.h: header dei metodi della struttura dati che gestisce una coda di elementi generici
        usata nello specifico per mantenere i pathname dei file salvati nel server
AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#ifndef CODA_H
#define CODA_H

#include "utils.h"

//struct nodo che costituisce un nodo della coda,
//mantiene 'key' contenente il valore e 'next' puntatore all'elemento successivo
typedef struct _nodo{
    void* key;
    void* next;
}nodo;

//struct Queue che contiene un puntatore alla testa e uno alla coda
//della coda ed il numero di elementi al suo interno
typedef struct _queue{
    int queue_size;
    nodo *head;
    nodo *tail;
}Queue;

/* EFFETTO: alloca ed inizializza una coda vuota
 *
 * RITORNA: NULL  -> errore nella creazione
 *          queue -> creazione avvenuta con successo 
 */
Queue* create();

/* 
 * EFFETTO: inserisce in coda a 'q' un nuovo elemento 'elem'
 */
void enqueue(Queue *q, void *elem);

/* EFFETTO: rimuove l'elemento in testa alla coda 'q'
 *
 * RITORNA: NULL       -> errore nell'estrazione
 *          to_dequeue -> estrazione avvenuta con successo
 */
void* dequeue(Queue *q);

/* EFFETTO: rimuove l'elemento 'elem' da 'q' se presente
 *
 * RITORNA:  1 -> estrazione avvenuta con successo
 *           0 -> nessuna estrazione
 */
int removeElementFromQueue(Queue *q, void *elem);

/*
 * EFFETTO: libera la memoria della coda 'q'
 */
void destroyQueue(Queue * q);

/*
 * EFFETTO: metodo ricorsivo che stampa su stdout la coda 'q'
 */
void printQueue(nodo *q);

#endif
