/*
 * =====================================================================================
 *
 *       Filename:  texdump.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/06/2013 19:26:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include "wad.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////////////////////

//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
// 



typedef enum {false, true} boolean;
//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
    short	originx;
    short	originy;
    short	patch;
    short	stepdir;
    short	colormap;
} mappatch_t;


//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
#pragma pack(1)
typedef struct
{
    char		name[8];
    boolean		masked;	
    short		width;
    short		height;
    //void		**columndirectory;	// OBSOLETE
    uint32_t    obsolete;
    short		patchcount;
    //mappatch_t	patches[1];
} maptexture_t;
#pragma pack()

/////////////////////////////////////////////////////////////////////

static void usage(void)
{
	printf("texdump <wad file>\n");
}

// Builds a lookup table with lump numbers for each texture patch lump
static uint32_t* BuildPatchLumpLookup(const wad_file* wad)
{
	// Load patch names lump
	// This lump contains all names of other lumps in this wad file that contain texture patches
	lump_t patchNamesLump;
	int error = LoadLumpName(wad, "PNAMES", &patchNamesLump);
	if (error)
	{
		fprintf(stderr, "Could not load PNAMES lump\n");
		return NULL;
	}

	// PNAMES lump starts with 32bit of total names in lump, each name is 8 bytes long
	uint32_t totalPatchNames = *(uint32_t*) patchNamesLump.data;
	printf("Got total %d patches in PNAMES lump\n", totalPatchNames);
	
	// Build patch lump lookup table with lump numbers
	uint8_t* patchLumpNames = ((uint8_t*) patchNamesLump.data) + sizeof(uint32_t);
	uint32_t* patchLumpLookup = (uint32_t*) malloc(sizeof(uint32_t) * totalPatchNames);

	for (unsigned i = 0; i < totalPatchNames; ++i)
	{
		char patchLumpName[WAD_LUMP_NAME_SIZE + 1] = {0};
		memcpy(patchLumpName, patchLumpNames + (i * WAD_LUMP_NAME_SIZE), WAD_LUMP_NAME_SIZE);

		int patchLumpNum = GetLumpNumberByName(wad, patchLumpName);
		if (patchLumpNum < 0)
		{
			FreeLump(&patchNamesLump);
			fprintf(stderr, "Could not get patch lump number by name %s\n", patchLumpName);
			return NULL;
		}

		patchLumpLookup[i] = patchLumpNum;
		printf("%d: patch lump name %s has number %d\n", i, patchLumpName, patchLumpNum);
	}

	FreeLump(&patchNamesLump);
	return patchLumpLookup;
}


int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("texdump: not enough arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	printf("sizeof short = %d, maptexture_t %d, void** %d, boolean %d\n", sizeof(short), sizeof(maptexture_t), sizeof(void**), sizeof(boolean));
	const char* wadfile = argv[1];
	
	wad_file wad;
	int error = LoadWadFile(wadfile, &wad);
	if (error)
	{
		fprintf(stderr, "Could not load wad file %s: %s\n", wadfile, strerror(error));
		return error;
	}

	// Build texture patch index lookup
	uint32_t* patchLumpLookup = BuildPatchLumpLookup(&wad);
	if (!patchLumpLookup)
	{
		fprintf(stderr, "Failed to build patch lump lookup table\n");
		return EXIT_FAILURE;
	}

	// Load texture lumps
	// Should be 2 lumps for commercial version (which we assume we have)
	lump_t tex1;
	error = LoadLumpName(&wad, "TEXTURE1", &tex1);	
	if (error)
	{
		fprintf(stderr, "Could not load TEXTURE1 lump\n");
		return error;
	}

	lump_t tex2;
	error = LoadLumpName(&wad, "TEXTURE2", &tex2);
	if (error)
	{
		fprintf(stderr, "Could not load TEXTURE2 lump\n");
		return error;
	}
	
	uint32_t tex1count = *(uint32_t*)tex1.data;
	uint32_t tex2count = *(uint32_t*)tex2.data;
	uint32_t totalTextures = tex1count + tex2count;
	printf("Total textures %d (TEXTURE1 %d + TEXTURE2 %d)\n", totalTextures, tex1count, tex2count);

	// Start with first texture lump
	uint8_t* textures = (uint8_t*)tex1.data;
	uint32_t* directory = (uint32_t*)tex1.data + 1;

	printf("\n --- TEXTURE1 --- \n");
	for (unsigned i = 0; i < totalTextures; ++i)
	{
		if (i == tex1count)
		{
			// switch to second lump
			textures = (uint8_t*)tex2.data;
			directory = (uint32_t*)tex2.data + 1;

			printf("\n --- TEXTURE2 --- \n");
		}

		uint32_t offset = *directory++;
		if (offset > tex1.size)
		{
			fprintf(stderr, "Invalid texture %d directory offset %d\n", i, offset);
			return -1;
		}
		
		maptexture_t* mtex = (maptexture_t*) (textures + offset);
		printf("Texture %3d: offset 0x%04x, name %8s, width %3d, height %3d, patches %d\n", 
				i, offset, mtex->name, mtex->width, mtex->height, mtex->patchcount);

	}

	return EXIT_SUCCESS;
}

