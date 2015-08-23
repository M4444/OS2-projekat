#include "kernfile.h"
#include <cstdio>
#include <string.h>

KernelFile::KernelFile(char m, char *n, char *e, ClusterNo firstCl, BytesCnt s)
{
	strcpy(name, n);
	strcpy(ext, e);
	data = firstCl;
	size = s;
	mode = m;

	opened = true;
}

KernelFile::KernelFile(BytesCnt byteSize)
{
	size = byteSize;
	//cursor = 0;
	opened = true;
}

char KernelFile::write(BytesCnt Bcnt, char* buffer)
{
	// TODO
	return 0;
}

BytesCnt KernelFile::read(BytesCnt Bcnt, char* buffer)
{

	// TODO
	return 0;
}

char KernelFile::seek(BytesCnt Bcnt)
{
	if(Bcnt > size) return 0;
	cursor = Bcnt;
	return 1;
}

BytesCnt KernelFile::filePos()
{
	return cursor;
}

char KernelFile::eof()
{
	return (cursor == size)?2:0;
}

BytesCnt KernelFile::getFileSize()
{
	return size;
}

///char KernelFile::truncate(); 	//** opciono

KernelFile::~KernelFile()
{
	opened = false;
	// TODO
}