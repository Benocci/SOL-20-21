#! /bin/bash

# SECONDO FILE DI TEST

# VERIFICA DI FUNZIONAMENTO DELL'ALGORITMO DI RIMPIAZZAMENTO


# Avvio server:
./server ./config/config2.txt &

# mantengo il pid del processo per mandare SIGHUP:
pid=$!

sleep 2

# Avvio i client:
./client -f mysocket.sk -p -t 200 -W ./fileClient/file1.txt,./fileClient/file2.txt,./fileClient/file8.txt -D ./fileServer/expFile/ &

./client -f mysocket.sk -p -t 2000 -w ./fileClient/testfile1/,n=1 -D ./fileServer/expFile/ &

./client -f mysocket.sk -p -t 1000 -W ./fileClient/file4.txt,./fileClient/file3.txt -D ./fileServer/expFile/ &

./client -f mysocket.sk -p -t 1500 -w ./fileClient/testfile2/,n=4 -D ./fileServer/expFile/ &

./client -f mysocket.sk -p -t 500 -W ./fileClient/file7.txt,./fileClient/file6.txt,./fileClient/file5.txt -D ./fileServer/expFile/ &

./client -f mysocket.sk -p -t 200 -W ./fileClient/file1.txt,./fileClient/file2.txt,./fileClient/file8.txt -D ./fileServer/expFile/ &

# attendo l'esecuzione dei client
sleep 3

# termino il sever mandandogli SIGHUP
kill -s SIGHUP $pid

wait $pid

echo -e -n "Fine Test2.\n"

exit 0