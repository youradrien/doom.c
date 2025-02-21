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

// #define M_PI       3.14159265358979323846
#define DEG2RAD(a) (a * M_PI / 180.0)
#define radToDeg(a) (a * 180.0 / M_PI)
#define PI_2 (M_PI / 2.0f)
#define PI_4 (M_PI / 4.0f)
#define TAU (2.0f * M_PI)

// eye-height of the player
// (he'll be able to crouch)
#define EYE_Z 1.65f
#define FOV 90
// horizontal fov
#define HFOV DEG2RAD(FOV)
// vertical fov
#define VFOV 0.5f

#define ZNEAR 0.0001f
#define ZFAR  128.0f

#define FPS 60
#define FRAME_TARGET_TIME (1000/FPS)
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 500
#define BLACK 0
#define C_X WINDOW_WIDTH/2
#define C_Y WINDOW_HEIGHT/2
#define normalize(_vn) ({ __typeof__(_vn) __vn = (_vn); const f32 l = length(__vn); (__typeof__(_vn)) { __vn.x / l, __vn.y / l }; })
#define ifnan(_x, _alt) ({ __typeof__(_x) __x = (_x); isnan(__x) ? (_alt) : __x; })


typedef float f32;

typedef struct v2_s
{
    f32 x;
    f32 y;
} v2;

typedef	struct s_player
{ 
    v2	pos;
    float   rotation;
} player;

typedef struct s_wall
{ 
    v2	a;
    v2	b;
    uint32_t color;
} wall;

typedef struct s_scene
{
    wall *_walls;
    int	 _walls_ix;
}   t_scene;


// Structure of the whole Game State
typedef struct s_game_state
{
    // SDL specifics
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture *texture;
    uint32_t *buffer;

    bool render_mode;
    bool quit;
    t_scene scene;

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

