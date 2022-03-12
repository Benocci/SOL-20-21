#! /bin/bash

while true;do
    ./client -f mysocket.sk -t 0 -W ./fileClient/file1.txt,./fileClient/file2.txt,./fileClient/file8.txt -D ./fileServer/expFile/ -r ./fileClient/file2.txt,./fileClient/file8.txt

    ./client -f mysocket.sk -t 0 -w ./fileClient/testfile2/,n=3 -c ./fileClient/testfile2/file11.txt -l ./fileClient/testfile1/file9.txt

    ./client -f mysocket.sk -t 0 -w ./fileClient/testfile1/,n=1 -R 3 -c ./fileClient/testfile1/file1.txt
    
    ./client -f mysocket.sk -t 0 -l ./fileClient/file7.txt -W ./fileClient/file7.txt -r ./fileClient/file7.txt -u ./fileClient/file7.txt

    ./client -f mysocket.sk -t 0 -R 0 -c ./fileClient/file1.txt -l ./fileClient/file3.txt -u ./fileClient/file3.txt
    
    ./client -f mysocket.sk -t 0 -W ./fileClient/file7.txt -W ./fileClient/file7.txt -r ./fileClient/file7.txt

    ./client -f mysocket.sk -t 0 -W ./fileClient/file8.txt,./fileClient/testfile1/file9.txt -c ./fileClient/testfile1/testfile2/file11.txt,./fileClient/testfile1/testfile2/file12.txt,./fileClient/testfile1/testfile2/file13.txt

    ./client -f mysocket.sk -t 0 -c ./fileClient/testfile1/file9.txt -W ./fileClient/testfile1/file9.txt -R 0 -d ./fileServer/saveFile

    ./client -f mysocket.sk -t 0 -R 0 -W ./fileClient/file6.txt -w ./fileClient/testfile1/,n=1 -u ./fileClient/testfile1/file9.txt

    ./client -f mysocket.sk -t 0 -r ./fileClient/testfile1/file9.txt -c ./fileClient/testfile1/file6.txt,./fileClient/testfile1/file8.txt
    
done