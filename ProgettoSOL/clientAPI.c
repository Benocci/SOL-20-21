/***************************************************************************************
CLIENTAPI.c: implementazione dei metodi che permettono al client di comunicare con il server

AUTORE: FRANCESCO BENOCCI

NOTE: 

****************************************************************************************/

#include "utils.h"
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <libgen.h>

#include "clientAPI.h"
#include "message.h"

//abilita la stampa ad ogni operazione:
bool print_stdout = false;

/* EFFETTO: funzione che controlla il codice di risposta dal server
 *
 * RITORNA:   0 -> nessun errore
 *           -1 -> codice di ritorno che porta ad un errore (setta opportunamente errno)
 */
int responseCheck(char response_code[RESPONSE_CODE_LENGTH]){

    //codici di risposta senza errore:
    if(strcmp(response_code, "ok") == 0){
        //printf("Ritorno dal server avvenuto con successo.\n");
        return 0;
    }

    if(strcmp(response_code, "w") == 0){
        //printf("Connessione con il server ottenuta con successo.\n");
        return 0;
    }

    //codice di risposta con errori:
    if(strcmp(response_code, "mcr") == 0){
        fprintf(stderr, "Operazione non possibile MCR: max connection reached.\n");
        errno = EUSERS;
        return -1;
    }

    if(strcmp(response_code, "ne") == 0){
        fprintf(stderr, "Operazione non possibile NE: file not exist.\n");
        errno = ENOENT;
        return -1;
    }

    if(strcmp(response_code, "nw") == 0){
        fprintf(stderr, "Operazione non possibile NW: file not written.\n");
        errno = ENOENT;
        return -1;
    }

    if(strcmp(response_code, "es") == 0){
        fprintf(stderr, "Operazione non possibile ES: empty server.\n");
        errno = ENOENT;
        return -1;
    }

    if(strcmp(response_code, "no") == 0){
        fprintf(stderr, "Operazione non possibile NE: file not open.\n");
        errno = ENOENT;
        return -1;
    }

    if(strcmp(response_code, "tb") == 0){
        fprintf(stderr, "Operazione non possibile TB: file too big.\n");
        errno = ENOENT;
        return -1;
    }

    if(strcmp(response_code, "ae") == 0){
        fprintf(stderr, "Operazione non possibile AE: file already exist.\n");
        errno = EEXIST;
        return -1;
    }

    if(strcmp(response_code, "ao") == 0){
        fprintf(stderr, "Operazione non possibile AO: file already open.\n");
        errno = EEXIST;
        return -1;
    }

    if(strcmp(response_code, "fe") == 0){
        fprintf(stderr, "Operazione non possibile FE: flag error.\n");
        errno = EINVAL;
        return -1;
    }

    if(strcmp(response_code, "l") == 0){
        fprintf(stderr, "Operazione non possibile L: file locked.\n");
        errno = EACCES;
        return -1;
    }

    if(strcmp(response_code, "aw") == 0){
        fprintf(stderr, "Operazione non possibile AW: file already writed.\n");
        errno = EEXIST;
        return -1;
    }

    if(strcmp(response_code, "ac") == 0){
        fprintf(stderr, "Operazione non possibile AC: file already closed.\n");
        errno = EEXIST;
        return -1;
    }

    if(strcmp(response_code, "au") == 0){
        fprintf(stderr, "Operazione non possibile AU: file already unlocked.\n");
        errno = EEXIST;
        return -1;
    }

    if(strcmp(response_code, "odo") == 0){
        fprintf(stderr, "Operazione non possibile ODO: file open by different owner.\n");
        errno = EACCES;
        return -1;
    }

    if(strcmp(response_code, "ldo") == 0){
        fprintf(stderr, "Operazione non possibile LDO: file lock by different owner.\n");
        errno = EACCES;
        return -1;
    }

    if(strcmp(response_code, "dfu") == 0){
        fprintf(stderr, "Operazione non possibile DFU: delete file unlocked.\n");
        errno = ENOPROTOOPT;
        return -1;
    }

    if(strcmp(response_code, "onv") == 0){
        fprintf(stderr, "ERRORE ONV: operation not valid.\n");
        errno = ENOSYS;
        return -1;
    }

    fprintf(stderr, "ERRORE UNKNOWN con codice: %s.\n", response_code);
    errno = EINVAL;
    return -1;
}

