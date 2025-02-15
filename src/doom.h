#ifndef DOOM_H

# define DOOM_H

#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define ASSERT(_e, ...) if (!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }

#define FPS 60
#define POV 120
#define FRAME_TARGET_TIME (1000/FPS)
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define BLACK 0

// #define M_PI       3.14159265358979323846
#define degToRad(angleDegrees) (angleDegrees * M_PI / 180.0)
#define radToDeg(angleRadians) (angleRadians * 180.0 / M_PI)


typedef	struct s_player
{
    float x,y;
    float   rotation;
    int l; // vertical lookup 
} player;


// Structure of the whole Game State
typedef struct s_game_state
{
    // SDL specifics
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture *texture;
    uint32_t *buffer;

    int lines[100][4];
    bool render_mode;
    bool quit;

    float deltaTime;
    uint32_t l_frameTime;

    // player, input
    player p;
    short int	pressed_keys[26];
} game_state;


// Structure to represent the lump directory entry
typedef struct
{
    char name[8];  // Lump name (8 characters, null-terminated)
    uint32_t offset;  // Offset where the lump starts in the file
    uint32_t size;  // Size of the lump
	//
} WAD_LumpEntry;

// WAD Header (first 4 bytes is "IWAD" or "PWAD")
typedef struct
{
    char magic[4];  // Always "IWAD" or "PWAD"
    uint32_t lump_count;  // Number of lumps in the WAD file
    uint32_t directory_offset;  // Offset where the lump directory starts
	//
} WAD_Header;


// WAD_parser
void parse_wad(game_state *g_, const char *filename);


#endif

