// File: kernfile.h
#pragma once

#include "fs.h"
#include "part.h"
#include <stdio.h>

class KernelFile{
private:
	///FILE *f;
	bool opened;
	BytesCnt cursor;
	char mode;

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
	//friend class File;
	KernelFile(char m, char *name, char *ext, ClusterNo firstCl, BytesCnt size);
	KernelFile(BytesCnt byteSize); 			//objekat fajla se moze kreirati samo otvaranjem
	///KernelFile *myImpl;
};