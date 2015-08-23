#include "fs.h"
#include "kernelfs.h"

KernelFS *FS::myImpl = new KernelFS();

FS::~FS()
{
	delete myImpl;
}

char FS::mount(Partition* partition)
{
	return myImpl->mount(partition);
}

char FS::unmount(char part)
{
	return myImpl->unmount(part);
}

char FS::format(char part)
{
	return myImpl->format(part);
}

char FS::doesExist(char* fname)
{
	return myImpl->doesExist(fname);
}

File* FS::open(char* fname, char mode)
{
	return myImpl->open(fname, mode);
}

char FS::deleteFile(char* fname)
{
	return myImpl->deleteFile(fname);
}

//char FS::createDir(char* dirname);

//char FS::deleteDir(char* dirname);

char FS::readDir(char* dirname, EntryNum n, Entry &e)
{
	return myImpl->deleteFile(dirname, n, e);
}


//FS ();