//funzione che stampa su stdout se è stato inserito -p come opzione al client
void printStdOut(int opt_type, char *filepath, int flags, char *responseCode, int num_bytes){
    if(print_stdout){
        printf("\n");
        printf("Tipo dell'operazione: %s\n", convertOptCodeInChar(opt_type));
        printf("Codice di risposta del server: %s\n", responseCode);
        if(filepath != NULL){
            printf("Filepath di riferimento: %s\n", filepath);
        }
        if(flags != -1){
            printf("Flag: %s", convertOptCodeInChar(flags));
        }
        if(num_bytes != -1){
            printf("Numero di byte letti/scritti: %d\n", num_bytes);
        }
        printf("\n");
    }
}

int openConnection(const char *sockname, int msec, const struct timespec abstime){
    //creo il socket:
    SYSCALL_RETURN("socket", fd_skt, socket(AF_UNIX, SOCK_STREAM, 0));
    
    //associo l'indirizzo:
    struct sockaddr_un sockaddr;
    strncpy(sockaddr.sun_path, sockname, UNIX_PATH_MAX);
    sockaddr.sun_family = AF_UNIX;

    //salvo il tempo iniziale:
    struct timespec startTime;
    startTime.tv_sec = time(0);
    
    while(connect(fd_skt, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1){//ciclo finchè non ottengo la connessione
        //salvo il tempo corrente:
        struct timespec currTime;
        currTime.tv_sec = time(0);
        
        if(abstime.tv_sec >= (startTime.tv_sec-currTime.tv_sec)){//se è passato troppo tempo ritorno -1 settando errno (connessione fallita)
            // fprintf(stderr,"ERRORE: la connessione al server ha richiesto troppo tempo.\n");
            errno = EAGAIN;
            return -1;
        }

        
        //socket ancora non pronto.
        printf("Socket not ready. Wait %d msec.\n", msec);
        sleep(msec*1000);//aspetto 'msec' millisecondi
    }

    //connessione stabilita, ricevo il messaggio di benvenuto:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive, fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(OPEN_CONNECTION, NULL, -1, to_receive->response_code, -1);

    if(responseCheck(to_receive->response_code) == 0){//se il messaggio è corretto ritorno il file descriptor
        return fd_skt;
    }
    else{
        return -1;
    }
}

int closeConnection(const char *sockname){
    if(!sockname){//se il sockname non è valido ritorno -1 settando errno
        errno = EINVAL;
        return -1;
    }

    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send, CLOSE_CONNECTION, 0, "","", NULL, 0);

    //invio il messaggio:
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }

    //ricevo il messaggio dal server:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive, fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, NULL, -1, to_receive->response_code, -1);

    //se il codice di risposta è corretto chiudo la connessione e ritorno 0, altrimenti ritorno -1(errno settato in responseCheck)
    if(responseCheck(to_receive->response_code) == 0){
        if(close(fd_skt) == -1){
            errno = ENOTCONN;
            return -1;
        }
        return 0;
    }
    else{
        return -1;
    }
}


int openFile(const char *pathname, int flags){
    if(!pathname){//se il pathname non è valido ritorno -1 settando errno
        errno = EINVAL;
        return -1;
    }
    
    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send, OPEN_FILE, flags, "", (char *)pathname, NULL, 0);

    //spedisco il messaggio:
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }

    //ricevo il messaggio:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, to_send->path, to_send->flags, to_receive->response_code, -1);

    //se il codice di risposta è corretto ritorno 0, -1 altrimenti
    if(responseCheck(to_receive->response_code) == 0){
        return 0;
    }
    else{
        return -1;
    }
}


int readFile(const char *pathname, void **buf, size_t *size){
    if(!pathname || !strlen(pathname) || !buf || !size){//se gli argomenti non sono validi ritorno -1 settando errno
        errno = EINVAL;
        return -1;
    }
    
    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send, READ_FILE, 0, "", (char *)pathname, NULL, 0);

    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }
    
    //ricevo il messaggio:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }
    
    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, to_receive->data_length);

    //se il codice di risposta è corretto scrivo nei campi il file letto e ritorno 0, -1 altrimenti
    if(responseCheck(to_receive->response_code) == 0){
        *size = to_receive->data_length;
        *buf = to_receive->data;
        free(to_receive);
        return 0;
    }
    else{
        return -1;
    }
}

