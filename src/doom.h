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
#include <unistd.h> 
#include <fcntl.h>
#include <limits.h>
#include <float.h>

#define ASSERT(_e, ...) if (!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }

// #define M_PI       3.14159265358979323846
#define DEG2RAD(a) (a * M_PI / 180.0)
#define radToDeg(a) (a * 180.0 / M_PI)
#define PI_2 (M_PI / 2.0f)
#define PI_4 (M_PI / 4.0f)
#define TAU (2.0f * M_PI)
#define EPSILON 1e-2  // Small tolerance to check proximity

// eye-height of the player
// (he'll be able to crouch)
#define EYE_Z 1.65f
#define FOV 60
// horizontal fov
#define HFOV DEG2RAD(FOV)
// vertical fov
#define VFOV 0.65f

#define ZNEAR 0.01f
#define ZFAR  256.0f

#define MAP_TILE 3
#define MAP_ZOOM 5

#define P_HEIGHT 7.0


#define FPS 60
#define FRAME_TARGET_TIME (1000/FPS)
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define BLACK 0
#define C_X WINDOW_WIDTH/2
#define C_Y WINDOW_HEIGHT/2
#define normalize(_vn) ({ __typeof__(_vn) __vn = (_vn); const f32 l = length(__vn); (__typeof__(_vn)) { __vn.x / l, __vn.y / l }; })
#define ifnan(_x, _alt) ({ __typeof__(_x) __x = (_x); isnan(__x) ? (_alt) : __x; })
#define screenclamp(_a)	((_a == WINDOW_WIDTH) ? (WINDOW_WIDTH - 1) : (_a))
/* #define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; }) */
#define min(_a, _b) ({ __typeof__(_a) __a = (_a), __b = (_b); __a < __b ? __a : __b; })
#define max(_a, _b) ({ __typeof__(_a) __a = (_a), __b = (_b); __a > __b ? __a : __b; })
#define clamp(_x, _mi, _ma) (min(max(_x, _mi), _ma))


#define v2_to_v2i(_v) ({ __typeof__(_v) __v = (_v); (v2i) { __v.x, __v.y }; })
#define v2i_to_v2(_v) ({ __typeof__(_v) __v = (_v); (v2) { __v.x, __v.y }; })

#define PLAYER_RADIUS 1.6

typedef float f32;
typedef int32_t  i32;
typedef struct v2_s
{
    f32 x;
    f32 y;
} v2;
typedef struct v2i_s 
{ 
    i32 x, y; 
} v2i;

typedef struct s_sector s_sector;
typedef struct s_wall s_wall;

typedef struct s_sector
{
    v2		*vertices[256];
    uint16_t	n_vertices;

    s_wall	*walls[256];
    uint32_t	n_walls;

    f32		floor;
    f32		ceil;

    s_sector	**portals;
    uint16_t	n_portals;

    int16_t	id;
}   sector;

//  [walls | linedefs]
typedef struct s_wall
{ 
    // ptrs to vertices
    v2	*a;
    v2	*b;

    sector  *_op_sect;
    sector  *_sector;
    bool    _in_bsp;
} wall;

typedef struct t_BSP_node t_BSP_node;
typedef struct	t_BSP_node
{
    t_BSP_node *_front;
    t_BSP_node *_back;
    wall    *_partition;
}   BSP_node;

typedef	struct s_player
{ 
    v2	pos;
    f32	height;
   
    // rotations
    f32	rotation;
    f32 anglecos, anglesin;

    // map-positions
    BSP_node *_node;
    sector *_sect;

    // standings
    bool _crouch;
    bool _crawl;
} player;



// "QUEUE" of sectors
#define MAX_QUEUE 128 // 128
#define MAX_SECTORS 512
typedef struct 
{
    sector *s;
    int x1;
    int x2;
} queue_entry;

typedef struct s_scene
{
    // walls (linedefs) 
    wall *_walls;
    int	 _walls_ix;

    // vertices
    v2	*_verts;
    uint32_t _verts_count;

    // sectors
    sector	*_sectors;
    uint32_t	_sectors_count;	

    // bsp tree
    BSP_node	*bsp_head;
    unsigned int _bsp_depth;

    // uncovered vertical-arrays
    int y_lo[WINDOW_WIDTH];
    int y_hi[WINDOW_WIDTH];

    // rendering queue 
    queue_entry _queue[MAX_QUEUE];
    char visited[MAX_SECTORS]; // per-sector visited flag
}   t_scene;



