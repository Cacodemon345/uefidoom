/*
 * =====================================================================================
 *
 *       Filename:  wad.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/06/2013 00:22:34
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wad.h"

////////////////////////////////////////////////////////////////////////////

int LoadWadFile(const char* path, wad_file* outFile)
{
	if (!outFile)
	{
		return EINVAL;
	}

	if (0 != access(path, R_OK))
	{
		fprintf(stderr, "LoadWadFile: invalid file path: %s\n", strerror(errno));
		return errno;
	}

	// Things we need to clear on error
	int error = 0;
	int fd = -1;
	lump_info* lumpTable = NULL;

	// Will not exit from this loop on success.
	do {
		int fd = open(path, O_RDONLY | O_EXCL, S_IRWXU);
		if (fd < 0)
		{
			fprintf(stderr, "LoadWadFile: failed to open wad file: %s\n", strerror(errno));
			error = errno;
			break;
		}
		
		wad_info header;
		ssize_t nbytes = read(fd, &header, sizeof(header));
		if (nbytes != sizeof(header))
		{
			fprintf(stderr, "LoadWadFile: failed to read wad header: %s\n", strerror(errno));
			error = errno;
			break;
		}

		if ((0 != memcmp(header.id, "IWAD", sizeof(header.id))) && (0 != memcmp(header.id, "PWAD", sizeof(header.id))))
		{
			fprintf(stderr, "LoadWadFile: invalid was id\n");
			error = EINVAL;
			break;
		}

		size_t lumpTableSize = sizeof(lump_info) * header.numlumps;
		lumpTable = (lump_info*) malloc(lumpTableSize);
		if (!lumpTable)
		{
			fprintf(stderr, "LoadWadFile: could not allocate memory for lump table cache\n");
			error = ENOMEM;
			break;
		}

		if (lumpTableSize != pread(fd, lumpTable, lumpTableSize, header.offset))
		{
			fprintf(stderr, "LoadWadFile: failed to read lump table: %s\n", strerror(errno));
			error = errno;
			break;
		}

		outFile->fd = fd;
		outFile->numlumps = header.numlumps;
		outFile->offset = header.offset;
		outFile->lumpTable = lumpTable;

		return 0;

	} while(0);

	// error cleanup
	if (fd > 0) close(fd);
	if (lumpTable != NULL) free(lumpTable);

	return error;
}

int CheckLumpName(const wad_file* wad, const char* lumpName, lump_info* outLump)
{
	if (!wad)
	{
		return EINVAL;
	}

	if (!lumpName)
	{
		return EINVAL;
	}

	if (!outLump)
	{
		return EINVAL;
	}

	for (unsigned i = 0; i < wad->numlumps; ++i)
	{
		if (0 == strncasecmp(wad->lumpTable[i].name, lumpName, WAD_LUMP_NAME_SIZE))
		{
			memcpy(outLump, &wad->lumpTable[i], sizeof(lump_info));
			return 0;
		}
	}

	return ENOENT;
}

int CheckLumpNum(const wad_file* wad, int num, lump_info* outLump)
{
	if (!wad)
	{
		return EINVAL;
	}

	if (!outLump)
	{
		return EINVAL;
	}

	if (num >= wad->numlumps)
	{
		return EINVAL;
	}

	memcpy(outLump, &wad->lumpTable[num], sizeof(*outLump));
	return 0;
}

int GetLumpNumberByName(const wad_file* wad, const char* name)
{
	if (!wad)
	{
		return -1;
	}
	
	if (!name)
	{
		return -1;
	}

	for (unsigned i = 0; i < wad->numlumps; ++i)
	{
		if (0 == strncasecmp(wad->lumpTable[i].name, name, WAD_LUMP_NAME_SIZE))
		{
			return i;
		}
	}
	
	return -1;
}

int LoadLump(const wad_file* wad, const lump_info* lumpInfo, lump_t* outLump)
{
	if (!wad)
	{
		return EINVAL;
	}

	if (!lumpInfo)
	{
		return EINVAL;
	}

	if (!outLump)
	{
		return EINVAL;
	}

	void* data = malloc(lumpInfo->size);
	if (!data)
	{
		fprintf(stderr, "LoadLump: could not allocate lump data buffer size %d bytes\n", lumpInfo->size);
		return EINVAL;
	}

	if (lumpInfo->size != pread(wad->fd, data, lumpInfo->size, lumpInfo->fileoff))
	{
		free(data);
		fprintf(stderr, "LoadLump: could not read lump data: %s\n", strerror(errno));
		return EINVAL;
	}

	memset(outLump, 0, sizeof(*outLump));
	memcpy(outLump->name, lumpInfo->name, sizeof(lumpInfo->name));
	outLump->data = data;
	outLump->size = lumpInfo->size;

	return 0;
}

int LoadLumpName(const wad_file* wad, const char* lumpName, lump_t* outLump)
{
	int error = 0;

	lump_info lumpInfo;
	error = CheckLumpName(wad, lumpName, &lumpInfo);
	if (error)
	{
		return error;
	}

	return LoadLump(wad, &lumpInfo, outLump);
}

int LoadLumpNum(const wad_file* wad, int lumpNum, lump_t* outLump)
{
	int error = 0;

	lump_info lumpInfo;
	error = CheckLumpNum(wad, lumpNum, &lumpInfo);
	if (error)
	{
		return error;
	}

	return LoadLump(wad, &lumpInfo, outLump);
}

void FreeLump(lump_t* lump)
{
	if (lump && lump->data)
	{
		free(lump->data);
	}
}

void CloseWadFile(wad_file* wad)
{
	if (wad)
	{
		close(wad->fd);
		free(wad->lumpTable);
	}
}

////////////////////////////////////////////////////////////////////////////

