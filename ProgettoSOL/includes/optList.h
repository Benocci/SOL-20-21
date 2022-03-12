/*******************************************************************************************
OPTLIST.h: header dei metodi della struttura dati che gestisce una coda di "opzioni"

AUTORE: FRANCESCO BENOCCI

NOTE: utilizzata nei client per mantenere singole richieste parsate opportunamente

*********************************************************************************************/
#ifndef OPTLIST_H
#define OPTLIST_H

#include "utils.h"
#include <stdbool.h>

//struct opt che costituisce un "nodo" della linkedList,
//mantiene tipo e argomenti dell'opzione e un puntatore all'opt seguente
typedef struct _opt{
    int type; 
    char *args;
    struct _opt *next;
}opt;

//struct optList che contiene un puntatore alla testa e uno alla coda
//della linkedList ed il numero di elementi al suo interno
typedef struct _optList{
    int numElem;
    opt *head;
    opt *tail;
}optList;

/* EFFETTO: inserisce dentro 'list' una nuova opt di tipo
 *          'type_op' con argomento 'args'
 *
 * RITORNA: 0  -> inserimento avvenuto con successo
 *          -1 -> errore in inserimento
 */
int pushOpt(optList *list, int type_op, char *args);

/* EFFETTO: rimuove l'elemento in testa a 'list'
 *
 * RITORNA: NULL   -> errore nell'estrazione
 *          to_pop -> estrazione avvenuta con successo
 */
opt *popOpt(optList *list);

/*
 * EFFETTO: libera la memoria dell'opzione 'to_delete'
 */
void deleteOpt(opt *to_delete);

/*
 * EFFETTO: libera la memoria della lista 'list'
 */
void deleteOptList(optList *list);

/* EFFETTO: controlla il tipo dell'ultima opzione nella 'list'
 *
 * RITORNA: -1    -> lista vuota
 *           type -> tipo dell'ultima opt
 */
int checkLastOpt(optList *list);

#endif