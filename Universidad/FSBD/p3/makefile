CC = gcc -g
CFLAGS = -Wall -Wextra -pedantic -std=c99

FILENAME = "test"
HEADERS = library.h
EXE = library
OBJ = $(EXE).o


all : $(EXE)

# compile all files ending in *.c
%.o: %.c $(HEADERS)
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c -o $@ $<

# link binary
$(EXE): $(DEPS) $(OBJ)
	$(CC) -o $(EXE) $(OBJ) $(LDLIBS)

clean:
	rm -f *.o core $(EXE) 

run:
	./$(EXE) worst_fit $(FILENAME)

valgrind:
	valgrind --leak-check=full ./$(EXE)