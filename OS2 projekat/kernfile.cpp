#include "kernfile.h"
#include <cstdio>
#include <string.h>
#include "kernelfs.h"

KernelFile::KernelFile(char p, char m, char *n, char *e, ClusterNo firstCl, BytesCnt s, EntryNum en)
{
	strcpy(name, n);
	strcpy(ext, e);
	data = firstCl;
	size = s;
	mode = m;
	entryNum = en;
	part = p;

	opened = true;
}

char KernelFile::write(BytesCnt Bcnt, char* buffer)
{
	if (!opened || mode == 'r') return 0;
	return KernelFS::writeFile(this, Bcnt, buffer);
}

BytesCnt KernelFile::read(BytesCnt Bcnt, char* buffer)
{
	if(!opened || cursor == size) return 0;
	return KernelFS::readFile(this, Bcnt, buffer);
}

char KernelFile::seek(BytesCnt Bcnt)
{
	if(!opened || Bcnt > size || mode == 'a') return 0;
	cursor = Bcnt;
	return 1;
}

BytesCnt KernelFile::filePos()
{
	if(!opened) return 0;
	return cursor;
}

char KernelFile::eof()
{
	if(!opened) return 1;
	return (cursor == size)?2:0;
}

BytesCnt KernelFile::getFileSize()
{
	if(!opened) return 0;
	return size;
}

KernelFile::~KernelFile()
{
	opened = false;
}