CC=gcc
SRC=main.c shell.c fs.c
EXE=sh
CONTAINER=fs_container

all:
	$(CC) -o $(EXE) $(SRC)

clean:
	rm -rf $(EXE) $(CONTAINER)
