#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <getopt.h>
#include <libgen.h>

#include "utils.h"
#include "clientAPI.h"
#include "optList.h"

//tempo in millisecondi che intercorre tra l'invio di due richieste:
long lag_time = 0;

//directory dove salvare i file espulsi dal server:
char dirExp[MAX_LENGTH];

//direcotry dove salvare i file letti dal server:
char dirSave[MAX_LENGTH];

/* EFFETTO: funzione che scrive il file passato come argomento nel server cioè,
 *          se non è presente nel server lo crea, scrive al suo interno il contenuto e lo chiude
 *          se il file è già presente nel server allora lo apre, appende il contenuto al suo interno e lo chiude
 *
 * RITORNA: -1 -> errore 
 *           0 -> terminazione con successo
 */
int writeFileOnServer(char *file_path, struct stat info){
    if(openFile(file_path, O_CREATE) == 0) {   // se apro il file con O_CREATE correttamento (creo il file) allora
        if(writeFile(file_path, dirExp) != 0) {  // scrivo al suo interno il contenuto di 'file_path'
            fprintf(stderr, "ERRORE: writeFile di %s\n", file_path);
        }
    }
    else if(openFile(file_path, O_NOFLAG) == 0){ // altrimenti se il file è già presente appendo al suo interno il contenuto di 'file_path'
        FILE *filePointer;
        filePointer = fopen(file_path, "rb");
        CHECK_NULL("fopen", filePointer);
        void *data = smalloc(sizeof(char)*info.st_size);
        if(fread(data, 1, info.st_size, filePointer) == info.st_size){
            if(appendToFile(file_path, data, info.st_size, dirExp) != 0){
                fprintf(stderr, "ERRORE: appendFile di %s\n", file_path);
            }
        }
        fclose(filePointer);
        free(data);
    }
    else{
        return -1;
    }

    if(closeFile(file_path) != 0) {      // e chiudo il file nel server
        return -1;
    }

    return 0;
}

/* EFFETTO: funzione che apre ricorsivamente la 'directory_path' e le directory al suo interno e scrive 'num_files' nel server (se num_file == -1 li scrivo tutti)
 *
 * RITORNA: -1 -> errore 
 *           0 -> terminazione con successo
 */
int recursiveOpenDirectory(char *directory_path, int num_files){
    DIR *directory;
    struct dirent *entry;

    if(!(directory = opendir(directory_path))){//directory non valida, ritorno -1
        fprintf(stderr, "ERRORE impossibile aprire la directory '%s'.", directory_path);
        return -1;
    }
    

    while((entry = readdir(directory)) != NULL && (num_files != 0)){//finchè ho directory da leggere e num_file != 0 ciclo
        char path[MAX_LENGTH];
        if(directory_path[strlen(directory_path)-1] != '/'){                     // se la directory non ha '/' come carattere finale:
            snprintf(path, sizeof(path), "%s/%s", directory_path, entry->d_name);// aggiungo '/' e il nome del file trovato
        }
        else{
            snprintf(path, sizeof(path), "%s%s", directory_path, entry->d_name);// altrimenti se ha '/' aggiungo solo il file trovato
        }

        struct stat info;
        if(stat(path, &info) == 0){//apro le statistiche del file ottenuto
            if(S_ISDIR(info.st_mode)){//se è una directory
                if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){//ignoro '.' e '..'
                    continue;
                }
                recursiveOpenDirectory(path, num_files);//chiamo ricorsivamente la funzione sulla nuova directory
            }
            else{//se è un file
                if(writeFileOnServer(path, info) == 0){ // lo scrivo nel server
                    num_files--;                        // e diminuisco 'num_files'
                }
                else{
                    fprintf(stderr, "ERRORE in scrittura di un file nel server\n");
                }
            }
        }
    }
    closedir(directory);//chiudo la directory

    return 0;
}