int readNFiles(int N, const char *dirname){
    if(N < 0){//se N è negativo ritorno -1 settando errno
        errno = EINVAL;
        return -1;
    }

    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send, READ_N_FILE, N, "", "", NULL, 0);

    //spedisco il messaggio:
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }
    
    bool loop = true;
    int i = 0;
    while(loop){
        //ricevo un nuovo messaggio contenente il file da leggere:
        Msg* file_read = smalloc(sizeof(Msg));
        if(receiveMsg(file_read, fd_skt) == -1){
            errno = EBADMSG;
            return -1;
        }

        //se il codice di risposta è "ok" leggo il messaggio:
        if(strcmp(file_read->response_code, "ok") == 0){
            i++;
            if(dirname == NULL){//se non ho impostato una cartella dove scrivere i file (-d nel client) stampo su stdout se abilitato
                if(print_stdout){
                    printf("File %s letto:\n%s\n", file_read->path, file_read->data);
                }
            }
            else{
                FILE* file;
                char pathname_file[PATH_MAX_LENGHT];
                memset(pathname_file, 0, PATH_MAX_LENGHT);
                strcpy(pathname_file, dirname);

                if(pathname_file[PATH_MAX_LENGHT-1] != '/'){ //se la cartella data non finiva con '/' lo aggiungo
                    strcat(pathname_file, "/");
                }
                strcat(pathname_file, basename(file_read->path));//aggiungo il nome del file alla stringa così da ottenere il pathname assoluto

                file = fopen(pathname_file, "wb");//apro il file in scrittura
                CHECK_NULL("fopen", file);

                fwrite(file_read->data, 1, file_read->data_length, file);//scrivo al suo interno il contenuto del file letto

                fclose(file);//chiudo il file
                
                if(print_stdout){//se c'è l'opzione -p attiva stampo il file anche su stdout
                    printf("File %s salvato in %s con il contenuto:\n%s\n", file_read->path, dirname, file_read->data);
                }
            }
        }
        else if(strcmp(file_read->response_code, "es") == 0){//codice di uscita empty server, esco dal ciclo
            if(print_stdout){
                printf("Server vuoto.\n");
            }
            break;
        }
        else{
            loop = false;
        }
    }

    //ricevo il messaggio di terminazione della chiamata al server:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, NULL, -1, to_receive->response_code, -1);
    if(print_stdout){
        printf("Numero file letti: %d", i);
    }

    //se il codice di risposta è corretto ritorno 0, -1 altrimenti
    if(responseCheck(to_receive->response_code) == 0){
        return 0;
    }
    else{
        return -1;
    }
}

