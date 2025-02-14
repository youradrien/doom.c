#include "doom.h"

void parse_wad(game_state *g_, const char *filename)
{
    FILE *file = fopen(filename, "rb"); // Open the WAD file in binary mode
    if (file == NULL) {
        perror("Error opening WAD file");
        return;
    }

    // Read the WAD header
    WAD_Header header;
    fread(&header, sizeof(WAD_Header), 1, file);
    // Check if it's a valid WAD file
    if (strncmp(header.magic, "IWAD", 4) != 0 &&
        strncmp(header.magic, "PWAD", 4) != 0) {
        printf("Invalid WAD file: %s\n", filename);
        fclose(file);
        return;
    }

    printf("WAD File: %s\n", filename);
    printf("WAD Type: %s\n", header.magic);
    printf("Number of Lumps: %u\n", header.lump_count);
    printf("Directory Offset: %u\n", header.directory_offset);

    // Seek to the lump directory and read the lumps
    fseek(file, header.directory_offset, SEEK_SET);

    // Read the lump directory entries
    WAD_LumpEntry *lumps = malloc(sizeof(WAD_LumpEntry) * header.lump_count);
    if (lumps == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return;
    }
    fread(lumps, sizeof(WAD_LumpEntry), header.lump_count, file);

    // Print the lump names
    printf("\nLumps in WAD file:\n");
    for (uint32_t i = 0; i < header.lump_count; i++)
    {
        // Null-terminate the lump name and print it
        lumps[i].name[7] = '\0'; // Ensure null-termination
        printf("Lump %u: %s, Offset: %u, Size: %u\n", i, lumps[i].name,
        lumps[i].offset, lumps[i].size);
  }
  free(lumps);
  fclose(file);
}
