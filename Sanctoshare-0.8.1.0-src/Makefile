SOURCE=src/sanctoshare.c
INSTALL_DIR=/usr/local/bin
CFLAGS=-std=c99 -Wall -Werror -pedantic 
EXEC=sanctoshare
LIBS=-lssl -lcrypto 

default:
	gcc $(CFLAGS) $(SOURCE) $(LIBS) -o $(EXEC)

install:
	make
	cp $(EXEC) $(INSTALL_DIR)

clean:
	rm $(EXEC)

uninstall:
	rm /usr/local/bin/sanctoshare
