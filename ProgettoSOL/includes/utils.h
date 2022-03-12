/*******************************************************************************************
UTILS.h: header con define e metodi utili al programma

AUTORE: FRANCESCO BENOCCI

NOTE: funzioni chiamate da più parti del programma

*********************************************************************************************/
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>

//definizione di valori utili al programma:
#ifndef BUFSIZE
#define BUFSIZE 256
#endif

#ifndef MAX_LENGTH
#define MAX_LENGTH 512
#endif

#ifndef PATH_MAX_LENGHT
#define PATH_MAX_LENGHT 256
#endif

#include "message.h"

//struct ProgramStatus che mantiene info utili al programma:
typedef struct programstatus{
    int* workerRequests; //array di int con i worker
    int num_current_con; //numero di connessioni attive
    int max_con;         //numero massimo di connessioni attivate

    int num_file_open;   //numero totale di file aperti
    int num_file_create; //numero totale di file creati
    int num_file_deleted;//numero totale di file eliminati
    int num_file_readed; //numero totale di file letti
    int num_file_writed; //numero totale di file scritti
    int num_file_locked; //numero totale di file messi in lock
    int num_file_closed; //numero totale di file chiusi

    int num_max_file_reached;  //massimo numero di file contenuti dal server
    int num_max_byte_reached;  //massimo numero di byte contenuti dal server
    int num_current_file_saved;//numero di file salvati al momento
    int num_current_file_open; //numero di file salvati al momento
    int num_current_bytes;     //numero di byte al'interno del server al momento
}ProgramStatus;

//flag utilizzati dall'opzione di op:
#define O_NOFLAG 300
#define O_CREATE 301
#define O_LOCK 302
#define O_LOCKED_CREATE 303

//Codici per il client:
#define SEND_DIRECTORY_OP 11  //-w
#define SEND_FILE_OP 12       //-W
#define READ_FILE_OP 13       //-r
#define READ_N_FILE_OP 14     //-R
#define LOCK_OP 15            //-l
#define UNLOCK_OP 16          //-u
#define REMOVE_OP 17          //-c
#define DIRECTORY_WRITE 18    //-d

//Codici per il server:
#define OPEN_FILE 101
#define READ_FILE 102
#define READ_N_FILE 103
#define WRITE_FILE 104
#define APPEND_TO_FILE 105
#define LOCK_FILE 106
#define UNLOCK_FILE 107
#define CLOSE_FILE 108
#define REMOVE_FILE 109

#define CLOSE_CONNECTION 255
#define OPEN_CONNECTION 254

#define NOMORE_FILE 111

//definizione di macro utili al programma:
#define SYSCALL_EXIT(name, return_val, system_call)\
    if((return_val = system_call) == -1){          \
        perror(#name);                             \
        int errno_copy = errno;                    \
        exit(errno_copy);                          \
    }

#define SYSCALL_RETURN(name, return_val, system_call)\
    if((return_val = system_call) == -1){            \
        perror(#name);                               \
        return -1;                                   \
    }

#define CHECK_FUNCTION(name, function, operation, expected_value)\
    if(function operation expected_value){                       \
        perror(#name);                                           \
        int errno_copy = errno;                                  \
        exit(errno_copy);                                        \
    }

#define CHECK_EINTR(name,fun)       \
    if(fun == -1 && errno != EINTR){\
        perror(#name);              \
        int errno_copy = errno;     \
        exit(errno_copy);           \
    }

#define CHECK_NULL(name, fun)  \
    if(fun == NULL){           \
        perror(#name);         \
        int errno_copy = errno;\
        exit(errno_copy);      \
    }

#define LOCK(name, mutex)                \
    if(pthread_mutex_lock(&mutex) != 0){ \
        perror(#name);                   \
        int errno_copy = errno;          \
        exit(errno_copy);                \
    }

#define UNLOCK(name, mutex)                \
    if(pthread_mutex_unlock(&mutex) != 0){ \
        perror(#name);                     \
        int errno_copy = errno;            \
        exit(errno_copy);                  \
    }

/* EFFETTO: safe malloc che alloca memoria controllando eventuali errori
 *          
 * RITORNA: memoria di 'size' byes  -> allocazione avvenuta con successo
 *          NULL                    -> errore 
 */
void *smalloc(size_t size);

/* EFFETTO: evitando letture parziali
 *
 * RITORNA: 0    -> se durante la lettura da fd leggo EOF
 *          -1   -> errore (errno settato)
 *          size -> termina con successo
 */
int readn(long fd, void *buf, size_t size);

/* EFFETTO: evitando scritture parziali
 *
 * RITORNA: 0    -> se durante la scrittura la write ritorna 0
 *          -1   -> errore (errno settato)
 *          size -> se la scrittura termina con successo
 */
int writen(long fd, void *buf, size_t size);

/*
 * EFFETTO: inserisce dentro a msg i campi dati come argomento
 */
void setMsg(Msg *msg,int opt_code, int flags, char response_code[RESPONSE_CODE_LENGTH], char path[RESPONSE_CODE_LENGTH], char *data, size_t size);

/* EFFETTO: funzione che spedisce al socket 'fd' il Msg 'to_send' controllando errori
 *
 * RITORNA: 0    -> spedizione avvenuta con successo
 *          -1   -> errore
 */
int sendMsg(Msg *to_send, long fd);

/* EFFETTO: funzione che riceve dal socket 'fd' il Msg 'to_recive' controllando errori
 *
 * RITORNA: 0    -> ricezione avvenuta con successo
 *          -1   -> errore
 */
int receiveMsg(Msg *to_recive, long fd);

/* 
 * EFFETTO: converte il codice intero dato in input in una stringa corrispondente
 */
char *convertOptCodeInChar(int opt_code);

/*
 * EFFETTO: stampa su stdout i campi del Msg quando presenti in maniera formattata (utile al debugging)
 */
void printINFOMsg(Msg *to_print);

/* 
 * EFFETTO: scrive le statistiche del server salvate su 'ps' sul file di log passato e su stdout
 */
void saveProgramStatusOnLogFile(FILE *fileLog, ProgramStatus ps, int num_worker, int print_on_stdout);

#endif