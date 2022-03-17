#
# MAKEFILE per la compilazione ed esecuzione del progetto
#

#cartella dei file headers:
HEADERS_DIRECTORY = includes

#cartella dei file oggetto:
OBJECT_DIRECTORY = object

#cartella script bash:
SCRIPT_DIRECTORY = scriptBash

#compilazione:
CC = gcc
CFLAGS += -g -std=c99 -Wall -pthread
INCLUDES = -I./$(HEADERS_DIRECTORY)


.PHONY: all clean client server test1 test2 test3

all: clean client server

client: $(OBJECT_DIRECTORY)/utils.o $(OBJECT_DIRECTORY)/clientAPI.o $(OBJECT_DIRECTORY)/optList.o
	$(CC) $(CFLAGS) $(INCLUDES) client.c -o client $(OBJECT_DIRECTORY)/utils.o $(OBJECT_DIRECTORY)/clientAPI.o $(OBJECT_DIRECTORY)/optList.o

server: $(OBJECT_DIRECTORY)/utils.o $(OBJECT_DIRECTORY)/fileParser.o $(OBJECT_DIRECTORY)/icl_hash.o $(OBJECT_DIRECTORY)/codaFile.o $(OBJECT_DIRECTORY)/codaTask.o
	$(CC) $(CFLAGS) $(INCLUDES) server.c -o server $(OBJECT_DIRECTORY)/utils.o $(OBJECT_DIRECTORY)/fileParser.o $(OBJECT_DIRECTORY)/icl_hash.o $(OBJECT_DIRECTORY)/codaFile.o $(OBJECT_DIRECTORY)/codaTask.o

#creazione file oggetti:
$(OBJECT_DIRECTORY)/%.o: %.c $(HEADERS_DIRECTORY)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECT_DIRECTORY)/*.o
	rm -r -f $(OBJECT_DIRECTORY)
	mkdir $(OBJECT_DIRECTORY)
	rm -f *.sk
	rm -f server
	rm -f client
	rm -f ./fileServer/expFile/*.txt
	rm -f ./fileServer/saveFile/*.txt

test1:
	$(MAKE) -s all
	bash $(SCRIPT_DIRECTORY)/test1.sh
	bash $(SCRIPT_DIRECTORY)/statistiche.sh logFile.txt

test2:
	$(MAKE) -s all
	bash $(SCRIPT_DIRECTORY)/test2.sh
	bash $(SCRIPT_DIRECTORY)/statistiche.sh logFile.txt

test3:
	$(MAKE) -s all
	bash $(SCRIPT_DIRECTORY)/test3.sh
	bash $(SCRIPT_DIRECTORY)/statistiche.sh logFile.txt
	