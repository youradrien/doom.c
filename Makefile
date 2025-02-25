##CFLAGS = `sdl2-config --libs --cflags`
SRC = $(wildcard ./src/*.c)
OBJ = $(SRC:.c=.o)
CC = gcc
CFLAGS = -Wall
CFLAGS += -std=c99
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-unused-function
INCLUDES = -I./src/*.h
LDFLAGS = $(shell sdl2-config --libs)
CFLAGS += $(shell sdl2-config --cflags)
CFALGS += -fsanitize=address
OS = $(shell uname -s)

EXEC = doom

ifeq ($(OS),Darwin)
	# macOS-specific SDL2 
	INCLUDES += -I/opt/homebrew/include/SDL2
else ifeq ($(OS),Linux)
	 # Linux-specific SDL2
	INCLUDES += /usr/include/SDL2
	INCLUDES += /usr/local/include/SDL2
else
	# Default commands for other systems
	INCLUDES += /mingw64/include/SDL2
endif

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC)  $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS) 

# Rule to build object files from source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -f doom

re: clean all	

run: re 
	./doom