//stampa su stdout i possibili comandi da dare al client (-h)
void command_list(){
    printf("Comandi accettati dal client:\
            \n-f filename : specifica il nome del socket AF_UNIX a cui connettersi;\
            \n-w dirname[,n=0] : invia al server 'n' file nella cartella ‘dirname’. Se n=0 non viene posto un limite.\
            \n-W file1[,file2]: lista di nomi di file da scrivere nel server separati da ‘,’;\
            \n-D dirname : cartella dove vengono scritti i file che il server rimuove a seguito di capacity misses, errore se prima non si fa ‘-w’ o ‘-W’.\
            \n-r file1[,file2] : lista di nomi di file da leggere dal server separati da ‘,’;\
            \n-R [n=0] : legge ‘n’ file qualsiasi attualmente memorizzati nel server; se n=0 allora vengono letti tutti;\
            \n-d dirname : cartella in memoria secondaria dove scrivere i file letti dal server con l’opzione ‘-r’ o ‘-R’.\
            \n-t time : tempo in millisecondi che intercorre tra l’invio di due richieste successive al server;\
            \n-l file1[,file2] : lista di nomi di file su cui acquisire la mutua esclusione;\
            \n-u file1[,file2] : lista di nomi di file su cui rilasciare la mutua esclusione;\
            \n-c file1[,file2] : lista di file da rimuovere dal server se presenti;\
            \n-p : abilita le stampe sullo standard output per ogni operazione.\n");
}

/* EFFETTO: funzione che processa l'operazione in testa a 'operation_to_do'
 *
 * RITORNA: -1 -> errore (settando errno)
 *           0 -> terminazione con successo
 */