// structure of the whole Game State
typedef struct s_game_state
{
    // SDL specifics
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture *texture;
    uint32_t *buffer;

    bool render_mode;
    bool quit;

    // whole scene
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

#define COLOR_COUNT 140  // Total number of colors

static const uint32_t g_colors[COLOR_COUNT] = {
    0xFF0000, // Red
    0x800000, // Maroon
    0xFFA500, // Orange
    0xFFFF00, // Yellow
    0x808000, // Olive
    0x008000, // Green
    0x00FF00, // Lime
    0x008080, // Teal
    0x00FFFF, // Cyan
    0x0000FF, // Blue
    0x000080, // Navy
    0x800080, // Purple
    0xFF00FF, // Magenta
    0xFFC0CB, // Pink
    0xFFFFFF, // White
    0xC0C0C0, // Silver
    0x808080, // Gray
    0x000000, // Black

    // Extended Colors
    0xDC143C, // Crimson
    0xFF4500, // Orange Red
    0xFFD700, // Gold
    0xDAA520, // Goldenrod
    0xADFF2F, // Green Yellow
    0x7FFF00, // Chartreuse
    0x32CD32, // Lime Green
    0x228B22, // Forest Green
    0x2E8B57, // Sea Green
    0x48D1CC, // Medium Turquoise
    0x20B2AA, // Light Sea Green
    0x5F9EA0, // Cadet Blue
    0x4682B4, // Steel Blue
    0x1E90FF, // Dodger Blue
    0x4169E1, // Royal Blue
    0x8A2BE2, // Blue Violet
    0x9932CC, // Dark Orchid
    0x9400D3, // Dark Violet
    0x8B008B, // Dark Magenta
    0xFF1493, // Deep Pink
    0xC71585, // Medium Violet Red
    0xDB7093, // Pale Violet Red
    0xFF69B4, // Hot Pink
    0xFA8072, // Salmon
    0xE9967A, // Dark Salmon
    0xF08080, // Light Coral
    0xCD5C5C, // Indian Red
    0xA52A2A, // Brown
    0x8B4513, // Saddle Brown
    0xD2691E, // Chocolate
    0xF4A460, // Sandy Brown
    0xB22222, // Firebrick
    0xFF8C00, // Dark Orange
    0xFFA07A, // Light Salmon
    0xFFDAB9, // Peach Puff
    0xEEE8AA, // Pale Goldenrod
    0xFAFAD2, // Light Goldenrod Yellow
    0xD8BFD8, // Thistle
    0xDDA0DD, // Plum
    0xEE82EE, // Violet
    0xBA55D3, // Medium Orchid
    0x9370DB, // Medium Purple
    0xDA70D6, // Orchid
    0xFFB6C1, // Light Pink
    0xF5DEB3, // Wheat
    0xDEB887, // Burlywood
    0xBC8F8F, // Rosy Brown
    0xA9A9A9, // Dark Gray
    0x696969, // Dim Gray
    0x556B2F, // Dark Olive Green
    0x6B8E23, // Olive Drab
    0x9ACD32, // Yellow Green
    0x7CFC00, // Lawn Green
    0x00FA9A, // Medium Spring Green
    0x00FF7F, // Spring Green
    0x3CB371, // Medium Sea Green
    0x66CDAA, // Medium Aquamarine
    0x7FFFD4, // Aquamarine
    0xB0E0E6, // Powder Blue
    0xADD8E6, // Light Blue
    0x87CEEB, // Sky Blue
    0x87CEFA, // Light Sky Blue
    0x4682B4, // Steel Blue
    0x708090, // Slate Gray
    0x778899, // Light Slate Gray
    0xB0C4DE, // Light Steel Blue
    0xFFFFE0, // Light Yellow
    0xFFFACD, // Lemon Chiffon
    0xFAF0E6, // Linen
    0xFDF5E6, // Old Lace
    0xFFE4E1, // Misty Rose
    0xFFF0F5, // Lavender Blush
    0xE6E6FA, // Lavender
    0xF5F5DC, // Beige
    0xF5F5F5, // White Smoke
    0xDCDCDC, // Gainsboro
    0xFFFAFA, // Snow
    0x00008B, // Dark Blue
    0x8B0000, // Dark Red
    0x006400, // Dark Green
    0x808000, // Olive
    0x008B8B, // Dark Cyan
    0xB8860B, // Dark Goldenrod
    0xFF6347, // Tomato
    0x8B4513, // Saddle Brown
    0x2F4F4F, // Dark Slate Gray
    0x191970, // Midnight Blue
    0x800000, // Maroon
    0x4B0082, // Indigo
    0x6A5ACD, // Slate Blue
    0x48D1CC, // Medium Turquoise
    0xA0522D, // Sienna
    0xD2B48C, // Tan
    0x708090, // Slate Gray
    0xF0E68C, // Khaki
    0x8FBC8F, // Dark Sea Green
    0x4682B4, // Steel Blue
    0xE0FFFF, // Light Cyan
    0x20B2AA, // Light Sea Green
    0xF0FFF0, // Honeydew
    0xFFF5EE, // Seashell
    0xF5FFFA, // Mint Cream
    0xB0E0E6, // Powder Blue
    0xAFEEEE, // Pale Turquoise
    0xE6E6FA, // Lavender
    0xF8F8FF, // Ghost White
    0xE0FFFF, // Azure
    0x00FFFF, // Aqua
    0x2E8B57, // Sea Green
    0xFFD700, // Gold
    0x8B008B  // Dark Magenta
};

// WAD_parser
void parse_wad(game_state *g_, const char *filename);


#endif

