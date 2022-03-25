#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <stdbool.h>

#include "fileParser.h"
#include "utils.h"
#include "icl_hash.h"
#include "message.h"
#include "codaTask.h"
#include "codaFile.h"

//puntatore alla struttura contentente le informazioni del file di config
ConfigFile *newConfigFile;

//puntatore alla struttura contenente lo stato del programma
ProgramStatus currentStatus = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
pthread_mutex_t mtx_status = PTHREAD_MUTEX_INITIALIZER;

//puntatore al file di log
FILE *file_log;
pthread_mutex_t mtx_file_log = PTHREAD_MUTEX_INITIALIZER;

// STORAGE:

//hashtable in cui sono salvati i file
icl_hash_t *hash_table = NULL;
//lista in cui sono salvati i fie
QueueFile *coda_file = NULL;
//mutex che regola l'accesso al server in maniera sincrona
pthread_mutex_t mtx_storage_access = PTHREAD_MUTEX_INITIALIZER;

//CODA TASK: coda in cui sono salvati gli fd dei client da servire:
QueueTask *coda_task = NULL;

//segnali di chiusura
volatile sig_atomic_t softExit = 0;
volatile sig_atomic_t hardExit = 0;

/*
 * EFFETTO: funzione che si mette in attesa di un segnale per la terminazione del server
 */
void signalWait(int signal){
    if(signal == SIGINT || signal == SIGQUIT){ // se ricevo SIGINT o SIGQUIT termino immediatamente il server.
        hardExit = 1;
    }
    if(signal == SIGHUP){ // se ricevo SIGHUP concludo le operazioni in corso e termino il server.
        softExit = 1;
    }
}

//funzione interna che controlla il numero di file:
// 1 -> numero massimo di file raggiunti
// 0 -> altrimenti
int checkMaxNumFile(){
    if(currentStatus.num_current_file_saved > newConfigFile->numFile){
        return 1;
    }
    else{
        return 0;
    }
}

//funzione interna che controlla il numero di byte salvati:
// 1 -> in caso di inserimento si sforano i byte massimi
// 0 -> altrimenti
int checkMaxNumByte(int byteToAdd){
    if(currentStatus.num_current_bytes + byteToAdd > newConfigFile->capacity){
        return 1;
    }
    else{
        return 0;
    }
}

//funzione interna che stampa sul log file un eventuale errore specificando il client su cui avviene:
void printErrorOnLogFile(char *errore, int num_client){
    LOCK("mtx_file_log", mtx_file_log);
    fprintf(file_log, "\tERRORE: %s sul client %d.\n", errore, num_client);
    UNLOCK("mtx_file_log", mtx_file_log);
}

/*
 * EFFETTO: funzione worker che processa una singola richiesta di un client
 */
