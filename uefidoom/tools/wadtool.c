/*
 * =====================================================================================
 *
 *       Filename:  wadtool.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/06/2013 18:57:46
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

static void usage(void)
{
	printf("wad info <wad file>\n");
	printf("wad dump <wad file> <lump name>\n");
	printf("wad unpack <wad file> <output folder>\n");
}

// wad info <wadfile>
static int DoInfo(int argc, char** argv)
{
	assert (0 == strcmp(argv[0], "info"));
	
	if (argc != 2)
	{
		usage();
		return EXIT_FAILURE;
	}

	const char* wadPath = argv[1];

	wad_file wad;
	int error = LoadWadFile(wadPath, &wad);
	if (error)
	{
		printf("Failed opening wad file %s: %s\n", wadPath, strerror(error));
		return error;	
	}

	printf("\nWAD file %s:\n", wadPath);
	printf("> total lumps = %d\n", wad.numlumps);
	printf("> lump table offset = %d\n", wad.offset);

	printf("\nLumps info:\n");

	for (unsigned i = 0; i < wad.numlumps; ++i)
	{
		lump_info lump;
		error = CheckLumpNum(&wad, i, &lump);
		if (error)
		{
			printf("Failed reading lump %d: %s\n", i, strerror(error));
			return error;
		}

		char namebuf[sizeof(lump.name) + 1] = {0};
		memcpy(namebuf, lump.name, sizeof(lump.name));

		printf("> %8s: offset %d, size %d\n", namebuf, lump.fileoff, lump.size);
	}

	CloseWadFile(&wad);
	return EXIT_SUCCESS;
}

// wad dump <wadfile> <lumpname>
static int DoDump(int argc, char** argv)
{
	assert (0 == strcmp(argv[0], "dump"));
	
	if (argc != 3)
	{
		usage();
		return EXIT_FAILURE;
	}

	const char* lumpName = argv[2];
	const char* wadPath = argv[1];

	wad_file wad;
	int error = LoadWadFile(wadPath, &wad);
	if (error)
	{
		printf("Failed opening wad file %s: %s\n", wadPath, strerror(error));
		return error;	
	}

	lump_t lump;
	error = LoadLumpName(&wad, lumpName, &lump);
	if (error)
	{
		printf("Could not find lump name %s\n", lumpName);
		return error;
	}

	for (size_t j = 0; j < lump.size; ++j)
	{
		if (0 == (j % 32))
		{
			printf("\n");
		}

		printf("%02x ", ((uint8_t*)lump.data)[j]);
	}

	printf("\n");

	FreeLump(&lump);
	CloseWadFile(&wad);

	return 0;
}

static int DoUnpack(int argc, char** argv)
{
	assert (0 == strcmp(argv[0], "unpack"));
	
	if (argc != 3)
	{
		usage();
		return EXIT_FAILURE;
	}

	const char* wadPath = argv[1];
	const char* dirPath = argv[2];

	wad_file wad;
	int error = LoadWadFile(wadPath, &wad);
	if (error)
	{
		printf("Failed opening wad file %s: %s\n", wadPath, strerror(error));
		return error;	
	}

	// Dump all lumps into files in a folder
	for (unsigned i = 0; i < wad.numlumps; ++i)
	{
		lump_t lump;
		error = LoadLumpNum(&wad, i, &lump);
		if (error)
		{
			printf("Could not load lump %d\n", i);
			return error;
		}

		char pathbuf[1024] = {0};
		snprintf(pathbuf, sizeof(pathbuf) - 1, "%s/%s", dirPath, lump.name);

		// some lumps have the same name, so append
		int lump_fd = open(pathbuf, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
		if (lump_fd < 0)
		{
			printf("Failed to create lump dump file %s: %s\n", pathbuf, strerror(errno));
			return errno;
		}

		if (lump.size != write(lump_fd, lump.data, lump.size))
		{
			printf("Failed writing lump data: %s\n", strerror(errno));
			return errno;
		}

		close(lump_fd);
		FreeLump(&lump);
	}

	CloseWadFile(&wad);
	return 0;
}


int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("wad: not enough arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	const char* command = argv[1];
	++argv;
	--argc;

	if (0 == strcmp(command, "info"))
	{
		return DoInfo(argc, argv);
	}
	else if (0 == strcmp(command, "dump"))
	{
		return DoDump(argc, argv);
	}
	else if (0 == strcmp(command, "unpack"))
	{
		return DoUnpack(argc, argv);
	}
	else
	{
		usage();
		return EXIT_FAILURE;
	}
}

