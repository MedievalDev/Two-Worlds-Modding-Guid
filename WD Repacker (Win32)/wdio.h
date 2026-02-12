#ifndef WDIO_H
#define WDIO_H

//Made for Two Worlds 1 WD files, other files not currently supported.

#include <stdio.h>
#include <windows.h>

#define WDFLAG_COMPRESSED 0x1
#define WDFLAG_UNKNOWN2 0x2
#define WDFLAG_PLAINTEXT 0x4

#define WDFLAG_EXT_STR 0x8
#define WDFLAG_EXT_INT 0x10
#define WDFLAG_GUID 0x20

#define WDFLAG_DEFAULT 0x31
#define WDFLAG_ALLEXT 0x38

#define WDERR_OK 0

#define WDERR_FORMAT_ERROR        0x1
#define WDERR_FILE_ERROR          0x2
#define WDERR_COMPRESSION_ERROR   0x4
#define WDERR_MEMORY_ERROR        0x8

#define WDERR_HEADER_ERROR        0x10
#define WDERR_DIRECTORY_ERROR     0x20

#define WDERR_UNSUPPORTED_VERSION 0x100
#define WDERR_PARTIAL_SUCCESS     (-1)

typedef struct __WD_Header{
	char flags;
	char *ext_str; //optional: flags & WDFLAG_EXT_STR
	int ext_int; //optional: flags & WDFLAG_EXT_INT
	unsigned char guid[16]; //optional: flags & WDFLAG_GUID
} WD_Header;

typedef struct __WD_Directory_Entry{
	char *source_path; //to archive, null when reading
	char *path; //when archived
	unsigned int offset;
	unsigned int compressed_length;
	unsigned int uncompressed_length;
	WD_Header header;
} WD_Directory_Entry;

typedef struct __WD_File_Descriptor{
	FILE *handle;//set to null after closing or fd_Free will attempt to close it
	WD_Header head;
	FILETIME timestamp;
	WD_Directory_Entry *dir;
	int dirlen;//actually unsigned short?
} WD_File_Descriptor;

//all functions return 0 on success
//ProgReport can return 
typedef int(*_ProgReport)(char *path);

int wd_createDefaultWDHeader(WD_Header* header);
//opens existing wd file and reads the directory information.
int wd_openExisting(WD_File_Descriptor* out);
//pipes the file out, with the compressed header if relevant or uncompressed otherwise.
int wd_pipeFileOut(FILE* destination, WD_File_Descriptor* source, WD_Directory_Entry* file);
//does the full write operation, including reading in files and archiving them.
int wd_createNew(WD_File_Descriptor* fd, _ProgReport progressCallback);
//parseFileHeader checks if file has header and copies flags and extras from it if present.
int wd_parseFileHeader(FILE* source, WD_Header* header);

int wd_Free(WD_File_Descriptor* fd);

#endif /* WDIO_H */