#! /bin/bash

# PRIMO FILE DI TEST

# TEST DEL SERVER CON VALGRIND –leak-check=full

# Avvio del server: 
valgrind --leak-check=full ./server ./config/config1.txt &

# mantengo il pid del processo per mandare SIGHUP
pid=$!

sleep 5

# Avvio i client:
./client -f mysocket.sk -p -t 200 -w ./fileClient/testfile2/,n=3

./client -f mysocket.sk -p -t 200 -W ./fileClient/file1.txt,./fileClient/file3.txt

./client -f mysocket.sk -p -t 200 -R 0 -W ./fileClient/file1.txt

./client -f mysocket.sk -p -t 200 -c ./fileClient/file3.txt

./client -f mysocket.sk -p -t 200 -r ./fileClient/testfile2/file11.txt -d ./fileServer/saveFile/

./client -f mysocket.sk -p -t 200 -l ./fileClient/file1.txt -r ./fileClient/file1.txt -u ./fileClient/file1.txt



# attendo l'esecuzione dei client
sleep 3

# termino il sever mandandogli SIGHUP
kill -s SIGHUP $pid

wait $pid

echo -e -n "Fine Test1.\n"

exit 0