/*
 * =====================================================================================
 *
 *       Filename:  wad.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/06/2013 18:34:05
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef EFI_DOOM_TOOLS_WAD_H
#define EFI_DOOM_TOOLS_WAD_H

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////

// File structures

#define WAD_ID_SIZE 4
#define WAD_LUMP_NAME_SIZE 8

typedef struct
{
	char id[WAD_ID_SIZE];
	uint32_t numlumps;
	uint32_t offset;
} wad_info;

typedef struct
{
	uint32_t fileoff;
	uint32_t size;
	char name[WAD_LUMP_NAME_SIZE];
} lump_info;

// Runtime structures

typedef struct 
{
	int fd; 				// File descriptor
	uint32_t numlumps;		// Total lumps in this WAD file
	uint32_t offset;
	lump_info* lumpTable;	// Cached table of lump info structured
} wad_file;

typedef struct 
{
	char name[WAD_LUMP_NAME_SIZE + 1]; 	// Name, +1 for extra 0 byte
	int num;		// Lump number in WAD file
	void* data;		// Data 
	uint32_t size;	// Data size
} lump_t;

////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

// Load and parse WAD file.
int LoadWadFile(const char* path, wad_file* outFile);

// Check that there is a lump with this name in wad file and get its info initialized on success
int CheckLumpName(const wad_file* wad, const char* lumpName, lump_info* outLump);
int CheckLumpNum(const wad_file* wad, int num, lump_info* outLump);

// Return a lump number with a given name.
// Returns either a positive lump number or a negative error code
int GetLumpNumberByName(const wad_file* wad, const char* name);

// Load lump data by its name, caller has to free returned buffer 
int LoadLump(const wad_file* wad, const lump_info* lump, lump_t* outLump);
int LoadLumpName(const wad_file* wad, const char* lumpName, lump_t* outLump);
int LoadLumpNum(const wad_file* wad, int lumpNum, lump_t* outLump);

void FreeLump(lump_t* lump);

// Close wad file handle
void CloseWadFile(wad_file* wad);

#ifdef __cplusplus
}
#endif

////////////////////////////////////////////////////////////////////////////

#endif // EFI_DOOM_TOOLS_WAD_H

