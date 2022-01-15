/*******************************************************************************************
ARRAYLIST.c: implementazione della struttura dati che viene utilizzata per gestire i client
             in coda che il server deve servire
AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include "arrayList.h"

arrayList* createArrayList(int size){
    if(size <= 0){ //se la size è <=0 restituisco NULL stampando errore:
        printf("ERRORE: size in createArrayList minore o uguale a 0.\n");
        return NULL;
    }

    //alloco dinamicamente l'arraylist e la coda (array) al suo interno:
    arrayList* new_list = smalloc(sizeof(arrayList));
    new_list->queue = smalloc(size*sizeof(int));

    //inizializzo i valori dell'arraylist a 0 e la maxSize:
    new_list->head = 0;
    new_list->tail = 0;
    new_list->currentSize = 0;
    new_list->maxSize = size;

    return new_list;
}


void deleteArrayList(arrayList* to_delete){
    //controllo che l'arrayList e l'array al suo interno non siano NULL:
    CHECK_NULL("to_delete", to_delete);
    CHECK_NULL("array in to_delete", to_delete->queue);

    //libero la memoria:
    free(to_delete->queue);
    free(to_delete);
}


void addToArrayList(arrayList* list, int to_add){
    //controllo che l'arrayList sia diverso da NULL:
    CHECK_NULL("add in una lista NULL", list);

    //controllo di non star superando la maxSize:
    if(list->maxSize <= list->currentSize){
        printf("Inserimento in una lista piena!\n");
        return;
    }

    //aggiungo il nuovo valore in coda all'array:
    list->currentSize++;
    list->queue[list->tail] = to_add;
    //aggiorno la nuova coda:
    list->tail = (list->tail+1) % list->maxSize;
}


int removeFromArrayList(arrayList*list){
    //controllo che l'arrayList sia diverso da NULL:
    CHECK_NULL("remove in una lista NULL", list);

    //controllo che l'array non sia vuoto:
    if(list->currentSize <= 0){
        printf("Rimozione da una lista vuota!\n");
        return -1;
    }

    //salvo l'elemento da rimuovere:
    int to_return = list->queue[list->head];
    //rimuovo l'elemento in testa all'arrayList:
    list->currentSize--;
    list->head = (list->head+1) % list->maxSize;

    return to_return;
}
