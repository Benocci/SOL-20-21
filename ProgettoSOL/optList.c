/*******************************************************************************************
OPTLIST.c: implementazione della struttura dati che viene utilizzata per gestire le "opzioni"
           date ad un client. (LinkedList come coda -> push&pop)
AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include "utils.h"
#include "optList.h"


int pushOpt(optList *list, int type_op, char *args){
    //alloco dinamicamente il nuovo opt da inserire:
    opt *new = smalloc(sizeof(opt));

    //inizializzo le sue info:
    new->type = type_op;
    new->args = args;
    new->next = NULL;

    //se la lista è vuota allora il nuovo elemento diventa sia testa che coda:
    if(list->head == NULL){
        list->head = new;
        list->tail = new;
    }
    else{//altrimenti inserisco in coda:
        opt *tmp = list->head;//prendo un riferimento alla testa

        while(tmp->next != NULL){//scorro finchè non arrivo ad avere un elemento con il successivo NULL
            tmp = tmp->next;
        }
        tmp->next = new;//appendo il nuovo elemento all'ultimo elemento trovato
        list->tail = new;//il nuovo elemento diventa la coda della lista
    }
    list->numElem++;//incremento il numero di elementi
    return 0;
}

opt *popOpt(optList *list){
    if(list->numElem == 0){//se la lista non ha elementi ritorno NULL
        return NULL;
    }
    else{//altrimenti
        //prendo un riferimento alla testa:
        opt *to_pop = list->head;
        //l'elemento successivo diventa il nuovo elemento in testa.
        list->head = list->head->next;
        list->numElem--;//decremento il numero di elementi
        if(list->numElem == 0){//se il numero di elementi è 0 allora anche la coda è NULL
            list->tail = NULL;
        }
        return to_pop;
    }
}

void deleteOpt(opt *to_delete){
    if(to_delete->args != NULL){//se gli argomenti non sono NULL libero la memoria:
        free(to_delete->args);
    }
    if(to_delete->next != NULL){//se il puntatore all'elemento successivo punta a qualcosa lo libero:
        to_delete->next = NULL;
    }
    free(to_delete);//dealloco 'to_delete'
}

void deleteOptList(optList *list){
    while(list->numElem > 0){   //finchè 'list' non ha 0 elementi
        deleteOpt(popOpt(list));//faccio una pop ed libero l'elemento rimosso
    }
}

int checkLastOpt(optList *list){
    if(list->numElem == 0){
        return -1;//se la lista non ha elementi ritorno -1
    }
    return list->tail->type;
}