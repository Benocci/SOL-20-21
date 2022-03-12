/*******************************************************************************************
CODAFILE.h: header dei metodi della struttura dati che gestisce una coda di file

AUTORE: FRANCESCO BENOCCI

NOTE: usata per mantenere i file in ordine di entrata così da espellerli con meccanica FIFO
      e per deallocare la memoria di tutti i file creati al termine del programma.

*********************************************************************************************/
#ifndef CODAFILE_H
#define CODAFILE_H

#include "utils.h"
#include "file.h"

//struct nodo che costituisce un nodo della coda,
//mantiene 'key' contenente il file e 'next' puntatore all'elemento successivo
typedef struct _nodoFile{
    File* key;
    void* next;
}nodoFile;

//struct Queue che contiene un puntatore alla testa e uno alla coda
//della coda ed il numero di elementi al suo interno
typedef struct _queueFile{
    int queue_size;
    nodoFile *head;
    nodoFile *tail;
}QueueFile;

/* EFFETTO: alloca ed inizializza una coda vuota
 *
 * RITORNA: NULL  -> errore nella creazione
 *          queue -> creazione avvenuta con successo 
 */
QueueFile* create();

/* 
 * EFFETTO: inserisce in coda a 'q' un nuovo elemento 'elem'
 */
void enqueue(QueueFile *q, void *elem);

/* EFFETTO: rimuove l'elemento in testa alla coda 'q'
 *
 * RITORNA: NULL       -> errore nell'estrazione
 *          to_dequeue -> estrazione avvenuta con successo
 */
void* dequeue(QueueFile *q);

/* EFFETTO: rimuove l'elemento 'elem' da 'q' se presente
 *
 * RITORNA:  1 -> estrazione avvenuta con successo
 *           0 -> nessuna estrazione
 */
int removeElementFromQueue(QueueFile *q, void *elem);

/*
 * EFFETTO: libera la memoria della coda 'q'
 */
void destroyQueue(QueueFile *q);

/*
 * EFFETTO: metodo ricorsivo che stampa su stdout la coda 'q'
 */
void printQueue(nodoFile *q);

#endif