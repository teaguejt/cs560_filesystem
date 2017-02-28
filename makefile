CC=gcc
SRC=main.c shell.c fs.c file.c
EXE=sh
CONTAINER=fs_container

all:
	$(CC) -g -o $(EXE) $(SRC)

clean:
	rm -rf $(EXE) $(CONTAINER)
