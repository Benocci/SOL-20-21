/*******************************************************************************************
FILEPARSER.h: header dei metodi per la creazione di un file parsed e di un configFile

AUTORE: FRANCESCO BENOCCI

NOTE: 

*********************************************************************************************/
#ifndef FILEPARSER_H
#define FILEPARSER_H

#define MAX_STRING_LEN_PATH 512
#define MAX_STRING_LEN 50

//struttura simile ad una lista che mantiene una stringa e 
//un puntatore all'elemento successivo
typedef struct _tokenizedFile{
    char word[MAX_STRING_LEN];
    struct _tokenizedFile *nextPtr;
}TokenizedFile;

//struttura che mantiene testa, coda, nome e delimitatore
//del file parsato
typedef struct _parsedFile{
    char filename[MAX_STRING_LEN_PATH];
    char delim[MAX_STRING_LEN];
    struct _tokenizedFile *head;
    struct _tokenizedFile *tail;
}ParsedFile;

//struttura che mantiene le informazione di un file di config
typedef struct _configFile{
    int capacity;
    int numFile;
    int numWorker;
    char socketName[MAX_STRING_LEN];
    int maxPendingConnection;
    char fileLogName[MAX_STRING_LEN];
}ConfigFile;

/* EFFETTO: crea un ParsedFile* partendo da 'filename' e 'delim'
 *
 * RITORNA: NULL -> errore in creazione
 *          new -> creazione avvenuta con successo
 */
ParsedFile *parseFile(char *filename, char *delim);

/* EFFETTO: elimina il ParsedFile* 'pF'
 *
 * RITORNA: -1 -> errore in eliminazione settando errno
 *           0 -> eliminazione avvenuta con successo
 */
int deleteParsedFile(ParsedFile *pF);

/* EFFETTO: crea un ConfigFile* partendo dal ParsedFile* "config"
 *
 * RITORNA: NULL -> errore in creazione
 *          new -> creazione avvenuta con successo
 */
ConfigFile* configFileCheck(ParsedFile *config);

#endif