int processOperation(optList *operation_to_do){
    char *file_path;//stringa su cui andrò a scrivere il file_path resistuito dalle chiamate al server se serve

    //switch sul tipo di operazione da svolgere:
    switch(operation_to_do->head->type){
        case SEND_DIRECTORY_OP: //-w
            if(operation_to_do->head->args == NULL){//se gli argomenti non sono validi ritorno -1 settando errno
                fprintf(stderr, "Errore nell'argomento dell'operazione di send directory(-w).\n");
                errno = EINVAL;
                return -1;
            }

            if(print_stdout){
                printf("Op di save di tot file contenuto in una cartella: %d(-w), argomenti: %s\n", operation_to_do->head->type, operation_to_do->head->args);
            }
    
            //leggo la directory passata come argomento:
            DIR* directory;
            char *directory_path = strtok(operation_to_do->head->args, ",");
            //se non è valida ritorno -1 settando errno
            if((directory = opendir(directory_path)) == NULL){
                fprintf(stderr, "ERRORE: apertura directory.\n");
                free(directory);
                errno = ENOENT;
                return -1;
            }
            else{
                if(print_stdout){
                    printf("Cartella di scrittura dei file = %s\n", directory_path);
                }
            }

            char *tmp = strtok(NULL, ",");
            int num_files = -1;

            //se presente leggo il numero di file da leggere:
            if(tmp != NULL){
                strtok(tmp, "=");
                if((tmp = strtok(NULL, ",")) != NULL){
                    num_files = atoi(tmp);
                }
            }
            
            //chiamo la funzione che legge il contenuto di directory_path con gli argomenti correnti:
            if(recursiveOpenDirectory(directory_path, num_files) != 0){
                fprintf(stderr, "Errore nell'invio dei file della directory %s\n", directory_path);
                return -1;
            }

            break;
        case SEND_FILE_OP: //-W
            if(operation_to_do->head->args == NULL){//se gli argomenti non sono validi ritorno -1 settando errno
                fprintf(stderr, "Errore nell'input\n");
                errno = EINVAL;
                return -1;
            }

            if(print_stdout){
                printf("Op di save di una lista di file: %d(-W), argomenti: %s\n", operation_to_do->head->type, operation_to_do->head->args);
            }

            //faccio il parsing della stringa passata come argomento:
            file_path = strtok(operation_to_do->head->args, ",");
            while(file_path != NULL){//finchè c'è un file ciclo
                struct stat info;
                if(stat(file_path, &info) == 0){// apro le statistiche del file
                    if(S_ISDIR(info.st_mode) == 0){// se il file è leggibile chiamo la funzione per scriverlo nel server
                        if(writeFileOnServer(file_path, info) == -1){
                            fprintf(stderr, "ERRORE writeFileOnServer\n");
                            return -1;
                        }
                    }
                    else{// altrimenti stampo errore e ritorno -1 settando errno
                        fprintf(stderr ,"Il file è una cartella\n");
                        errno = EISDIR;
                        return -1;
                    }
                }
                else{// non è possibile aprire le statistiche del file
                    fprintf(stderr, "Il file non esiste\n");  
                    errno = ENOENT;
                    return -1;
                }

                file_path = strtok(NULL, ",");
            }
            
            break;
        case READ_FILE_OP: //-r
            if(operation_to_do->head->args == NULL){//se gli argomenti non sono validi ritorno -1 settando errno
                fprintf(stderr, "Errore nell'input\n");
                errno = EINVAL;
                return -1;
            }

            if(print_stdout){
                printf("Op di lettura di una lista di file: %d(-r), argomenti: %s\n", operation_to_do->head->type, operation_to_do->head->args);
            }
            

            //inizializzo il buffer dove andrà il file letto e la sua size:
            void *buf;
            size_t buf_size;
            file_path = strtok(operation_to_do->head->args, ",");

            while (file_path != NULL){//finchè leggo file ciclo
                if(openFile(file_path, O_NOFLAG) != 0) { // apro il file senza flag
                    fprintf(stderr, "ERRORE: openFile\n");
                    return -1;
                }

                if(readFile(file_path, &buf, &buf_size) == 0){// ne leggo il contenuto
                    if(opendir(dirSave) != NULL){ // se è stata settata la cartella dove salvare i file letti (-d)
                        //scrivo il file nella cartella
                        FILE* file;
                        char pathname_file[PATH_MAX_LENGHT];
                        memset(pathname_file, 0, PATH_MAX_LENGHT);
                        strcpy(pathname_file, dirSave);
                        if(pathname_file[PATH_MAX_LENGHT-1] != '/'){
                            strcat(pathname_file, "/");
                        }
                        strcat(pathname_file, basename(file_path));
                        file = fopen(pathname_file, "wb");
                        CHECK_NULL("fopen", file);
                        fwrite(buf, 1, buf_size, file);
                        fclose(file);
                        
                        if(print_stdout){
                            printf("File %s salvato in %s con il contenuto:\n%s\n", file_path, dirSave, (char *)buf);
                        }
                        
                        free(buf);
                    }
                    else{
                        if(print_stdout){
                            printf("File %s:\n%s\n", file_path, (char *)buf);
                        }   
                    }
                }
                else{
                    fprintf(stderr, "ERRORE: readFile\n");
                    return -1;
                }

                if(closeFile(file_path) != 0){ // chiudo il file
                    fprintf(stderr, "ERRORE: closeFile\n");
                    return -1;
                }
                
                file_path = strtok(NULL, ",");
            }

            break;
        case READ_N_FILE_OP: //-R
            if(operation_to_do->head->args == NULL){//se gli argomenti non sono validi ritorno -1 settando errno
                fprintf(stderr, "Errore nell'input\n");
                errno = EINVAL;
                return -1;
            }

            if(print_stdout){
                printf("Op di lettura di più file: %d(-R), argomenti: %s\n", operation_to_do->head->type, operation_to_do->head->args);
            }
            
            int num;
            if(operation_to_do->head->args != NULL) { 
                num = atoi(operation_to_do->head->args);
            } else { // se non è stato inserito il numero di file da leggere lo imposto a 0 (leggi tutti i file)
                num = 0; 
            }

            if(opendir(dirSave) == NULL){// se non è stata settata la directory dove salvare i file passo NULL come argomento
                if(readNFiles(num, NULL) != 0) {
                    fprintf(stderr, "ERRORE: readNFile\n");
                    return -1;
                }
            }
            else{ // altrimenti passo la 'dirSave'
                if(readNFiles(num, dirSave) != 0) {
                    fprintf(stderr, "ERRORE: readNFile\n");
                    return -1;
                }
            }

            
            break;
        case LOCK_OP: //-l
            if(operation_to_do->head->args == NULL){//se gli argomenti non sono validi ritorno -1 settando errno
                fprintf(stderr, "Errore nell'input\n");
                errno = EINVAL;
                return -1;
            }

            if(print_stdout){
                printf("Op di lock del file: %d(-l), argomenti: %s\n", operation_to_do->head->type, operation_to_do->head->args);
            }
            
            file_path = strtok(operation_to_do->head->args, ",");
            while(file_path != NULL){//ciclo finchè leggo file
                if(openFile(file_path, O_NOFLAG) != 0) { // apro il file senza flag (avrei potuto anche aprirlo direttamente con la flag O_LOCK)
                    fprintf(stderr, "ERRORE: openFile\n");
                    return -1;
                }
                
                if(lockFile(file_path) != 0){ // metto il file in lock
                    fprintf(stderr, "ERRORE: lockFile\n");
                    if(closeFile(file_path) != 0){ // chiudo il file
                        fprintf(stderr, "ERRORE: closeFile\n");
                        return -1;
                    }
                    return -1;
                }

                if(closeFile(file_path) != 0){ // chiudo il file
                    fprintf(stderr, "ERRORE: closeFile\n");
                    return -1;
                }
                
                file_path = strtok(NULL, ",");
            }

            break;
        case UNLOCK_OP: //-u
            if(operation_to_do->head->args == NULL){//se gli argomenti non sono validi ritorno -1 settando errno
                fprintf(stderr, "Errore nell'input\n");
                errno = EINVAL;
                return -1;
            }

            if(print_stdout){
                printf("Op di unlock: %d(-u), argomenti: %s\n", operation_to_do->head->type, operation_to_do->head->args);
            }
            
            
            file_path = strtok(operation_to_do->head->args, ",");
            while(file_path != NULL){//ciclo finchè ho file
                if(openFile(file_path, O_NOFLAG) != 0) { // apro il file
                    fprintf(stderr, "ERRORE: openFile\n");
                    return -1;
                }

                if(unlockFile(file_path) != 0){ // resetto la lock
                    fprintf(stderr, "ERRORE: unlockFile\n");
                    if(closeFile(file_path) != 0){ // chiudo il file
                        fprintf(stderr, "ERRORE: closeFile\n");
                        return -1;
                    }
                    return -1;
                }

                if(closeFile(file_path) != 0){ // chiudo il file
                    fprintf(stderr, "ERRORE: closeFile\n");
                    return -1;
                }

                file_path = strtok(NULL, ",");
            }

            break;
        case REMOVE_OP: //-c
            if(operation_to_do->head->args == NULL){//se gli argomenti non sono validi ritorno -1 settando errno
                fprintf(stderr, "Errore nell'input\n");
                errno = EINVAL;
                return -1;
            }

            if(print_stdout){
                printf("Op di rimozione: %d(-c), argomenti: %s\n", operation_to_do->head->type, operation_to_do->head->args);
            }
            
            file_path = strtok(operation_to_do->head->args, ",");
            while(file_path != NULL){//ciclo finchè ho file
                if(openFile(file_path, O_LOCK) != 0){ // apro il file in modalità bloccante
                    fprintf(stderr, "ERRORE: openFile\n");
                    return -1;
                }
                
                if(removeFile(file_path) != 0){ //rimuovo il file dal server
                    fprintf(stderr, "ERRORE: removeFile\n");
                    return -1;
                }
                
                if(print_stdout){
                    printf("File %s rimosso dal server.\n", file_path);    
                }
                
                file_path = strtok(NULL, ",");
            }

            break;
        default://errore
            fprintf(stderr, "ERRORE: tipo dell'operazione non valido\n");
            errno = EBADRQC;
            return -1;
    }

    return 0;
}

