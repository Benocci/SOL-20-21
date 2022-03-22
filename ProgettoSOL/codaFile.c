/*******************************************************************************************
CODAFILE.c: implementazione della struttura dati utilizzata per mantenere i
        file salvati all'interno del server
AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include "codaFile.h"
#include "utils.h"

QueueFile* create(){
    QueueFile* _queue = smalloc(sizeof(QueueFile));
    _queue->head = _queue->tail = NULL;//inizializzo testa e coda a NULL
    _queue->queue_size = 0;            //e size a 0
    
    return _queue;
}


void enqueue(QueueFile *q, void *elem){
    if(q == NULL || elem == NULL){//se l'elemento è vuoto o la coda è vuota ritorno
        return;
    }
    //alloco e inizializzo il nuovo nodo da aggiungere:
    nodoFile *new = (nodoFile *)smalloc(sizeof(nodoFile));
    new->key = elem;
    new->next = NULL;

    //se la coda 'q' è vuota il nuovo nodo diventa sia testa che coda
    if(q->head == NULL || q->queue_size == 0){
        q->head = new;
        q->tail = new;
    }
    else{//altrimenti
        nodoFile *tmp = q->tail;
        tmp->next = new;//metto in coda il nuovo elemento
        q->tail = new;  //imposto come nuova coda il nuovo elemento
        
    }
    q->queue_size++;//incremento la size
}

void* dequeue(QueueFile *q){
    if(q->head == NULL || q->queue_size == 0){//se l'elemento è vuoto(quindi la coda è vuota)
        return NULL;//ritorno NULL
    }
    else{
        nodoFile *to_dequeue = q->head;//prendo un puntatore all'elemento in testa da rimuovere
        q->head = q->head->next;   //pongo come nuova testa l'elemento successivo
        q->queue_size--;           //decremento la siza
        if(q->queue_size == 0){    //se la coda è vuota imposto come coda NULL;
            q->tail = NULL;
        }

        File * to_return = to_dequeue->key;
        free(to_dequeue);
        return to_return;         //ritorno il file rimosso
    }
}

int removeElementFromQueue(QueueFile *q, void* elem){
    if(q->head == NULL || q->queue_size == 0){//se la coda è vuota o la testa è NULL ritorno 0 (nessuna eliminazione)
        return 0;
    }

    if(strcmp(q->head->key->pathname, elem) == 0){//se l'elemento da rimuovere è in testa
        File *tmp = dequeue(q);                   //rimuovo l'elemento con dequeue

        if(tmp->data != NULL){               //libero la memoria del contenuto del file
            free(tmp->data);
        }
        if(tmp != NULL){                     //libero la memoria del file stesso
            free(tmp);
        }
        
        
        return 1; // ritorno 1 (eleminazione avvenuta)
    }

    nodoFile* currPtr = q->head->next; //prendo un riferimento al secondo elemento
    nodoFile* precPtr = q->head;       //ed uno a quello in testa

    while(currPtr != NULL){//finchè non arrivo alla fine della cod
        if(strcmp(currPtr->key->pathname, (char *)elem) == 0){//se trovo l'elemento:
            nodoFile* tmp = currPtr;       //estraggo il nodo
            //rimuovo l'elemento dalla lista:
            precPtr->next = currPtr->next;
            if(tmp->key->data != NULL){ //libero la memoria del contenuto del file
                free(tmp->key->data);
            }
            free(tmp->key);//libero la memoria del file stesso
            free(tmp);     //libero la memoria del nodo
            
            q->queue_size--; //decremento la size
            return 1;        // ritorno 1 (eleminazione avvenuta)
        }
        else{//altrimenti procedo nella coda al prossimo elemento:
            currPtr = currPtr->next;
            precPtr = precPtr->next;
        }
    }

    return 0; // ritorno 0 (nessuna eleminazione)
}

void destroyQueue(QueueFile *q){
    while(q->head != NULL){     //finchè la coda non è vuota
        nodoFile *tmp = q->head;    //prendo il nodo in testa
        q->head = q->head->next;//lo separo dalla coda
        tmp->next = NULL;
        free(tmp->key->data);  //libero la memoria del contenuto del file
        free(tmp->key);        //libero la memoria del file stesso
        free(tmp);             //libero la memoria del nodo
        q->queue_size--;       //decremento la size
    }
    
    free(q);//libero la memoria della coda
}


void printQueue(nodoFile *q){
    if(q == NULL){//se la coda è vuota stampo NULL e ritorno
        printf("NULL\n");
        return;
    }
    printf("'%s'->", (char *)q->key);//stampo l'elemento
    printQueue(q->next);             //e chiamo ricorsivamente "printQueue" sul nodo successivo
}

