// File: kernfile.h
#pragma once

#include "fs.h"
#include "part.h"
#include <stdio.h>

class KernelFile{
private:
	bool opened;
	BytesCnt cursor;
	char mode;
	char part;
	EntryNum entryNum;

	char name[FNAMELEN + 1];
	char ext[FEXTLEN + 1];
	ClusterNo data;
	BytesCnt size;

public:
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate(); 	//** opciono
	~KernelFile();		//zatvaranje fajla

private:
	friend class FS;
	friend class KernelFS;
	KernelFile(char p, char m, char *n, char *e, ClusterNo firstCl, BytesCnt s, EntryNum en);
};