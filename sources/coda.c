/*******************************************************************************************
CODA.c: implementazione della struttura dati utilizzata per mantenere i pathname dei
        file salvati all'interno del server
AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include "coda.h"

Queue* create(){
    Queue* _queue;
    if( (_queue = malloc(sizeof(Queue*))) == NULL){//alloco una nuova coda
        perror("Errore allocazione coda:");
        exit(EXIT_FAILURE);
    }
    _queue->head = _queue->tail = NULL;//inizializzo testa e coda a NULL
    _queue->queue_size = 0;            //e size a 0
    
    return _queue;
}


void enqueue(Queue *q, void *elem){
    //alloco e inizializzo il nuovo nodo da aggiungere:
    nodo *new = smalloc(sizeof(nodo));
    new->key  = smalloc(sizeof(char)*strlen((char *)elem));
    strcpy(new->key, (char *)elem);
    new->next = NULL;

    //se la coda 'q' è vuota il nuovo nodo diventa sia testa che coda
    if(q->head == NULL || q->queue_size == 0){
        q->head = new;
        q->tail = new;
    }
    else{//altrimenti
        nodo *tmp = q->head;

        while (tmp->next != NULL){//scorro finchè non arrivo all'ultimo elemento
            tmp = tmp->next;
        }
        tmp->next = new;//metto in coda il nuovo elemento
        q->tail = new;  //imposto come nuova coda il nuovo elemento
        
    }
    q->queue_size++;//incremento la size
}

void* dequeue(Queue *q){
    if(q->head == NULL || q->queue_size == 0){//se l'elemento è vuoto(quindi la coda è vuota)
        return NULL;//ritorno NULL
    }
    else{
        nodo *to_dequeue = q->head;//prendo un puntatore all'elemento in testa da rimuovere
        q->head = q->head->next;   //pongo come nuova testa l'elemento successivo
        q->queue_size--;           //decremento la siza
        if(q->queue_size == 0){    //se la coda è vuota imposto come coda NULL;
            q->tail = NULL;
        }
        return to_dequeue;         //ritorno il nodo rimosso
    }
}

int removeElementFromQueue(Queue *q, void *elem){
    if(q->head == NULL || q->queue_size == 0){
        return 0;
    }

    if(strcmp(q->head->key, elem) == 0){//se l'elemento da rimuovere è in testa
        nodo *tmp = dequeue(q); //rimuovo l'elemento con dequeue
        if(tmp != NULL){
            free(tmp);
        }
        return 1;
    }

    nodo* currPtr = q->head->next; //prendo un riferimento al secondo elemento
    nodo* precPtr = q->head;       //ed uno a quello in testa

    while(currPtr != NULL){//finchè non arrivo alla fine della coda
        if(strcmp(currPtr->key, elem) == 0){//se trovo l'elemento:
            nodo* tmp = currPtr;       //estraggo il nodo
            //rimuovo l'elemento dalla lista:
            precPtr->next = currPtr->next;
            if(tmp != NULL){
                free(tmp); //libero la memoria dell'elemento da rimuovere
            }
            q->queue_size--; //decremento la size
            return 1;
        }
        else{//altrimenti procedo nella coda al prossimo elemento:
            currPtr = currPtr->next;
            precPtr = precPtr->next;
        }
    }
    return 0;
}

//funzione ausiliaria che libera la memoria del nodo passato come argomento
void deleteNodo(nodo *node){
    if(node != NULL){
        if(node->key != NULL){
            free(node->key);
        }
        node->next = NULL;
        free(node);
    }
}

void destroyQueue(Queue * q){
    while(q->queue_size > 0){  //finchè la coda non è vuota
        deleteNodo(dequeue(q));//faccio dequeue e libero la memoria dell'elemento rimosso
    }
    
    free(q);//libero la memoria della coda
}


void printQueue(nodo *q){
    if(q == NULL){//se la coda è vuota stampo NULL e ritorno
        printf("NULL\n");
        return;
    }
    printf("'%s'->", (char *)q->key);//stampo l'elemento
    printQueue(q->next);             //e chiamo ricorsivamente "printQueue" sul nodo successivo
}