int main(int argc, char *argv[]){
    if(argc < 2){//controllo di avere abbastanza argomenti
        printf("Argomenti insufficienti.\n");
        return -1;
    }

    //alloco e inizializzo la struttura che mantiene le operazioni:
    optList *operation_to_do = smalloc(sizeof(optList));
    operation_to_do->numElem = 0;
    operation_to_do->head = NULL;
    operation_to_do->tail = NULL;

    //tre booleani che ci permettono (o meno) di ripetere alcune opzioni (-f -h -p)
    bool f_repeat = false;
    bool h_repeat = false;
    bool p_repeat = false;


    int opt;
    while((opt = getopt(argc, argv, ":hf:w:W:D:r:R:d:t:l:u:c:p")) != -1){//ciclo in cui su fa getopt degli argomenti passati al programma
        
        int lastOpt;// intero in cui inserisco (se necessario) il tipo del'ultima operazione

        switch(opt){//switch sul tipo di operazione trovato:
            case 'h':
                if(h_repeat == false){//se non è già stato fatto stampo la lista comandi
                    command_list();
                }
                h_repeat = true;
                break;
            case 'f':
                if(f_repeat == false){//se non è già stato fatto scrivo il pathname del socket a cui connettersi
                    skt_path = smalloc(MAX_LENGTH);
                    strcpy(skt_path, optarg);
                }
                f_repeat = true;
                break;
            case 'w':
                if(pushOpt(operation_to_do, SEND_DIRECTORY_OP, optarg) == -1){//inserisco l'operazione (-w) nella struttura
                    fprintf(stderr, "ERRORE: push dell'operazione.\n");
                }
                break;
            case 'W':
                if(pushOpt(operation_to_do, SEND_FILE_OP, optarg) == -1){//inserisco l'operazione (-W) nella struttura
                    fprintf(stderr, "ERRORE: push dell'operazione.\n");
                }
                break;
            case 'D':
                lastOpt = checkLastOpt(operation_to_do);//leggo il tipo dell'ultima operazione fatta
                if(lastOpt != SEND_DIRECTORY_OP && lastOpt != SEND_FILE_OP){//se è diversa da '-w' o '-W' allore stampo errore
                    fprintf(stderr, "-D richiede prima -w o -W\n");
                }
                else{//altrimenti copio l'argomento in dirExp e sarà la directory dove salvare i file espulsi dal server
                    strcpy(dirExp, optarg);
                }
                break;
            case 'r':
                if(pushOpt(operation_to_do, READ_FILE_OP, optarg) == -1){//inserisco l'operazione (-r) nella struttura
                    fprintf(stderr, "ERRORE: push dell'operazione.\n");
                }
                break;
            case 'R':
                if(pushOpt(operation_to_do, READ_N_FILE_OP, optarg) == -1){//inserisco l'operazione (-R) nella struttura
                    fprintf(stderr, "ERRORE: push dell'operazione.\n");
                }
                break;
            case 'd':
                lastOpt = checkLastOpt(operation_to_do);//leggo il tipo dell'ultima operazione fatta
                if(lastOpt != READ_FILE_OP && lastOpt != READ_N_FILE_OP){//se è diversa da '-r' o '-R' allora stampo errore
                    fprintf(stderr, "-d richiede prima -r o -R\n");
                }
                else{//altrimenti copio l'argomento in dirSave e sarà la directory dove salvare i file letti
                    strcpy(dirSave, optarg);
                }
                break;
            case 't':
                lag_time = atol(optarg);//setto il tempo di lag(tra un operazione e l'altra)
                break;
            case 'l':
                if(pushOpt(operation_to_do, LOCK_OP, optarg) == -1){//inserisco l'operazione (-l) nella struttura
                    fprintf(stderr, "ERRORE: push dell'operazione.\n");
                }
                break;
            case 'u':
                if(pushOpt(operation_to_do, UNLOCK_OP, optarg) == -1){
                    fprintf(stderr, "ERRORE: push dell'operazione.\n");//inserisco l'operazione (-u) nella struttura
                }
                break;
            case 'c':
                if(pushOpt(operation_to_do, REMOVE_OP, optarg) == -1){
                    fprintf(stderr, "ERRORE: push dell'operazione.\n");//inserisco l'operazione (-c) nella struttura
                }
                break;
            case 'p':
                if(p_repeat == false){//se non è già stato fatto imposto la stampa su stdout a true
                    print_stdout = true;
                }
                p_repeat = true;

                break;
            default:
                fprintf(stderr, "ERRORE, fare -h per lista comandi.\n");
        }
    }

    if(skt_path != NULL){//se è stato inserito il nome di un socket provo a connettermi
        struct timespec abstime;
        abstime.tv_sec = 10;
        abstime.tv_nsec = 0;
        
        fd_skt = openConnection(skt_path, 1000, abstime);
        if(fd_skt == -1){
            // fprintf(stderr, "ERRORE: openConnection\n");
            return -1;
        }
    }
    else{//altrimenti stampo errore libero la memoria ed termino il programma.
        fprintf(stderr, "Socket non definito, impossibile collegarsi al server.\n");
        deleteOptList(operation_to_do);
        free(operation_to_do);
        return -1;
    }

    if(print_stdout){
        printf("Connessione stabilita correttamente.\n");
    }

    struct timespec t;
    t.tv_nsec = (lag_time%1000)*1000000000;
    t.tv_sec = lag_time/1000;

    //inizio a comunicare con il server:
    while(operation_to_do->numElem > 0){//ciclo finchè ho operazioni da eseguire
        
        nanosleep(&t, NULL);//attesa (lag)

        if(processOperation(operation_to_do) != 0){//processo l'operazione, in caso di errore stampo una descrizione del'errore e il suo codice(errno).
            fprintf(stderr, "Impossibile completare l'operazione correttamente, ERRORE: %s(Codice errno=%d).\n", strerror(errno), errno);
        }

        free(popOpt(operation_to_do));//elimino l'operazione eseguita
    }

    //chiudo la connessione:
    if(closeConnection(skt_path) == 0){
        if(print_stdout){
            printf("Connessione chiusa correttamente\n");
        }
    }
    else{
        fprintf(stderr, "ERRORE in chiusura di connessione.\n");
    }

    //libero la memoria:
    free(skt_path);
    deleteOptList(operation_to_do);
    free(operation_to_do);
    
    return 0;                
}