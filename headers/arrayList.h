/*******************************************************************************************
ARRAYLIST.h: header dei metodi della struttura dati arrayList

AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "utils.h"
#include <getopt.h>
#include <time.h>
#include <errno.h>

//struct arrayList che contiene un array "queue" e 4 valori che 
//mantengono testa, coda, size massima e corrente dell'array
typedef struct _arrayList{
    int head;
    int tail;
    int* queue;
    int maxSize;
    int currentSize;
}arrayList;

/* EFFETTO: crea una arrayList di dimensione "size"
 *
 * RITORNA: NULL -> errore in creazione
 *          new_list -> creazione avvenuta con successo
 */
arrayList* createArrayList(int size);

/* EFFETTO: elimina "to_delete" e libera la memoria
 */
void deleteArrayList(arrayList* to_delete);

/* EFFETTO: aggiunge a "list" l'elemento "to_add" se questo è != NULL e se "list" non è piena
 */
void addToArrayList(arrayList* list, int to_add);

/* EFFETTO: rimuove l'elemento in coda di "list"
 *
 * RITORNA: -1 -> errore: lista vuota
 *          elemento rimosso -> rimozione avvenuta con successo
 */
int removeFromArrayList(arrayList*list);

#endif
