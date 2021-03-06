/*
	written by Missingno_force a.k.a. Missingmew
	Copyright (c) 2014-2018
	see LICENSE for details
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define FILENAMELENGTH 16
#define DIRNAMELENGTH 8
#define BLOCKSIZE 0x800

#ifdef _WIN32
#include <direct.h>
#define createDirectory(dirname) mkdir(dirname)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define createDirectory(dirname) mkdir(dirname, 0777)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define FILENAMELENGTH 16
#define DIRNAMELENGTH 8
#define BLOCKSIZE 0x800

#ifdef _WIN32
#include <direct.h>
#define createDirectory(dirname) mkdir(dirname)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define createDirectory(dirname) mkdir(dirname, 0777)
#endif

int writeFile( FILE *input, int length, FILE *output ) {
	unsigned char dataBuffer[1024];
	unsigned int bytesLeft = length;
	
	while(bytesLeft) {
		unsigned int wantedRead;
		if(bytesLeft >= sizeof(dataBuffer))
			wantedRead = sizeof(dataBuffer);
		else
			wantedRead = bytesLeft;
		unsigned int haveRead = fread(dataBuffer, 1, wantedRead, input);
		if(haveRead != wantedRead) {
			printf("haveRead != wantedRead: %d != %d\n", haveRead, wantedRead);
			perror("This broke");
			return 0;
		}

		unsigned int haveWrite = fwrite(dataBuffer, 1, haveRead, output);
		if(haveWrite != haveRead) {
			printf("haveWrite != haveRead: %d != %d\n", haveWrite, haveRead);
			return 0;
		}
		
		bytesLeft -= haveRead;
	}
	return 1;
}

typedef struct {
	char name[DIRNAMELENGTH];
	uint32_t subDirCount;
	uint32_t subDirOffset;
	uint32_t subFileCount;
	uint32_t subFileOffset;
	uint32_t unknown2;
	uint32_t unknown3;
}__attribute__((packed)) tocDirectoryEntry;

typedef struct {
	char name[FILENAMELENGTH];
	uint32_t fileOffset;
	uint32_t fileSize;
	uint32_t unknown1;
	uint32_t unknown2;
}__attribute__((packed)) tocFileEntry;


char directoryState[4][16];
char completeName[48] = { 0 };
int dirlevel = 0;
char rootchar[4] = "ZOE";

void processDirectoryListing( FILE *f, tocDirectoryEntry entry, char *curpath, unsigned int tgsfix ) {
	tocFileEntry file;
	unsigned int i;
	FILE *o;
	char *filepath = NULL;
	
	filepath = malloc(strlen(curpath)+1+strlen(entry.name)+1+16+1); // max filename length is 16
	sprintf(filepath, "%s/%s", curpath, entry.name);
	
	printf("processDirectoryListing - Current directory: %s at %08lx\n", filepath, ftell(f) );
	createDirectory( filepath );
	for( i = 0; i < entry.subFileCount; i++ ) {
		fseek( f, (entry.subFileOffset*BLOCKSIZE)+(32*i), SEEK_SET );
		fread( &file, sizeof(file), 1, f );
		if(!file.name[0]) {
			printf("skipping invalid toc entry %d of %d\n", i+1, entry.subFileCount);
			continue;
		}
		if( tgsfix ) file.fileOffset -= 0x18;
		sprintf(filepath, "%s/%s/%s", curpath, entry.name, file.name);
		printf("Current file: %s at %08x\n", filepath, file.fileOffset*BLOCKSIZE );
		if( !(o = fopen( filepath, "wb" ))) {
			printf("Couldnt open file %s\n", filepath);
			return;
		}
		fseek( f, file.fileOffset*BLOCKSIZE, SEEK_SET );
		writeFile( f, file.fileSize, o );
		fclose(o);
	}
	free(filepath);
	return;
}
	
void processDirectory( FILE *f, tocDirectoryEntry entry, char *curpath, unsigned int tgsfix ) {
	memset( completeName, 0, 48 );
	tocDirectoryEntry workentry;
	unsigned int i;
	char *newpath = NULL;
	
	if(!curpath) {
		newpath = malloc(sizeof("ZOE")+1);
		sprintf(newpath, "ZOE");
	}
	else {
		newpath = malloc(strlen(curpath)+1+strlen(entry.name)+1);
		sprintf(newpath, "%s/%s", curpath, entry.name);
	}
	
	printf("processDirectory - Current directory: %s at %08lx\n", newpath, ftell(f) );
	createDirectory( newpath );
	
	for( i = 0; i < entry.subDirCount; i++ ) {
		fseek( f, entry.subDirOffset+(i*32), SEEK_SET );
		fread( &workentry, sizeof(workentry), 1, f );
		if( tgsfix && workentry.subFileOffset ) workentry.subFileOffset -= 0x18;
		if( workentry.subDirCount ) processDirectory( f, workentry, newpath, tgsfix );
		else processDirectoryListing( f, workentry, newpath, tgsfix );
	}
	
	free(newpath);
	return;
}

int main( int argc, char **argv ) {
	
	tocDirectoryEntry workdir;
	unsigned int tgsfix = 0;
	FILE *f;
	
	memset( workdir.name, 0, DIRNAMELENGTH );
	
	if ( argc < 2 ) {
		printf("Not enough args!\nUse: %s DAT-file\n", argv[0]);
		return 1;
	}
	if( argc == 3 ) tgsfix = 1;
	
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	fseek( f, 0, SEEK_SET );
	
	fread( &workdir, sizeof(workdir), 1, f );
	
	processDirectory( f, workdir, NULL, tgsfix );
	
	printf("Done.\n");
	
	fclose(f);
	return 0;
}
