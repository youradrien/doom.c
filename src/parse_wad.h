#ifndef PARSE_WAD_H
#define PARSE_WAD_H

#include <stdio.h>
#include <stdlib.h>

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
// void parse_wad(game_state *g_, const char *filename);

#endif
