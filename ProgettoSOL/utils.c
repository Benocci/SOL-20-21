/*******************************************************************************************
UTILS.c: implementazione di funzioni utili all'intero programma

AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#include "utils.h"
#include "message.h"

void *smalloc(size_t size){
    void *to_return = malloc(size);//alloco il nuovo elemento grande 'size'
    if(to_return == NULL){         //se la malloc non avviene correttamente stampo errore e ritorno
        perror("ERRORE: malloc");
        exit(EXIT_FAILURE);
    }

    memset(to_return, 0, size);   //setto 'size' byte (tutti) del nuovo elemento a 0 e ritorno
    return to_return;
}

int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;
        left    -= r;
	bufptr  += r;
    }
    return size;
}

int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}

char *convertOptCodeInChar(int opt_code){
    switch(opt_code){ // semplice switch che in base al codice (intero) dell'opzione ritorna una stringa contenente il codice:
        case OPEN_FILE:{
            return "OPEN_FILE";
        }
        case READ_FILE:{
            return "READ_FILE";
        }
        case READ_N_FILE:{
            return "READ_N_FILE";
        }
        case WRITE_FILE:{
            return "WRITE_FILE";
        }
        case APPEND_TO_FILE:{
            return "APPEND_TO_FILE";
        }
        case LOCK_FILE:{
            return "LOCK_FILE";
        }
        case UNLOCK_FILE:{
            return "UNLOCK_FILE";
        }
        case CLOSE_FILE:{
            return "CLOSE_FILE";
        }
        case REMOVE_FILE:{
            return "REMOVE_FILE";
        }
        case CLOSE_CONNECTION:{
            return "CLOSE_CONNECTION";
        }
        case OPEN_CONNECTION:{
            return "OPEN_CONNECTION";
        }
        case O_CREATE:{
            return "O_CREATE";
        }
        case O_LOCK:{
            return "O_LOCK";
        }
        case O_LOCKED_CREATE:{
            return "O_LOCKED_CREATE";
        }
        case O_NOFLAG:{
            return "O_NOFLAG";
        }
        case 0:{
            return "RESPONSE";
        }
        default:{
            ;
        }
    }
    return "ERRORE";
}

void setMsg(Msg *msg,int opt_code, int flags, char response_code[RESPONSE_CODE_LENGTH], char path[RESPONSE_CODE_LENGTH], char *data, size_t size){
    //inserisco nei campi di msg le info passate per argomento:
    msg->opt_code = opt_code;
    msg->flags = flags;
    strcpy(msg->response_code, response_code);
    strcpy(msg->path, path);
    if(data == NULL){
        msg->data = NULL;
        msg->data_length = -1;
    }
    else{
        msg->data_length = size;
        msg->data = smalloc(sizeof(char)*msg->data_length);
        memcpy(msg->data, data, msg->data_length);
    }
}

int sendMsg(Msg *to_send, long fd){
    CHECK_NULL("Send Msg NULL",to_send);

    to_send->pid = getpid();

    if(write(fd, to_send, sizeof(Msg)) == -1){//spedisco la struttura Msg (i campi char* saranno vuoti)
        fprintf(stderr, "ERRORE: write messaggio.\n");
        printINFOMsg(to_send);
        return -1;
    }

    if(to_send->data != NULL && to_send->data_length > 0){         //se è presente
        if(writen(fd, to_send->data, to_send->data_length) == -1){ //spedisco il data a parte con writen per evitare scritture parziali
            fprintf(stderr, "ERRORE: write data.\n");
            return -1;
        }

        free(to_send->data);
    }

    return 0;
}

int receiveMsg(Msg *to_receive, long fd){
    CHECK_NULL("Send Msg NULL",to_receive);
    //in modo speculare al sendMsg:
    if(read(fd, to_receive, sizeof(Msg)) == -1){//ricevo il Msg con i campi char* vuoti
        fprintf(stderr, "ERRORE: read messaggio vuoto.\n");
        printINFOMsg(to_receive);
        return -1;
    }

    if(to_receive->data_length < 0){//se il data_lenght è >0 vuol dire che ho un data da leggere
        to_receive->data = NULL;
    }
    else{
        to_receive->data = smalloc(sizeof(char)*to_receive->data_length); //alloco memoria per il data
        if(readn(fd, to_receive->data, to_receive->data_length) == -1){   //ricevo il data con readn per evitare letture parziali
            fprintf(stderr, "ERRORE: read data.\n");
            return -1;
        }
    }

    return 0;
}

void printINFOMsg(Msg *to_print){//funzione utile al debugging
    printf("INFO MESSAGGIO di tipo %s(%d):\n",convertOptCodeInChar(to_print->opt_code), to_print->opt_code);

    if(to_print->path == NULL){
        printf("\tnessun path definito\n");
    }
    else{
        printf("\tPath = '%s' lungo '%ld'\n", to_print->path, strlen(to_print->path));
    }

    if(to_print->data_length <= 0){
        printf("\tnessun data definito\n");
    }
    else{
        //printf("\tData = '%s' lungo '%d'\n", to_print->data, to_print->data_length);
        printf("\tData lungo %d .\n", to_print->data_length);
    }

    printf("\tResponse code: '%s'\n", to_print->response_code);
}

void saveProgramStatusOnLogFile(FILE *fileLog, ProgramStatus ps, int num_worker, int print_on_stdout){
    //stampo sul file di Log le statistiche del server contenute nella struct 'ps'
    fprintf(fileLog, "\nSTATISTICHE GENERALI DEL SERVER:\n");
    fprintf(fileLog, "Numero connessioni massime: %d\n", ps.max_con);
    fprintf(fileLog, "Numero file creati: %d\n", ps.num_file_create);
    fprintf(fileLog, "Numero file aperti: %d\n", ps.num_file_open);
    fprintf(fileLog, "Numero file eliminati: %d\n", ps.num_file_deleted);
    fprintf(fileLog, "Numero file chiusi: %d\n", ps.num_file_closed);
    fprintf(fileLog, "Numero file bloccati: %d\n", ps.num_file_locked);
    fprintf(fileLog, "Numero file letti: %d\n", ps.num_file_readed);
    fprintf(fileLog, "Numero file scritti: %d\n", ps.num_file_writed);
    fprintf(fileLog, "Numero massimo di file raggiunti: %d\n", ps.num_max_file_reached);
    fprintf(fileLog, "Dimensione massima di Mbytes raggiunta: %f\n", (float)ps.num_max_byte_reached/1000000);
    fprintf(fileLog, "Stampo il numero di richieste per ogni worker:\n");

    //se è stata abilitata la stampa su stdout stampo anche li le statistiche
    if(print_on_stdout){  
        printf("\nSTATISTICHE GENERALI DEL SERVER:\n");
        printf("Numero connessioni massime: %d\n", ps.max_con);
        printf("Numero file creati: %d\n", ps.num_file_create);
        printf("Numero file aperti: %d\n", ps.num_file_open);
        printf("Numero file eliminati: %d\n", ps.num_file_deleted);
        printf("Numero file chiusi: %d\n", ps.num_file_closed);
        printf("Numero file bloccati: %d\n", ps.num_file_locked);
        printf("Numero file letti: %d\n", ps.num_file_readed);
        printf("Numero file scritti: %d\n", ps.num_file_writed);
        printf("Numero massimo di file raggiunti: %d\n", ps.num_max_file_reached);
        printf("Dimensione massima di Mbytes raggiunta: %f\n", (float)ps.num_max_byte_reached/1000000);
        printf("Stampo il numero di richieste per ogni worker(%d):\n", num_worker);
    }

    for(int i=0; i < num_worker; i++){
        fprintf(fileLog, "\tWorker numero %d: %d richieste servite.\n", i, ps.workerRequests[i]);
        if(print_on_stdout){
            printf("\tWorker num %d: %d richieste servite.\n", i, ps.workerRequests[i]);
        }
    }

    return;
}