all: librarian reader
librarian: server/librarian.c utils/utils.c utils/utils.h
	gcc -o librarian server/librarian.c utils/utils.c -I utils
reader: client/reader.c utils/utils.c utils/utils.h
	gcc -o reader client/reader.c utils/utils.c -I utils

clean:
	rm librarian
	rm reader
