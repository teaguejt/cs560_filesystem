CC=gcc
SRC=main.c shell.c
EXE=sh
CONTAINER=fs_container

all:
	$(CC) -o $(EXE) $(SRC)

clean:
	rm -rf $(EXE) $(CONTAINER)
