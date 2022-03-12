#!/bin/bash

# FILE STATISTICHE.SH che parsa il file di log e ne estrae delle stats.

if [ $# = 0 ];then
    echo "Argomenti insufficienti"
    exit 1
fi

if [ ! -e $1 ];then
    echo "Errore file $1 non esiste."
    exit 1
fi

if [ ! -f $1 ];then
    echo "Errore file $1 speciale."
    exit 1
fi

if [ ! -r $1 ];then 
    echo "Errore file $1 non leggibile."
    exit 1
fi

echo "-------------------------------------------------------------"
echo "Parsing del file di log ed estrazione statistiche:"

# read e size media:
echo -e -n "Numero di read: "
grep "READ_FILE" $1 | wc -l
echo -e -n "Numero di file letti con read_n: "
grep "'READ_N_FILE':" $1 | wc -l
echo -e -n "Numero di byte medi letti: "
grep "byte letti" $1 | cut -d: -f 2 | cut -d" " -f 2 | awk '{SOMMA += $1; NUMFILE += 1} END {if(NUMFILE > 0){print int(SOMMA/NUMFILE) " bytes"}else{print "0 bytes"}}'

# write e size media:
echo -e -n "Numero di write: "
grep "WRITE_FILE" $1 | wc -l
echo -e -n "Numero di append: "
grep "APPEND_TO_FILE" $1 | wc -l
echo -e -n "Numero di byte medi scritti: "
grep "byte scritti" $1 | cut -d: -f 2 | cut -d" " -f 2 | awk '{SOMMA += $1; NUMFILE += 1} END {if(NUMFILE > 0){print int(SOMMA/NUMFILE) " bytes"}else{print "0 bytes"}}'

# lock:
echo -e -n "Numero di lock: "
grep "LOCK_FILE" $1 | wc -l

# open-lock:
echo -e -n "Numero di open-lock: "
grep "O_LOCK" $1 | wc -l

# unlock:
echo -e -n "Numero di unlock: "
grep "UNLOCK_FILE" $1 | wc -l

# close:
echo -e -n "Numero di close: "
grep "CLOSE_FILE" $1 | wc -l

# max Mbytes reached:
grep "Dimensione massima di Mbytes" $1

# max numFile reached:
grep "Numero massimo di file raggiunti" $1

# num di volte in cui è avvenuto rimpiazzamento:
echo -e -n "Numero di volte in cui è avvenuto il rimpiazzamento: "
grep "rimpiazzo il file" $1 | wc -l

# num di richieste servite da ogni worker thread:
echo -e -n "Numero di richieste per ogni worker:\n"
grep "Worker numero" $1

# max connection:
grep "Numero connessioni massime" $1

# errori rilevati:
echo -e -n "Numero di risposte negative dal server: "
grep "ERROR" $1 | wc -l

echo "Fine statistiche."

echo "-------------------------------------------------------------"

exit 0