void *worker(void *args){
    int comPipe = ((int*)args)[0];   //fd della pipe di comunicazione per la conclusione del task
    int num_worker = ((int*)args)[1];//intero che identifica il numero del worker
    free(args);

    printf("Avvio worker numero %d con pipe %d\n", num_worker, comPipe);

    //inizializzo i messaggi:
    Msg* to_send = smalloc(sizeof(Msg));
    Msg* to_receive = smalloc(sizeof(Msg));

    while(1){
        if(softExit || hardExit){ //controllo se le variabli di uscita sono entrambi negative
            printf("Worker numero %d terminato\n", num_worker);
            free(to_receive);
            free(to_send);
            return (void *)NULL;
        }

        //rimuovo dalla coda il fd da servire
        int socket_con = dequeueTask(coda_task);

        //nel caso di un fd == -1 ricomincio il ciclo per controllare un eventuale uscita dal ciclo:
        if(socket_con == -1){
            continue;
        }

        //ricevo il messaggio dal client:
        if(receiveMsg(to_receive, socket_con) == -1){
            fprintf(stderr, "ERRORE: receiveMsg.\n");
            return (void *)-1;
        }
        
        //stampo sul logfile l'operazione con i suoi argomenti
        LOCK("mtx_file_log", mtx_file_log);
        if(to_receive->opt_code == CLOSE_CONNECTION){
            fprintf(file_log, "\nOperazione di tipo '%s' del client %d.\n", convertOptCodeInChar(to_receive->opt_code), socket_con);
        }
        else{
            fprintf(file_log, "\nOperazione di tipo '%s' del client %d:\n", convertOptCodeInChar(to_receive->opt_code), socket_con);
        }

        if(to_receive->opt_code != READ_N_FILE && to_receive->opt_code != CLOSE_CONNECTION){
            fprintf(file_log, "\tNome file: %s.\n", to_receive->path);
        }
        UNLOCK("mtx_file_log", mtx_file_log);

        //incremento il numero di richieste eseguite dal worker:
        currentStatus.workerRequests[num_worker]++;

        //inizializzo il messaggio da inviare sulla pipe di chiusura:
        char pipeInput[RESPONSE_CODE_LENGTH];
        //scrivo sul messaggio da inviare sulla pipe il fd del client che sto servendo:
        snprintf(pipeInput, RESPONSE_CODE_LENGTH, "%04d", socket_con);

        //blocco l'accesso all'hash table e estraggo
        LOCK("mtx_storage_access", mtx_storage_access);
        File* old_file = icl_hash_find(hash_table, to_receive->path);

        //chiusura della connessione:
        if(to_receive->opt_code == CLOSE_CONNECTION){
            UNLOCK("mtx_storage_access",mtx_storage_access);
            //scrivo dentro to_send le info del file:
            to_send->opt_code = CLOSE_CONNECTION;

            //imposto codice di risposta per il client di operazione avvenuta con successo:
            setMsg(to_send, 0, 0, "ok", "", NULL, 0);
            //imposto il codice che permette di chiudere la comunicazione sull'input del pipe:
            strcpy(pipeInput, "c");

            LOCK("mtx_file_log", mtx_file_log);
            fprintf(file_log, "Operazione di tipo '%s' del client %d svolta correttamente.\n", convertOptCodeInChar(to_receive->opt_code), socket_con);
            UNLOCK("mtx_file_log", mtx_file_log);

            if(sendMsg(to_send, socket_con) == -1){ 
                fprintf(stderr, "ERRORE: sendMsg.\n");
                return (void *)-1;
            }

            if(write(comPipe, pipeInput, RESPONSE_CODE_LENGTH) == -1){
                fprintf(stderr ,"ERRORE: write pipe.");
                return (void *)-1;
            }

            close(socket_con);
            continue;
        }

        //switch sul tipo di operazione da fare:
        switch(to_receive->opt_code){
            case OPEN_FILE: {
                LOCK("mtx_file_log", mtx_file_log);
                fprintf(file_log, "\tFlags: %s.\n", convertOptCodeInChar(to_receive->flags));
                UNLOCK("mtx_file_log", mtx_file_log);
                
                if(to_receive->flags == O_NOFLAG || to_receive->flags == O_LOCK){ //open file
                    //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                    if(old_file == NULL){
                        UNLOCK("mtx_storage_access",mtx_storage_access);
                        fprintf(stderr, "Operazione non possibile: apertura di un file inesistente\n");
                        printErrorOnLogFile("file inesistente", socket_con);
                        setMsg(to_send, -1, 0, "ne","", NULL, 0);
                        break;
                    }

                    if(old_file->openBy != -1){
                        UNLOCK("mtx_storage_access",mtx_storage_access);
                        fprintf(stderr, "Operazione non possibile: file già aperto\n");
                        printErrorOnLogFile("file già aperto", socket_con);
                        setMsg(to_send, -1, 0, "ao","", NULL, 0);
                        break;
                    }
                    
                    if(old_file->lockedBy != to_receive->pid && old_file->lockedBy != 0){
                        UNLOCK("mtx_storage_access",mtx_storage_access);
                        fprintf(stderr, "Operazione non possibile: apertura di un file bloccato da parte di un utente non abilitato\n");
                        printErrorOnLogFile("file non accessibile", socket_con);
                        setMsg(to_send, -1, 0, "ao","", NULL, 0);
                        break;
                    }
                        
                    //apertura del file:
                    old_file->openBy = to_receive->pid;
                    //setto il lock del file in base alla flag:
                    if(to_receive->flags == O_LOCK){
                        old_file->lockedBy = to_receive->pid;
                    }
                    UNLOCK("mtx_storage_access",mtx_storage_access);

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    if(to_receive->flags == O_LOCK){
                        currentStatus.num_file_locked++;
                    }
                    currentStatus.num_file_open++;
                    currentStatus.num_current_file_open++;
                    UNLOCK("mtx_status", mtx_status);

                    //imposto codice di risposta per il client di operazione avvenuta con successo:
                    setMsg(to_send, 0, 0, "ok", "", NULL, 0);
                    break;
                }
                else if(to_receive->flags == O_CREATE || to_receive->flags == O_LOCKED_CREATE){ //create file
                    //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                    if(old_file != NULL){
                        UNLOCK("mtx_storage_access",mtx_storage_access);
                        printErrorOnLogFile("file esistente", socket_con);
                        setMsg(to_send, -1, 0, "ae", "", NULL, 0);
                        break;
                    }

                    //creo il nuovo file da inserire nel server:
                    File* new_file = smalloc(sizeof(File));
                    if(to_receive->flags == O_CREATE){
                        new_file->lockedBy = 0;
                    }
                    else{
                        new_file->lockedBy = to_receive->pid;
                    }

                    new_file->openBy = to_receive->pid;
                    strcpy(new_file->pathname, to_receive->path);
                    
                    if(icl_hash_insert(hash_table, new_file->pathname, new_file) == NULL){
                        fprintf(stderr, "ERRORE: insert hash table\n");
                        return (void *)-1;
                    }
                    //inserisco il nome del file nella coda dei file:
                    enqueue(coda_file, new_file);
                    UNLOCK("mtx_storage_access",mtx_storage_access);

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_current_file_saved++;
                    currentStatus.num_file_create++;
                    currentStatus.num_file_open++;
                    currentStatus.num_current_file_open++;
                    if(to_receive->flags == O_LOCKED_CREATE){
                        currentStatus.num_file_locked++;
                    }
                    UNLOCK("mtx_status", mtx_status);

                    //imposto codice di risposta per il client di operazione avvenuta con successo:
                    setMsg(to_send, 0, 0, "ok", "", NULL, 0);
                }
                else{//se la flag è errata imposto il codice di uscita per il client apposito
                    setMsg(to_send, 0, 0, "fe", "", NULL, 0);
                    fprintf(stderr, "ERRORE: flag errata");
                }
                break;
            }
            case READ_FILE: {
                //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                if(old_file == NULL){    
                    fprintf(stderr, "Operazione non possibile: apertura di un file inesistente\n");
                    printErrorOnLogFile("file inesistente", socket_con);
                    setMsg(to_send, -1, 0, "ne","", NULL, 0);
                    break;
                }

                if(old_file->openBy == -1){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file chiuso\n");
                    printErrorOnLogFile("file chiuso", socket_con);
                    setMsg(to_send, -1, 0, "no","", NULL, 0);
                    break;
                }

                //metto sul messaggio di risposta il file richiesto dal client:
                setMsg(to_send, 0, 0, "ok",old_file->pathname ,old_file->data, old_file->data_length);
                UNLOCK("mtx_storage_access", mtx_storage_access);

                //aggiorno le statistiche del server:
                LOCK("mtx_status", mtx_status);
                currentStatus.num_file_readed++;
                UNLOCK("mtx_status", mtx_status);

                LOCK("mtx_file_log", mtx_file_log);
                fprintf(file_log, "\tNumero di byte letti: %d.\n", to_send->data_length);
                UNLOCK("mtx_file_log", mtx_file_log);
                break;
            }
            case READ_N_FILE:{//(uso flag come numero N di file da leggere)
                if(currentStatus.num_current_file_saved == 0){
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    setMsg(to_send, 0, 0, "es","", NULL, 0);

                    if(sendMsg(to_send, socket_con) == -1){
                        fprintf(stderr, "ERRORE: sendMsg.\n");
                    }

                    break;
                }

                //creo la coda temporanea dove inserire i file da leggere:
                QueueFile *file_to_send = create();
                
                int num_file_letti=0, bytes_letti=0;
                nodoFile* currentPtr = coda_file->head; // prendo il file in testa alla coda
                File *file = icl_hash_find(hash_table, (void *)currentPtr->key); // lo cerco nella hash_table

                //N è passato dal client nel campo flags
                while(num_file_letti < to_receive->flags || to_receive->flags == 0){ // ciclo per leggere N file (se N == 0 ciclo infinito)
                    if(currentPtr == NULL){ // se il puntatore è NULL non ho più file da leggere
                        break;
                    }

                
                    if(file->openBy != -1){ // se il file è già aperto passo a leggere il successivo
                        currentPtr = currentPtr->next;
                        if(currentPtr != NULL){
                            file = icl_hash_find(hash_table, (void *)currentPtr->key);
                        }
                        continue;
                    }
                    
                    //apro il file:
                    file->openBy = to_receive->pid;

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_file_open++;
                    currentStatus.num_current_file_open++;
                    UNLOCK("mtx_status", mtx_status);

                    if(file->data != NULL){ // se il contenuto del file è leggibile allora lo inserisco nella coda dei file da inviare
                        File *to_add = smalloc(sizeof(File));
                        to_add->data_length = file->data_length;
                        to_add->lockedBy = file->lockedBy;
                        to_add->openBy = file->openBy;
                        strcpy(to_add->pathname, file->pathname);
                        to_add->data = smalloc(sizeof(char)*to_add->data_length);
                        memcpy(to_add->data, file->data, file->data_length);

                        enqueue(file_to_send, to_add);
                        //aggiorno il numero di file e di byte letti
                        bytes_letti += file->data_length;
                        num_file_letti++;

                        //aggiorno il file di log:
                        LOCK("mtx_file_log", mtx_file_log);
                        fprintf(file_log, "\nFile letto con operazione di tipo '%s':\n", convertOptCodeInChar(to_receive->opt_code));
                        fprintf(file_log, "\tNome file: %s.\n", to_add->pathname);
                        fprintf(file_log, "\tNumero di byte letti: %ld.\n", to_add->data_length);
                        UNLOCK("mtx_file_log", mtx_file_log);
                    }

                    //chiudo il file:
                    file->openBy = -1;

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_file_closed++;
                    currentStatus.num_current_file_open--;
                    UNLOCK("mtx_status", mtx_status);

                    if(currentPtr->next == NULL){
                        break;
                    }

                    //leggo il file successivo:
                    currentPtr = currentPtr->next;
                    file = icl_hash_find(hash_table, (void *)currentPtr->key);
                }
                UNLOCK("mtx_storage_access", mtx_storage_access);

                //ciclo che svuota la coda temporanea e invia i file al client uno a uno:
                bool loop = true;
                do{
                    File *currentFile = dequeue(file_to_send);

                    if(currentFile == NULL){//se non ho più file invio il messaggio "nmf" e termino il ciclo
                        setMsg(to_send, 0, 0, "nmf","", NULL, 0);
                        loop = false;
                    }
                    else{
                        setMsg(to_send, 0, 0, "ok", currentFile->pathname, currentFile->data, currentFile->data_length);
                    }

                    // invio il messaggio dove dico se dovrò o meno espellere file
                    if(sendMsg(to_send, socket_con) == -1){ 
                        fprintf(stderr, "ERRORE: sendMsg.\n");
                        return (void *)-1;
                    }

                    if(currentFile != NULL){
                        free(currentFile->data);
                        free(currentFile);
                    }
                }
                while(loop);

                //distruggo la coda temporanea
                destroyQueue(file_to_send);

                //aggiorno le statistiche del server:
                LOCK("mtx_status", mtx_status);
                currentStatus.num_file_readed += num_file_letti;
                UNLOCK("mtx_status", mtx_status);

                setMsg(to_send, 0, 0, "ok","", NULL, 0);
                break;
            }
            case WRITE_FILE:{
                //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                if(old_file == NULL){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file inesistente.\n");
                    printErrorOnLogFile("file inesistente", socket_con);
                    setMsg(to_send, -1, 0, "ne","", NULL, 0);
                    break;
                }

                if(old_file->openBy == -1){
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    fprintf(stderr, "Operazione non possibile: file chiuso\n");
                    printErrorOnLogFile("file chiuso", socket_con);
                    setMsg(to_send, -1, 0, "no","", NULL, 0);
                    break;
                }

                if(old_file->data_length != 0){
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    fprintf(stderr, "Operazione non possibile: scrittura su file non vuoto\n");
                    printErrorOnLogFile("file già scritto", socket_con);
                    setMsg(to_send, -1, 0, "aw","", NULL, 0);
                    break;
                }

                if(old_file->lockedBy != to_receive->pid && old_file->lockedBy != 0){
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    fprintf(stderr, "Operazione non possibile: scrittura su un file bloccato da parte di un utente non abilitato\n");
                    printErrorOnLogFile("file inaccessibile", socket_con);
                    setMsg(to_send, -1, 0, "l","", NULL, 0);
                    break;
                }

                if(to_receive->data_length > newConfigFile->capacity){
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    fprintf(stderr, "Operazione non possibile: file troppo grande per essere salvato.\n");
                    printErrorOnLogFile("file troppo grande", socket_con);
                    setMsg(to_send, -1, 0, "tb","", NULL, 0);
                    break;
                }

                //controllo di non superare la capacità del server:
                bool loop;
                if(checkMaxNumByte(to_receive->data_length) == 1){
                    setMsg(to_send, 0, 0, "ef", "", NULL, 0);
                    loop = true;
                }
                else{
                    setMsg(to_send, 0, 0, "ok", "", NULL, 0);
                    loop = false;
                }
                
                // invio il messaggio dove dico se dovrò o meno espellere file
                if(sendMsg(to_send, socket_con) == -1){ 
                    fprintf(stderr, "ERRORE: sendMsg.\n");
                    return (void *)-1;
                }

                //creo la coda temporanea dove inserire i file da spedire:
                QueueFile *file_to_send = create();

                while(checkMaxNumByte(to_receive->data_length) == 1){// se devo espellere file
                    //elimino il file dal server secondo la politica FIFO:
                    File *file = dequeue(coda_file);

                    //se cerco di espellere lo stesso file inserito lo reinserisco ( in coda )
                    if(strcmp(file->pathname, old_file->pathname) == 0){
                        enqueue(coda_file, file);
                        continue;
                    }

                    if(icl_hash_delete(hash_table, file->pathname, NULL, NULL) == -1){
                        fprintf(stderr, "ERRORE: delete hash table\n");
                        return (void *)-1;
                    }

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_file_deleted++;
                    currentStatus.num_current_bytes = currentStatus.num_current_bytes - file->data_length;
                    UNLOCK("mtx_status", mtx_status);

                    LOCK("mtx_file_log", mtx_file_log);
                    fprintf(file_log, "\tServer pieno per capacità, rimpiazzo il file '%s'.\n", to_send->path);
                    fprintf(file_log, "\tNumero di byte eliminati: %d.\n", to_send->data_length);
                    UNLOCK("mtx_file_log", mtx_file_log);

                    enqueue(file_to_send, file);
                }

                //svuoto la coda temporanea contenente tutti i file da espulsi:
                while(loop){
                    File *currentFile = dequeue(file_to_send);

                    if(currentFile == NULL){
                        setMsg(to_send, 0, 0, "ne","", NULL, 0);
                        loop = false;
                    }
                    else{
                        setMsg(to_send, 0, 0, "ok", currentFile->pathname, currentFile->data, currentFile->data_length);
                    }

                    // invio il messaggio dove dico se dovrò o meno continuare ad inviare altri file
                    if(sendMsg(to_send, socket_con) == -1){ 
                        fprintf(stderr, "ERRORE: sendMsg.\n");
                        return (void *)-1;
                    }

                    if(currentFile != NULL){
                        free(currentFile->data);
                        free(currentFile);
                    }
                }

                destroyQueue(file_to_send);

                //controllo di non superare il numero massimo di file nel server:
                if(checkMaxNumFile() == 1){
                    //elimino il file dal server secondo la politica FIFO:
                    File *tmp = dequeue(coda_file);

                    if(icl_hash_delete(hash_table, tmp->pathname, NULL, NULL) == -1){
                        fprintf(stderr, "ERRORE: delete hash table\n");
                        return (void *)-1;
                    }
                    

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_file_deleted++;
                    currentStatus.num_current_file_saved--;
                    currentStatus.num_current_bytes = currentStatus.num_current_bytes - tmp->data_length;
                    UNLOCK("mtx_status", mtx_status);

                    //scrivo dentro to_send le info del file espulso:
                    setMsg(to_send, 0, 0, "ok", tmp->pathname, tmp->data, tmp->data_length);

                    LOCK("mtx_file_log", mtx_file_log);
                    fprintf(file_log, "\tServer pieno per numero di file, rimpiazzo il file '%s'.\n", to_send->path);
                    fprintf(file_log, "\tNumero di byte eliminati: %d.\n", to_send->data_length);
                    UNLOCK("mtx_file_log", mtx_file_log);

                    //elimino il file:
                    free(tmp->data);
                    free(tmp);
                }
                else{
                    //scrivo dentro to_send le info:
                    setMsg(to_send, 0, 0, "ok","", NULL, 0);
                }

                //scrivo il file
                old_file->data_length = to_receive->data_length;
                old_file->data = smalloc(sizeof(char)*to_receive->data_length);
                memcpy(old_file->data, to_receive->data, to_receive->data_length);
                free(to_receive->data);
                UNLOCK("mtx_storage_access",mtx_storage_access);

                //aggiorno le statistiche del server:
                LOCK("mtx_status", mtx_status);
                if(currentStatus.num_max_file_reached <= currentStatus.num_current_file_saved && currentStatus.num_current_file_saved <= newConfigFile->numFile){
                    currentStatus.num_max_file_reached = currentStatus.num_current_file_saved;
                }
                currentStatus.num_file_writed++;
                currentStatus.num_current_bytes += to_receive->data_length;
                if(currentStatus.num_max_byte_reached < currentStatus.num_current_bytes){
                    currentStatus.num_max_byte_reached = currentStatus.num_current_bytes;
                }
                UNLOCK("mtx_status", mtx_status);

                LOCK("mtx_file_log", mtx_file_log);
                fprintf(file_log, "\tNumero di byte scritti: %d.\n", to_receive->data_length);
                UNLOCK("mtx_file_log", mtx_file_log);
                break;
            }
            case APPEND_TO_FILE:{
                //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                if(old_file == NULL){
                    UNLOCK("mtx_storage_access",mtx_storage_access); 
                    fprintf(stderr, "Operazione non possibile: file inesistente\n");
                    printErrorOnLogFile("file inesistente", socket_con);
                    setMsg(to_send, -1, 0, "ne","", NULL, 0);
                    break;
                }

                if(old_file->openBy == -1){
                    UNLOCK("mtx_storage_access",mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file chiuso\n");
                    printErrorOnLogFile("file chiuso", socket_con);
                    setMsg(to_send, -1, 0, "no","", NULL, 0);
                    break;
                }

                if(old_file->lockedBy != to_receive->pid && old_file->lockedBy != 0){
                    UNLOCK("mtx_storage_access",mtx_storage_access);
                    fprintf(stderr, "Operazione non possibile: scrittura su un file bloccato da parte di un utente non abilitato\n");
                    printErrorOnLogFile("file inaccessibile", socket_con);
                    setMsg(to_send, -1, 0, "l","", NULL, 0);
                    break;
                }

                if(to_receive->data_length+old_file->data_length > newConfigFile->capacity){
                    UNLOCK("mtx_storage_access",mtx_storage_access);
                    fprintf(stderr, "Operazione non possibile: file troppo grande per essere salvato.\n");
                    printErrorOnLogFile("file troppo grande", socket_con);
                    setMsg(to_send, -1, 0, "tb","", NULL, 0);
                    break;
                }

                //controllo di non superare la capacità del server:
                bool loop;
                if(checkMaxNumByte(to_receive->data_length) == 1){
                    setMsg(to_send, 0, 0, "ef", "", NULL, 0);
                    loop = true;
                }
                else{
                    setMsg(to_send, 0, 0, "ok", "", NULL, 0);
                    loop = false;
                }
                
                // invio il messaggio dove dico se dovrò o meno espellere file
                if(sendMsg(to_send, socket_con) == -1){ 
                    fprintf(stderr, "ERRORE: sendMsg.\n");
                    return (void *)-1;
                }

                QueueFile *file_to_send = create();

                while(checkMaxNumByte(to_receive->data_length) == 1){// se devo espellere file
                    //elimino il file dal server secondo la politica FIFO:
                    File *file = dequeue(coda_file);

                    if(strcmp(file->pathname, old_file->pathname) == 0){
                        enqueue(coda_file, file);
                        continue;
                    }

                    if(icl_hash_delete(hash_table, file->pathname, NULL, NULL) == -1){
                        fprintf(stderr, "ERRORE: delete hash table\n");
                        return (void *)-1;
                    }

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_file_deleted++;
                    currentStatus.num_current_bytes = currentStatus.num_current_bytes - file->data_length;
                    UNLOCK("mtx_status", mtx_status);


                    LOCK("mtx_file_log", mtx_file_log);
                    fprintf(file_log, "\tServer pieno per capacità, rimpiazzo il file '%s'.\n", to_send->path);
                    fprintf(file_log, "\tNumero di byte eliminati: %d.\n", to_send->data_length);
                    UNLOCK("mtx_file_log", mtx_file_log);

                    enqueue(file_to_send, file);
                }

                while(loop){
                    File *currentFile = dequeue(file_to_send);

                    if(currentFile == NULL){
                        setMsg(to_send, 0, 0, "ne","", NULL, 0);
                        loop = false;
                    }
                    else{
                        setMsg(to_send, 0, 0, "ok", currentFile->pathname, currentFile->data, currentFile->data_length);
                    }

                    // invio il messaggio dove dico se dovrò o meno espellere file
                    if(sendMsg(to_send, socket_con) == -1){ 
                        fprintf(stderr, "ERRORE: sendMsg.\n");
                        return (void *)-1;
                    }

                    if(currentFile != NULL){
                        free(currentFile->data);
                        free(currentFile);
                    }
                }

                destroyQueue(file_to_send);

                //modifiche file:
                void *data = smalloc(old_file->data_length+to_receive->data_length);
                memcpy(data, old_file->data, old_file->data_length);
                memcpy((char *)data+(old_file->data_length), to_receive->data, to_receive->data_length);
                free(old_file->data);
                old_file->data = data;
                old_file->data_length = old_file->data_length+to_receive->data_length;
                free(to_receive->data);
                UNLOCK("mtx_storage_access",mtx_storage_access);

                //aggiorno le statistiche del server:
                LOCK("mtx_status", mtx_status);
                currentStatus.num_file_writed++;
                currentStatus.num_current_bytes += to_receive->data_length;
                if(currentStatus.num_max_byte_reached < currentStatus.num_current_bytes){
                    currentStatus.num_max_byte_reached = currentStatus.num_current_bytes;
                }
                UNLOCK("mtx_status", mtx_status);
                
                LOCK("mtx_file_log", mtx_file_log);
                fprintf(file_log, "\tNumero di byte scritti: %d.\n", to_receive->data_length);
                UNLOCK("mtx_file_log", mtx_file_log);

                //imposto codice di risposta per il client di operazione avvenuta con successo:
                setMsg(to_send, 0, 0, "ok","", NULL, 0);
                break;
            }
            case LOCK_FILE:{
                //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                if(old_file == NULL){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file inesistente\n");
                    printErrorOnLogFile("file inesistente", socket_con);
                    setMsg(to_send, -1, 0, "ne","", NULL, 0);
                    break;
                }

                if(old_file->openBy == -1){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file chiuso\n");
                    printErrorOnLogFile("file chiuso", socket_con);
                    setMsg(to_send, -1, 0, "no","", NULL, 0);
                    break;
                }
                
                if(old_file->lockedBy == 0){              //se il file non era in stato di lock
                    old_file->lockedBy = to_receive->pid;        //imposto lo stato di lock
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    setMsg(to_send, 0, 0, "ok","", NULL, 0);

                    //aggiorno le statistiche del server:
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_file_locked++;
                    UNLOCK("mtx_status", mtx_status);
                }
                else{                                      //se il file era già in stato di lock
                    if(old_file->lockedBy == to_receive->pid){  //se sono il propietario della lock allora "ok"
                        UNLOCK("mtx_storage_access", mtx_storage_access);
                        setMsg(to_send, 0, 0, "ok","", NULL, 0);
                    }
                    else{                                   //altrimenti attendo finchè non si libera lo stato di lock
                        UNLOCK("mtx_storage_access", mtx_storage_access);
                        setMsg(to_send, -1, 0, "l","", NULL, 0);
                        enqueueTask(coda_task, socket_con);
                    }
                }

                break;
            }
            case UNLOCK_FILE:{
                //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                if(old_file == NULL){
                    UNLOCK("mtx_storage_access",mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file inesistente\n");
                    printErrorOnLogFile("file inesistente", socket_con);
                    setMsg(to_send, -1, 0, "ne","", NULL, 0);
                    break;
                }

                if(old_file->openBy == -1){
                    UNLOCK("mtx_storage_access",mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file chiuso\n");
                    printErrorOnLogFile("file chiuso", socket_con);
                    setMsg(to_send, -1, 0, "no","", NULL, 0);
                    break;
                }
                
                if(old_file->lockedBy == 0){
                    UNLOCK("mtx_storage_access",mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file non bloccato\n");
                    printErrorOnLogFile("file non bloccato", socket_con);
                    setMsg(to_send, -1, 0, "au","", NULL, 0);
                    break;
                }

                if(old_file->lockedBy != to_receive->pid && old_file->lockedBy != 0){
                    UNLOCK("mtx_storage_access",mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file impossibile sbloccare un file se non si è gli owner\n");
                    printErrorOnLogFile("file inaccessibile", socket_con);
                    setMsg(to_send, -1, 0, "ldo","", NULL, 0);
                    break;
                }

                //rimuovo la lock dal file
                old_file->lockedBy = 0;
                UNLOCK("mtx_storage_access",mtx_storage_access);
                
                //aggiorno le statistiche del server:
                LOCK("mtx_status", mtx_status);
                currentStatus.num_file_locked--;
                UNLOCK("mtx_status", mtx_status);

                setMsg(to_send, 0, 0, "ok","", NULL, 0);
                break;
            }
            case CLOSE_FILE:{
                //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                if(old_file == NULL){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file inesistente\n");
                    printErrorOnLogFile("file inesistente", socket_con);
                    setMsg(to_send, -1, 0, "ne","", NULL, 0);
                    break;
                }

                if(old_file->openBy == -1){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file già chiuso.\n");
                    printErrorOnLogFile("file già chiuso", socket_con);
                    setMsg(to_send, -1, 0, "ac","", NULL, 0);
                    break;
                }

                if(old_file->openBy != to_receive->pid){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file aperto da un'altra persona\n");
                    printErrorOnLogFile("file già aperto", socket_con);
                    setMsg(to_send, -1, 0, "odo","", NULL, 0);
                    break;
                }

                old_file->openBy = -1;
                UNLOCK("mtx_storage_access", mtx_storage_access);

                //aggiorno le statistiche del server:
                LOCK("mtx_status", mtx_status);
                currentStatus.num_current_file_open--;
                currentStatus.num_file_closed++;
                UNLOCK("mtx_status", mtx_status);

                setMsg(to_send, 0, 0, "ok","", NULL, 0);
                break;
            }
            case REMOVE_FILE:{
                //controlli sul file che in caso di errore stampano lato client e impostano codice di uscita del relativo errore:
                if(old_file == NULL || old_file->pathname == NULL){
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    printErrorOnLogFile("file inesistente", socket_con);
                    fprintf(stderr, "Operazione non possibile: file inesistente\n");
                    setMsg(to_send, -1, 0, "ne","", NULL, 0);
                    break;
                }

                if(old_file->lockedBy == 0){
                    UNLOCK("mtx_storage_access", mtx_storage_access);
                    fprintf(stderr, "Operazione non possibile: rimozione file non in lock\n");
                    printErrorOnLogFile("file non bloccato", socket_con);
                    setMsg(to_send, -1, 0, "dfu","", NULL, 0);
                    break;
                }

                if(old_file->lockedBy != to_receive->pid){
                    UNLOCK("mtx_storage_access", mtx_storage_access);    
                    fprintf(stderr, "Operazione non possibile: file in lock da parte di un altro processo\n");
                    printErrorOnLogFile("file già in blocco", socket_con);
                    setMsg(to_send, -1, 0, "ldo","", NULL, 0);
                    break;
                }

                //copio in una stringa il pathname del file da eliminare e la sua grandezza:
                char delete[MAX_LENGTH];
                strcpy(delete, old_file->pathname);
                int data_length_deleted = old_file->data_length;
                
                //elimino il file dalla hash_table
                if(icl_hash_delete(hash_table, delete, NULL, NULL) == -1){
                    fprintf(stderr, "ERRORE: delete element in hashtable\n");
                    return (void *)-1;
                }

                //elimino il file dalla coda
                if(removeElementFromQueue(coda_file, (void *)delete) == -1){
                    fprintf(stderr, "ERRORE: rimozione elemento dalla coda dei pathname\n");
                    return (void *)-1;
                }

                UNLOCK("mtx_storage_access", mtx_storage_access);

                //aggiorno le statistiche del server:
                LOCK("mtx_status", mtx_status);
                currentStatus.num_file_deleted++;
                currentStatus.num_file_closed++;
                currentStatus.num_current_file_saved--;
                currentStatus.num_current_bytes = currentStatus.num_current_bytes - data_length_deleted;
                UNLOCK("mtx_status", mtx_status);

                setMsg(to_send, 0, 0, "ok","", NULL, 0);
                break;
            }
            default:{
                fprintf(stderr, "ERRORE: tipo dell'operazione non valido\n");
                setMsg(to_send, -1, 0, "onv","", NULL, 0);
            }
        }

        if(strcmp(to_send->response_code, "ok") == 0){
            LOCK("mtx_file_log", mtx_file_log);
            fprintf(file_log, "Operazione di tipo %s del client %d svolta correttamente.\n", convertOptCodeInChar(to_receive->opt_code), socket_con);
            UNLOCK("mtx_file_log", mtx_file_log);
        }

        //invio utlimo messaggio di notifica:
        if(sendMsg(to_send, socket_con) == -1){ 
            fprintf(stderr, "ERRORE: sendMsg.\n");
            return (void *)-1;
        }

        //invio sulla pipe il messaggio di closeConnectio 'c' oppure il fd socket_con
        if(write(comPipe, pipeInput, RESPONSE_CODE_LENGTH) == -1){
            fprintf(stderr ,"ERRORE: write pipe.");
            return (void *)-1;
        }
    }

    return (void *)NULL;
}


