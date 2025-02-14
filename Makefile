CFLAGS = `sdl2-config --libs --cflags`
CC = gcc
CFLAGS = -Wall
CFLAGS += -std=c99
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-unused-function
INCLUDES = -I./src/*.h
CFLAGS += $(shell sdl2-config --libs --cflags)
CFLGS += -fsanitize=address
OS = $(shell uname -s)

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

build:
	$(CC)  $(CFLAGS)  ./src/*.c  -o doom


clean:
	rm -f doom

re: clean build	

run: re 
	./doom
