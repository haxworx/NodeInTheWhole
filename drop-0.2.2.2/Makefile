SOURCE=src/drop.c
INSTALL_DIR=/usr/local/bin
CFLAGS=-std=c99 -Wall -Werror -pedantic 
EXEC=drop
LIBS=-lssl -lcrypto 

default:
	gcc $(CFLAGS) $(SOURCE) $(LIBS) -o $(EXEC)

install:
	make
	cp $(EXEC) $(INSTALL_DIR)

clean:
	rm $(EXEC)

uninstall:
	make clean
	-rm /usr/local/bin/$(EXEC)