int writeFile(const char *pathname, const char *dirname){
    if (!pathname || !strlen(pathname) || !dirname || !strlen(pathname)){//se gli argomenti non sono validi ritorno -1 settando errno
        errno = EINVAL;
        return -1;
    }
    
    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    

    //apro in lettura il file da cui devo leggere:
    FILE* to_write = fopen(pathname, "rb");
    if(!to_write) {
        errno = ENOENT;
        return -1;
    }

    //apro le statistiche del file
    struct stat sb;
    if(stat(pathname, &sb) == -1) {
        errno = EBADF;
        return -1;
    }

    //leggo il contenuto del file:
    void *buffer = smalloc(sb.st_size);
    if(fread(buffer, 1, sb.st_size, to_write) != sb.st_size) {
        free(buffer);
        free(to_send);
        fprintf(stderr, "ERRORE: read del file da scrivere.\n");
        errno = EIO;
        return -1;
    }
    fclose(to_write);//e lo chiudo

    //metto il contenuto del file nel messaggio da spedire:
    setMsg(to_send, WRITE_FILE, 0, "", (char *)pathname, buffer, sb.st_size);

    //spedisco il messaggio.
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }

    //ricevo il messaggio di risposta:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    int expFile;
    if(strcmp(to_receive->response_code, "ef") == 0){ // se il codice è 'ef' allora deve espellere un file
        expFile = 1;
    }
    else if(strcmp(to_receive->response_code, "ok") == 0){ // se il codice è 'ok' allora continuo
        expFile = 0;
    }
    else{ // codice errato:
        //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
        printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, to_receive->data_length);
        responseCheck(to_receive->response_code);//setta errno
        return -1;
    }

    while(expFile == 1){ // devo espellere un file per capacità
        //ricevo il messaggio che contiene il file da espellere:
        Msg* deleted_file = smalloc(sizeof(Msg));
        if(receiveMsg(deleted_file,fd_skt) == -1){
            errno = EBADMSG;
            return -1;
        }

        //se il codice di risposta è "ok" leggo il messaggio:
        if(strcmp(deleted_file->response_code, "ok") == 0){
            if(opendir(dirname) != NULL){//se 'dirname' è una directory allora:
                //creo un file su cui scrivere il file espulso:
                FILE *file;
                char pathname_file[PATH_MAX_LENGHT];
                memset(pathname_file, 0, PATH_MAX_LENGHT);
                strcpy(pathname_file, dirname);
                if(pathname_file[PATH_MAX_LENGHT-1] != '/'){
                    strcat(pathname_file, "/");
                }
                strcat(pathname_file, basename(deleted_file->path));
                file = fopen(pathname_file, "wb");
                CHECK_NULL("fopen", file);
                fwrite(deleted_file->data, 1, deleted_file->data_length, file);
                fclose(file);

                if(print_stdout){
                    printf("File espulso per capacità superata %s salvato in %s.\n\n", deleted_file->path, dirname);
                }
            }
            else{//se 'dirname' non è valida allora (se presente -p) notifico su stdout l'espulsione del file.
                if(print_stdout){
                    printf("File %s espulso dal server per capacità superata, impossibile salvarlo.\n\n", deleted_file->path);
                }
            }
        }
        else{
            expFile = 0;
        }
    }

    //ricevo il messaggio di risposta:
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }
    
    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, to_receive->data_length);
    
    //se il codice di risposta non è corretto ritorno -1 altrimenti:
    if(responseCheck(to_receive->response_code) == 0){
        if(to_receive->data != NULL){ //se il campo data del messaggio non è vuoto è stato espulso un file per numero di file massimi
            if(opendir(dirname) != NULL){//se 'dirname' è una directory allora:
                //creo un file su cui scrivere il file espulso:
                FILE *file;
                char pathname_file[PATH_MAX_LENGHT];
                memset(pathname_file, 0, PATH_MAX_LENGHT);
                strcpy(pathname_file, dirname);
                if(pathname_file[PATH_MAX_LENGHT-1] != '/'){
                    strcat(pathname_file, "/");
                }
                strcat(pathname_file, basename(to_receive->path));
                file = fopen(pathname_file, "wb");
                CHECK_NULL("fopen", file);
                fwrite(to_receive->data, 1, to_receive->data_length, file);
                fclose(file);

                if(print_stdout){
                    printf("File espulso per numero di file superati %s salvato in %s.\n\n", to_receive->path, dirname);
                }
            }
            else{//se 'dirname' non è valida allora (se presente -p) notifico su stdout l'espulsione del file.
                if(print_stdout){
                    printf("File %s espulso dal server per numero di file superati, impossibile salvarlo.\n\n", to_receive->path);
                }
            }
        }
        else{
            if(print_stdout){
                printf("Nessun file espulso per numero di file.\n");
            }
        }
        return 0;
    }
    else{
        return -1;
    }
}

int appendToFile(const char* pathname, void* buf, size_t size, const char *dirname){
    if(!pathname || !strlen(pathname) || !buf || size == 0 || !dirname || !strlen(pathname)){//se gli argomenti non sono validi ritorno -1 settando errno
        errno = EINVAL;
        return -1;
    }
    
    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send, APPEND_TO_FILE, 0, "", (char *)pathname, buf, size);

    //spedisco il messaggio:
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }
    
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    int expFile;
    if(strcmp(to_receive->response_code, "ef") == 0){ // se il codice è 'ef' allora deve espellere un file
        expFile = 1;
    }
    else if(strcmp(to_receive->response_code, "ok") == 0){ // se il codice è 'ok' allora continuo
        expFile = 0;
    }
    else{ // codice errato:
        //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
        printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, to_receive->data_length);
        responseCheck(to_receive->response_code);//setta errno
        return -1;
    }

    while(expFile == 1){ // devo espellere un file per capacità
        //ricevo il messaggio che contiene il file da espellere:
        Msg* deleted_file = smalloc(sizeof(Msg));
        if(receiveMsg(deleted_file,fd_skt) == -1){
            errno = EBADMSG;
            return -1;
        }

        //se il codice di risposta è "ok" leggo il messaggio:
        if(strcmp(deleted_file->response_code, "ok") == 0){
            if(opendir(dirname) != NULL){//se 'dirname' è una directory allora:
                //creo un file su cui scrivere il file espulso:
                FILE *file;
                char pathname_file[PATH_MAX_LENGHT];
                memset(pathname_file, 0, PATH_MAX_LENGHT);
                strcpy(pathname_file, dirname);
                if(pathname_file[PATH_MAX_LENGHT-1] != '/'){
                    strcat(pathname_file, "/");
                }
                strcat(pathname_file, basename(deleted_file->path));
                file = fopen(pathname_file, "wb");
                CHECK_NULL("fopen", file);
                fwrite(deleted_file->data, 1, deleted_file->data_length, file);
                fclose(file);

                if(print_stdout){
                    printf("File espulso per capacità superata %s salvato in %s.\n\n", deleted_file->path, dirname);
                }
            }
            else{//se 'dirname' non è valida allora (se presente -p) notifico su stdout l'espulsione del file.
                if(print_stdout){
                    printf("File %s espulso dal server per capacità superata, impossibile salvarlo.\n\n", deleted_file->path);
                }
            }
        }
        else{
            expFile = 0;
        }
    }

    //ricevo il messaggio di terminazione della chiamata al server:
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, size);

    //se il codice di risposta non è corretto ritorno -1 altrimenti:
    if(responseCheck(to_receive->response_code) == 0){
        return 0;
    }
    else{
        return -1;
    }
}

