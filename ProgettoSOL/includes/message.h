/*******************************************************************************************
MESSAGE.h: header che contiene la struct dei messaggi inviati tra client e server

AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#ifndef MESSAGE_H
#define MESSAGE_H

#include "utils.h"

#ifndef RESPONSE_CODE_LENGTH
#define RESPONSE_CODE_LENGTH 5
#endif

//struct Msg che costituisce un messaggio spedibile tra client e server
//mantiene i seguenti elementi:
typedef struct _msg{
    int opt_code;//opt_code è il codice dell'opzione che vuole fare il client(se 0 allora il messaggio è dal server al client)
    int flags;   //flag utili per alcune opzioni (open_file), utilizzata anche per salvare il numero di file da leggere in readNFiles
    char response_code[RESPONSE_CODE_LENGTH];// viene settato per comunicare al client il codice di risposta del server

    char path[PATH_MAX_LENGHT];//path del file su cui applicare l'opzione richiesta

    int pid;//PID del thread che invia il messaggio

    int data_length;//lunghezza del contenuto del file su cui applicare l'azione richiesta (-1 se non necessario)
    char* data;     //contenuto del file su cui applicare l'azione richiesta (NULL se non necessario)
} Msg;

#endif