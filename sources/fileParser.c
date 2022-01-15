/*******************************************************************************************
FILEPARSER.c: implementazione delle funzioni utilizzate per leggere, parsarere e ottenere da
              un file una struttura contenente le info utili al programma (comprensione del file di config)
AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "utils.h"
#include "fileParser.h"

//funzione di supporto che aggiunge alla lista un nuovo nodo:
int push(ParsedFile *list, struct _tokenizedFile *newNode){
    if(!list || !newNode){ //se gli argomenti non sono validi ritorno errore
        errno = EINVAL;
        return -1;
    }

    if(list->head == NULL){  //se la lista è vuota
        list->head = newNode;//il nuovo elemento diventa la testa
    }
    else{ //altrimenti
        list->tail->nextPtr = newNode; //viene messo in coda
    }
    list->tail = newNode; //il nuovo elemento diventa la coda della lista

    return 0;
}

//funzione di supporto che rimuove dalla lista il nodo in testa
struct _tokenizedFile *pop(ParsedFile *list){
    if(!list){ //se la lista non è valida ritorno errore
        errno = EINVAL;
        return NULL;
    }

    //mantengo l'elemento da rimuovere
    struct _tokenizedFile *to_pop = list->head;
    list->head = to_pop->nextPtr;//la nuova testa diventa l'elemento successivo

    if(list->head == NULL){//se la lista è vuota allora anche la coda diventa NULL
        list->tail = NULL;
    }

    to_pop->nextPtr = NULL;//"scollego" l'elemento dalla lista
    return to_pop;
}

//funzione di supporto che prende un FILE* 'fd' e lo tokenizza inserendolo in una lista ParsedFile 'pF'
int tokenizer_r(ParsedFile *pF, FILE *fd){
    char buf[MAX_STRING_LEN];//buffer in cui inserisco una riga letta dal file
    
    while(fgets(buf, MAX_STRING_LEN, fd)){//leggo finchè non raggiungo EOF
        if(buf[0] == '\0' && buf[0] == '\n'){//ignoro terminatori e newline
            continue;
        }
        

        char *token = strtok(buf, pF->delim);//tokenizzo la riga letta
        while(token){//finchè ho nuovi token ciclo
            struct _tokenizedFile *newNode = smalloc(sizeof(struct _tokenizedFile));//creo un nodo
            strncpy(newNode->word, token, sizeof(newNode->word));//copio nel nodo il token
            newNode->word[sizeof(newNode->word)] = '\0';//metto alla fine della parola il terminatore
            newNode->nextPtr = NULL;//metto il nodo successivo a NULL
            
            push(pF, newNode);//inserisco dentro a pF il nodo appena creato
            token = strtok(NULL, pF->delim);//continuo a tokenizzare la riga
        }

    }
    return 0;
}

ParsedFile *parseFile(char *filename, char *delim){
    //creo un nuovo parsedFile:
    ParsedFile *new = smalloc(sizeof(ParsedFile));

    if(!new){//controllo che sia stato allocato correttamente
        return NULL;
    }
    
    //inizializzo con gli argomenti passati
    strncpy(new->filename, filename, MAX_STRING_LEN_PATH);
    strncpy(new->delim, delim, MAX_STRING_LEN);
    
    //apro il file:
    FILE *fd;
    if((fd = fopen(filename, "r")) == NULL){
        perror("error: open");
        return NULL;
    }

    //faccio il parsing del file e lo inserisco nel parsedFile:
    if(tokenizer_r(new, fd) == -1){
        perror("error: tozenizer");
        return NULL;
    }
    
    fclose(fd);//chiudo il file
    return new;
}

int deleteParsedFile(ParsedFile *pF){
    if(!pF){//se 'pF' non è valido ritorno errore
        errno = EINVAL;
        return -1;
    }

    struct _tokenizedFile* tmp;//creo un puntatore temporaneo

    while(pF->head != NULL){//finchè la lista non è vuota
        tmp = pop(pF);//faccio una pop e mantengo l'elemento nel puntatore
        free(tmp);    //per poi liberarne la memoria
    }
    free(pF);

    return 0;
}

ConfigFile* configFileCheck(ParsedFile *config){
    struct _tokenizedFile *tmpPtr = config->head;//mantengo un puntatore temporaneo alla testa
    int configOpt;//variabile in cui salvo il tipo di opzione
    ConfigFile* new = smalloc(sizeof(ConfigFile));//creo un nuovo ConfigFile

    while(tmpPtr != NULL){//finchè la lista ha elementi ciclo
        configOpt = (int)tmpPtr->word[0];//salvo il tipo di opzione
        switch(configOpt){ //switch sul tipo di opzione per distinguere i casi:
            case 'c': 
                if(tmpPtr->nextPtr != NULL){ //se l'elemento successivo è valido:
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                    new->capacity = atoi(tmpPtr->word);//salvo la capacità del server
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                }
                break;
            case 'f':
                if(tmpPtr->nextPtr != NULL){ //se l'elemento successivo è valido:
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                    new->numFile = atoi(tmpPtr->word);//salvo il numero di file massimi del server
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                }
                break;
            case 'w':
                if(tmpPtr->nextPtr != NULL){ //se l'elemento successivo è valido:
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                    new->numWorker = atoi(tmpPtr->word);//salvo il numero di worker del server
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                }
                break;
            case 's':
                if(tmpPtr->nextPtr != NULL){ //se l'elemento successivo è valido:
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                    strncpy(new->socketName, tmpPtr->word, strlen(tmpPtr->word));//salvo il nome del socket
                    new->socketName[strlen(tmpPtr->word)-1] = '\0';              //e metto il terminatore alla fine
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                }
                break;
            case 'p':
                if(tmpPtr->nextPtr != NULL){ //se l'elemento successivo è valido:
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                    new->maxPendingConnection = atoi(tmpPtr->word);//salvo il numero massimo di connessioni pendenti
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                }
                break;
            case 'l':
                if(tmpPtr->nextPtr != NULL){ //se l'elemento successivo è valido:
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                    strncpy(new->fileLogName, tmpPtr->word, strlen(tmpPtr->word)); //salvo il nome del file di log
                    new->fileLogName[strlen(tmpPtr->word)] = '\0';                 //e metto il terminatore alla fine
                    tmpPtr = tmpPtr->nextPtr;//vado al prossimo elemento
                }
                break;
            default: //nel caso di elementi indesiderati nel file ritorno NULL stampando errore:
                printf("ERRORE: file di config non valido\n");
                return NULL;
        }
    }

    return new;
}
