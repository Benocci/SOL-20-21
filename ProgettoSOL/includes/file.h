/*******************************************************************************************
OPTLIST.h: header dei metodi della struttura dati che gestisce una coda di "opzioni"

AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include "utils.h"

//struct File che mantiene il file salvato nel server (dentro all'hashtable)
//mantiene i seguenti elementi:
typedef struct _file{
    char pathname[PATH_MAX_LENGHT];//pathname del file

    char *data;//contenuto del file (NULL se non ancora scritto)
    size_t data_length;//lunghezza del contenuto del file(-1 se non ancora scritto)

    int lockedBy; //PID del client che ha lockato il file
    int openBy;   //PID del client che ha aperto il file
}File;