int main(int argc, char *argv[]){
    //controllo che venga passato un solo num_worker, cioè il file di config:
    if(argc != 2){
        printf("Argomenti incorretti, usare ./server config.txt\n");
        return -1;
    }

    //passo il pathname alla funzione parseFile che resituisce una lista con il file parsato all'interno:
    ParsedFile *config = parseFile(argv[1], "=");

    //la lista viene letta in maniera opportuna e viene inserito nella struct ConfigFile tutte le informazioni utili al server:
    newConfigFile = configFileCheck(config);

    //libero la memoria del file parsato:
    deleteParsedFile(config);

    if(!newConfigFile){//controllo che il file di Config sia stato letto correttamente
        fprintf(stderr, "File di config non letto correttamente.\n");
        return -1;
    }
    //gestisco i segnali:
    
    //ignoro SIGPIPE:
    struct sigaction sig_ign;
    memset(&sig_ign, 0, sizeof(sig_ign));
    sig_ign.sa_handler = SIG_IGN;
    CHECK_FUNCTION("sigaction SIGPIPE", sigaction(SIGPIPE, &sig_ign, NULL), == , -1);
    
    //imposto i segnali custom:
    struct sigaction sig_handler;
    memset(&sig_handler, 0, sizeof(sig_handler));
    sig_handler.sa_handler = signalWait;
    sigset_t handler_mask;

    CHECK_FUNCTION("sigemptyset", sigemptyset(&handler_mask), ==, -1);
    CHECK_FUNCTION("sigaddset SIGINT", sigaddset(&handler_mask, SIGINT), == , -1);
    CHECK_FUNCTION("sigaddset SIGQUIT", sigaddset(&handler_mask, SIGQUIT), == , -1);
    CHECK_FUNCTION("sigaddset SIGHUP", sigaddset(&handler_mask, SIGHUP), == , -1);
    sig_handler.sa_mask = handler_mask;

    CHECK_EINTR("sigaction SIGINT", sigaction(SIGINT ,&sig_handler, NULL));
    CHECK_EINTR("sigaction SIGQUIT", sigaction(SIGQUIT ,&sig_handler, NULL));
    CHECK_EINTR("sigaction SIGHUP", sigaction(SIGHUP ,&sig_handler, NULL));

    //creazione tabella hash per salvare i file:
    hash_table = icl_hash_create(newConfigFile->numFile / 10 + 1, NULL, NULL);
    CHECK_NULL("ERRORE: creazione hash table", hash_table);

    //creazione coda per i file:
    coda_file = create();
    CHECK_NULL("ERRORE: creazione coda file", coda_file);
    
    //creazione coda per i task:
    coda_task = createQueueTask(MAX_LENGTH);
    CHECK_NULL("ERRORE: creazione coda task", coda_task);

    //apertura del file di log in scrittura:
    file_log = fopen(newConfigFile->fileLogName, "w+");
    CHECK_NULL("fopen fileLog", file_log);
    fprintf(file_log, "FILE DI LOG:\n");
    
    //CONNESSIONE:
    
    // inizializzo la pipe per la comunicazione tra worker e main thread:
    char pipeBuffer[RESPONSE_CODE_LENGTH];
    int comunicationPipe[2];
    CHECK_FUNCTION("pipe", pipe(comunicationPipe), ==, -1);

    //1. SOCKET (creazione del socket)
    int fd_sk, fd_max = 0;
    SYSCALL_EXIT("socket", fd_sk, socket(AF_UNIX, SOCK_STREAM, 0));

    //2. BIND (assegnazione del socket)
    int notused;
    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, newConfigFile->socketName, strlen(newConfigFile->socketName)+1);
    SYSCALL_EXIT("bind", notused, bind(fd_sk, (struct sockaddr*)&address, sizeof(address)));

    //3. LISTEN (socket messo in ascolto)
    SYSCALL_EXIT("listen", notused, listen(fd_sk, 1));

    // inizializzo la set
    fd_set set, tmpset;

    if(fd_sk > fd_max){
        fd_max = fd_sk;
    }

    FD_ZERO(&set);
    FD_ZERO(&tmpset);

    FD_SET(fd_sk, &set);
    FD_SET(comunicationPipe[0], &set);

    Msg* welcome_msg = smalloc(sizeof(Msg));
    setMsg(welcome_msg, 0, 0, "w", "WELCOME\0", NULL, 0);

    //alloco un array di pthread_t associati ognuno ad un thread worker:
    pthread_t* id_pool = smalloc(newConfigFile->numWorker *sizeof(pthread_t));
    currentStatus.workerRequests = smalloc(newConfigFile->numWorker*sizeof(int));
    
    
    for(int i = 0; i < newConfigFile->numWorker; i++){
        int *argomenti = smalloc(sizeof(int)*2);
        argomenti[0] = comunicationPipe[1];
        argomenti[1] = i;
        CHECK_FUNCTION("pthread_create", pthread_create(&id_pool[i], NULL, worker, argomenti), !=, 0);
        currentStatus.workerRequests[i] = 0;
    }

    printf("Inizio server..\n");
    while(!hardExit){
        tmpset = set;

        if(select(fd_max + 1, &tmpset, NULL, NULL, NULL) == -1){
            if(softExit || hardExit){
                break;
            }
            fprintf(stderr,"ERRORE: select fallita\n");
            return -1;
        }

        for(int i=0; i <= fd_max; i++){
            if(FD_ISSET(i, &tmpset)){
                if(i == fd_sk){ // nuova richiesta di un nuovo client
                    // accetto una nuova connessione:
                    int socket_con;
                    SYSCALL_EXIT("accept", socket_con, accept(fd_sk, NULL, NULL));

                    //aggiorno le statistiche del server
                    LOCK("mtx_status", mtx_status);
                    currentStatus.num_current_con++;
                    if(currentStatus.num_current_con > currentStatus.max_con){
                        currentStatus.max_con = currentStatus.num_current_con;
                    }
                    UNLOCK("mtx_status", mtx_status);

                    if(softExit){
                        close(socket_con);
                    }
                    else{//se non ho softExit == 1 apro effettivamente la connessione
                        FD_SET(socket_con, &set);

                        if(fd_max < socket_con){
                            fd_max = socket_con;
                        }

                        //invio il messaggio di benvenuto (avvenuta connessione):
                        if(sendMsg(welcome_msg, socket_con) == -1){
                            fprintf(stderr, "ERRORE: sendMsg.\n");
                        }
                    }
                }
                else if(i == comunicationPipe[0]){ // richiesta terminata dal worker
                    if(read(i, pipeBuffer, RESPONSE_CODE_LENGTH) == -1){
                        fprintf(stderr ,"ERRORE: read pipe.\n");
                    }

                    //se ho ricevuto sulla pipe 'c' allora termino il ciclo for (chiudo la connessione)
                    if(strcmp(pipeBuffer, "c") == 0){
                        break;
                    }

                    //altrimenti se il fd ricevuto è != 0 setto nuovamente tmpset
                    if(atol(pipeBuffer) != 0){
                        FD_SET(atol(pipeBuffer), &tmpset);
                        if(atol(pipeBuffer) > fd_max){
                            fd_max = atol(pipeBuffer);
                        }
                    }
                    else{
                        if(currentStatus.num_current_con == 0 && softExit){
                            break;
                        }
                    }
                    
                }
                else{ // nuova richiesta da un client già connesso
                    FD_CLR(i, &set);
                    if(i == fd_max){
                        fd_max--;
                    }
                    
                    //inserisco il fd nella coda dei task
                    enqueueTask(coda_task, i);
                }
            }
        }

    }

    //inserisco nella coda task dei messaggi che permettono di terminare i thread worker
    for(int i = 0; i < newConfigFile->numWorker; i++){
        enqueueTask(coda_task, -1);
    }

    //attendo la terminazione dei thread worker
    for(int i = 0; i < newConfigFile->numWorker; i++){
        CHECK_FUNCTION("pthread_join workers", pthread_join(id_pool[i], NULL), !=, 0);
    }
    //libero la memoria dell'array contenente i pthread_t dei thread worker
    free(id_pool);

    //libero la memoria del messaggio mandato:
    free(welcome_msg->data);
    free(welcome_msg);

    //copio le statistiche del server nel file di log.
    fprintf(file_log, "\n");
    saveProgramStatusOnLogFile(file_log, currentStatus, newConfigFile->numWorker, 1);
    
    //libero la memoria e chiudo gli stream aperti:
    fclose(file_log);
    free(currentStatus.workerRequests);
    close(comunicationPipe[0]);
    close(comunicationPipe[1]);
    unlink(newConfigFile->socketName);
    icl_hash_destroy(hash_table, NULL, NULL);
    destroyQueue(coda_file);
    destroyQueueTask(coda_task);
    free(newConfigFile);

    return 0;
}