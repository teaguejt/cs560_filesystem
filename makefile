CC=gcc
SRC=main.c
EXE=fs_driver
CONTAINER=fs_container

all:
	$(CC) -o $(EXE) $(SRC)

clean:
	rm -rf $(EXE) $(CONTAINER)
