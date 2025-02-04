########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC = gcc
CFLAGS = -Wall -ansi -pedantic

# Makefile settings - Can be customized.
APPNAME = antgame

########################################################################
####################### Targets beginning here #########################
########################################################################

all: antgame

# Builds the game
$(APPNAME): space.o command.o game.o game_actions.o graphic_engine.o game_loop.c libscreen.a
	$(CC) -o $@ $^

# Creates the dependecy rules
space.o: space.c
	$(CC) $(CFLAGS) -c $^

command.o: command.c
	$(CC) $(CFLAGS) -c $^

game.o: game.c
	$(CC) $(CFLAGS) -c $^

game_actions.o: game_actions.c
	$(CC) $(CFLAGS) -c $^

graphic_engine.o: graphic_engine.c
	$(CC) $(CFLAGS) -c $^

################### Cleaning rules for Unix-based OS ###################
# Cleans complete project
.PHONY: clean
clean:
	rm -f *.o $(APPNAME)