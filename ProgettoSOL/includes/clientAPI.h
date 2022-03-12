/***************************************************************************************
CLIENTAPI.h: header dei metodi che permettono al client di comunicare con il server

AUTORE: FRANCESCO BENOCCI

NOTE: 

****************************************************************************************/
#ifndef CLIENTAPI_H
#define CLIENTAPI_H

#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"

#define UNIX_PATH_MAX 108

#define OPEN_FILE 101
#define READ_FILE 102
#define READ_N_FILE 103
#define WRITE_FILE 104
#define APPEND_TO_FILE 105
#define LOCK_FILE 106
#define UNLOCK_FILE 107
#define CLOSE_FILE 108
#define REMOVE_FILE 109

//file descriptor del socket usato per comunicare con il server:
int fd_skt;

//pathname del socket:
char *skt_path;

//boolano esterno usato come controllo nelle stampe su stdout, portato a true se viene inserita l'opzione -p:
extern bool print_stdout;

//Interfaccia richiesta dalla specifica per interagire con il file server (API)

/* EFFETTO: apre una connessione AF_UNIX sul socket "sockname", ripetendola dopo "msec" millisecondi
 *          fino al passare di "abstime" tempo.
 *
 * RITORNA: -1 -> errore settando errno
 *           fd_skt -> terminazione con successo
 */
int openConnection(const char *sockname, int msec, const struct timespec abstime);

/* EFFETTO: chiude la connessione AF_UNIX sul socket "sockname".
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int closeConnection(const char *sockname);

/* EFFETTO: richiede al server di aprire o creare il file indicato da "pathname". 
 *          
 * FLAGS:   0 -> O_NOFLAGS
 *          1 -> O_CREATE 
 *          2 -> O_LOCK
 *          3 -> O_CREATE | O_LOCK -> O_LOCKED_CREATE
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int openFile(const char *pathname, int flags);

/* EFFETTO: richiede al server di leggere tutto il contenuto del file indicato da "pathname",
 *          ritorna in "buf" l'area sullo heap di dimensione "size".
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int readFile(const char *pathname, void **buf, size_t *size);

/* EFFETTO: richiede al server di leggere N file qualsiasi, se ce ne sono meno li legge tutti.
 *          Se N <= 0 si leggono tutti i file memorizzati.
 *
 * RITORNA: -1 -> errore settando errno
 *           numero >= 0 -> numero di file letti
 */
int readNFiles(int N, const char *dirname);

/* EFFETTO: richiede al server di scrivere il file puntato da "pathname" nel file server.
 *          Se "dirname" != NULL il file eventualmente espulso dal server perchè pieno andrà inserito li.
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int writeFile(const char *pathname, const char *dirname);

/* EFFETTO: richiede al server di scrivere in coda al file indicato da "pathname" i "size" bytes contenuti in "buf"
 *          Se "dirname" != NULL il file eventualmente espulso dal server perchè pieno andrà inserito li.
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int appendToFile(const char* pathname, void* buf, size_t size, const char *dirname);

/* EFFETTO: richiede al server di settare O_LOCK al file puntato da "pathname".
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int lockFile(const char *pathname);

/* EFFETTO: richiede al server di resettare O_LOCK al file puntato da "pathname".
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int unlockFile(const char *pathname);

/* EFFETTO: richiede al server di chiudere il file puntato da "pathname"-
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int closeFile(const char *pathname);

/* EFFETTO: richiede al server di rimuovere il file puntato da "pathname" dal server.
 *
 * RITORNA: -1 -> errore settando errno
 *           0 -> terminazione con successo
 */
int removeFile(const char *pathname);

#endif