#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

#include "types.h"

//Build with: gcc -o build_pfs0 build_pfs0.c

#define MAX_FS_ENTRIES 0x10//If ever needed, this constant and the size of stringtable can be increased.

typedef struct {
	u32 magicnum;//"PFS0"
	u32 file_count;
	u32 stringtable_size;
	u32 val_xc;//"Zero/Reserved"
} pfs0_header;

typedef struct {
	u64 offset;
	u64 size;
	u32 stringtable_offset;
	u32 val_x14;//"Normally zero?"
} pfs0_fsentry;

int build_pfs0(char *in_dirpath, char *out_pfs0_filepath)
{
	DIR *dir = NULL;
	struct dirent *cur_dirent = NULL;
	struct stat objstats;
	FILE *fout = NULL, *fin = NULL;
	int ret=0;
	u32 tmplen=0;
	u32 pos;
	u8 *tmpbuf;

	u32 objcount = 0;
	u32 stringtable_offset=0;
	u64 filedata_reloffset=0;

	pfs0_header header;
	pfs0_fsentry fsentries[MAX_FS_ENTRIES];
	pfs0_fsentry *fsentry;

	char objpath[256];

	char stringtable[0x100];

	memset(&header, 0, sizeof(header));
	memset(fsentries, 0, sizeof(fsentries));
	memset(stringtable, 0, sizeof(stringtable));

	dir = opendir(in_dirpath);
	if(dir==NULL)
	{
		printf("Failed to open dirpath.\n");
		return 1;
	}

	fout = fopen(out_pfs0_filepath, "wb");
	if(fout==NULL)
	{
		printf("Failed to open PFS0 filepath.\n");
		closedir(dir);
		return 1;
	}

	while((cur_dirent = readdir(dir)))
	{
		if(strcmp(cur_dirent->d_name, ".")==0 || strcmp(cur_dirent->d_name, "..")==0)continue;

		memset(objpath, 0, sizeof(objpath));
		snprintf(objpath, sizeof(objpath)-1, "%s%s", in_dirpath, cur_dirent->d_name);

		if(stat(objpath, &objstats)==-1)
		{
			printf("Failed to stat: %s\n", objpath);
			ret = 2;
			break;
		}

		if((objstats.st_mode & S_IFMT) == S_IFDIR)//directory
		{
			printf("Directories aren't supported, skipping... (%s)\n", objpath);
		}
		else if((objstats.st_mode & S_IFMT) == S_IFREG)//file
		{
			if(objcount>=MAX_FS_ENTRIES)
			{
				printf("Maximum fs object count already reached.\n");
				ret = 3;
				break;
			}

			fsentry = &fsentries[objcount];

			fsentry->offset = filedata_reloffset;
			fsentry->size = objstats.st_size;
			filedata_reloffset+= fsentry->size;
			fsentry->stringtable_offset = stringtable_offset;

			tmplen = strlen(cur_dirent->d_name)+1;
			if(stringtable_offset+tmplen > sizeof(stringtable))
			{
				printf("Max size of stringtable reached.\n");
				ret = 4;
				break;
			}

			strncpy(&stringtable[stringtable_offset], cur_dirent->d_name, sizeof(stringtable)-stringtable_offset);
			stringtable_offset+= tmplen;

			objcount++;
		}
		else
		{
			printf("Invalid FS object type.\n");
			ret = 14;
			break;
		}
	}

	closedir(dir);

	if(ret==0)
	{
		stringtable_offset = (stringtable_offset+0x1f) & ~0x1f;

		header.magicnum = le_word(0x30534650);
		header.file_count = le_word(objcount);
		header.stringtable_size = le_word(stringtable_offset);

		fwrite(&header, 1, sizeof(header), fout);
		fwrite(fsentries, 1, sizeof(pfs0_fsentry)*objcount, fout);
		fwrite(stringtable, 1, stringtable_offset, fout);

		stringtable_offset = 0;

		for(pos=0; pos<objcount; pos++)
		{
			tmplen = strlen(&stringtable[stringtable_offset]);
			if(tmplen==0)
			{
				printf("Empty string entry found in stringtable.\n");
				ret = 5;
				break;
			}
			tmplen++;

			if(stringtable_offset+tmplen > sizeof(stringtable))
			{
				printf("Max size of stringtable reached during stringtable entry reading.\n");
				ret = 4;
				break;
			}

			memset(objpath, 0, sizeof(objpath));
			snprintf(objpath, sizeof(objpath)-1, "%s%s", in_dirpath, &stringtable[stringtable_offset]);
			stringtable_offset+=tmplen;

			fin = fopen(objpath, "rb");
			if(fin==NULL)
			{
				printf("Failed to open filepath for filedata.\n");
				ret = 1;
				break;
			}

			tmpbuf = malloc(fsentries[pos].size);
			if(tmpbuf==NULL)
			{
				printf("Failed to allocate filedata.\n");
				ret = 6;
				fclose(fin);
				break;
			}

			tmplen = fread(tmpbuf, 1, fsentries[pos].size, fin);
			fclose(fin);

			fwrite(tmpbuf, 1, fsentries[pos].size, fout);
			free(tmpbuf);
		}
	}

	fclose(fout);

	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;
	u32 pos;

	char dirpath[256];

	if(argc < 3)
	{
		printf("%s\n", argv[0]);
		printf("Build a PFS0 from an input directory.\n");
		printf("Usage:\n");
		printf("%s <in directory> <out PFS0 filepath>\n", argv[0]);
		return 0;
	}

	pos = strlen(argv[1]);

	if(pos==0 || pos>sizeof(dirpath)-1)
	{
		printf("Dirpath is length is invalid.\n");
		return 1;
	}
	pos--;

	memset(dirpath, 0, sizeof(dirpath));
	strncpy(dirpath, argv[1], sizeof(dirpath)-1);
	if(dirpath[pos] != '/')dirpath[pos+1] = '/';

	ret = build_pfs0(dirpath, argv[2]);

	if(ret)printf("Failed to build PFS0.\n");
	return 0;
}