int lockFile(const char *pathname){
    if(!pathname || !strlen(pathname)){//se pathname non è valido ritorno -1 e setto errno
        errno = EINVAL;
        return -1;
    }
    
    Msg* to_receive = smalloc(sizeof(Msg));
    
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send, LOCK_FILE, 0, "", (char *)pathname, NULL, 0);

    do{
        //spedisco il messaggio:
        if(sendMsg(to_send, fd_skt) == -1){
            errno = ECOMM;
            return -1;
        }

        //ricevo il messaggio di ritorno:
        if(receiveMsg(to_receive,fd_skt) == -1){
            errno = EBADMSG;
            return -1;
        }

        //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
        printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, -1);

    }while(strcmp(to_receive->response_code, "l") == 0);


    //se il codice di risposta è corretto ritorno 0, -1 altrimenti
    if(responseCheck(to_receive->response_code) == 0){
        return 0;
    }
    else{
        return -1;
    }
}

int unlockFile(const char *pathname){
    if(!pathname || !strlen(pathname)){//se pathname non è valido ritorno -1 e setto errno
        errno = EINVAL;
        return -1;
    }

    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send,UNLOCK_FILE , 0, "", (char *)pathname, NULL, 0);

    //spedisco il messaggio:
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }

    //ricevo il messaggio di ritorno:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, -1);

    //se il codice di risposta è corretto ritorno 0, -1 altrimenti
    if(responseCheck(to_receive->response_code) == 0){
        return 0;
    }
    else{
        return -1;
    }
}

int closeFile(const char *pathname){
    if(!pathname || !strlen(pathname)){//se pathname non è valido ritorno -1 e setto errno
        errno = EINVAL;
        return -1;
    }
    
    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send, CLOSE_FILE, 0, "", (char *)pathname, NULL, 0);

    //spedisco il messaggio:
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }

    //ricevo il messaggio di ritorno:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, -1);

    //se il codice di risposta è corretto ritorno 0, -1 altrimenti
    if(responseCheck(to_receive->response_code) == 0){
        return 0;
    }
    else{
        return -1;
    }
}

int removeFile(const char *pathname){
    if(!pathname || !strlen(pathname)){//se pathname non è valido ritorno -1 e setto errno
        errno = EINVAL;
        return -1;
    }

    //creo il messaggio da spedire:
    Msg* to_send = smalloc(sizeof(Msg));
    setMsg(to_send,REMOVE_FILE , 0, "", (char *)pathname, NULL, 0);

    //spedisco il messaggio:
    if(sendMsg(to_send, fd_skt) == -1){
        errno = ECOMM;
        return -1;
    }

    //ricevo il messaggio di ritorno:
    Msg* to_receive = smalloc(sizeof(Msg));
    if(receiveMsg(to_receive,fd_skt) == -1){
        errno = EBADMSG;
        return -1;
    }

    //stampo (se abilitato) il tipo di operazione, i suoi argomenti e il codice di risposta del server
    printStdOut(to_send->opt_code, to_send->path, -1, to_receive->response_code, -1);

    //se il codice di risposta è corretto ritorno 0, -1 altrimenti
    if(responseCheck(to_receive->response_code) == 0){
        return 0;
    }
    else{
        return -1;
    }
}