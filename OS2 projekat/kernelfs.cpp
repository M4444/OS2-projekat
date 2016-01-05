#include "kernelfs.h"
#include "part.h"
#include "file.h"
#include "kernfile.h"
#include <cstring>
#include <stdio.h>
#include <cmath>

Partition* KernelFS::partitions[26];
char* KernelFS::FATcache;
char KernelFS::clCache[ClusterSize];
ClusterNo KernelFS::currClNum;
bool KernelFS::clCached;

CRITICAL_SECTION KernelFS::critical;

KernelFS::KernelFS()
{
	InitializeCriticalSection(&critical);
	for(int i = 0; i<26; i++) KernelFS::partitions[i] = nullptr;
	KernelFS::clCached = false;
}

KernelFS::~KernelFS()
{
	for(int i = 0; i<26; i++) delete KernelFS::partitions[i];
	delete[] KernelFS::FATcache;
	DeleteCriticalSection(&critical);
}

void KernelFS::Bto4B(unsigned char* z, const unsigned char c)
{
	for(int i = 0; i < 4; ++i) z[i] = c;
}

void KernelFS::ulongTo4B(unsigned char* z, const unsigned long l)
{
	for(int i=0; i<4; i++) z[i] = char ((l >> i*8) & 0xff);
}

unsigned long KernelFS::B4toUlong(const unsigned char* z)
{
	unsigned long p, l;
	l = 0;
	for(int i = 0; i < 4; ++i)
	{
		p = z[i];
		l |= p & 0xff << i * 8;
	}
	return l;
}

void KernelFS::clear4B(unsigned char* z)
{
	Bto4B(z, 0);
}

bool KernelFS::partIsMounted(char p)
{
	if(KernelFS::partitions['A' - p] != nullptr) return true;
	else return false;
}

void KernelFS::writeFAT(ClusterNo num, const char *buffer)
{
	if(!KernelFS::FATcache) return;

	memcpy(KernelFS::FATcache + num*ClusterSize, buffer, ClusterSize);
}

int KernelFS::getFileName(char *name, const char* fname)
{
	int cnt, s = 0;
	for(cnt = 0; cnt<8; cnt++)
	{
		if(fname[cnt + 3] == '.') break;
		name[cnt] = fname[cnt + 3];
	}
	for(int i = cnt; i < 8; i++)
	{
		name[i] = ' ';
		s++;
	}
	name[8] = '\0';
	return 8-s;
}

void KernelFS::getFileExt(char *ext, const char* fname)
{
	ext[3] = '\0';
	ext[2] = fname[strlen(fname) - 1];
	ext[1] = fname[strlen(fname) - 2];
	ext[0] = fname[strlen(fname) - 3];
}

ClusterNo KernelFS::getClNumFromBsize(unsigned int sizeB)
{
	if(sizeB == 0) return 0;
	ClusterNo clNum = sizeB / ClusterSize - (sizeB % ClusterSize) == 0 ? 1 : 0;
	return clNum;
}

int KernelFS::getMountNum(char part)
{
	int num = 'A' - part;
	if(num < 0 || num > 26) return -1;
	if(!partIsMounted(part)) return -2;
}

char KernelFS::mount(Partition* partition)
{
	int i=0;
	while((KernelFS::partitions[i] != nullptr) || i == 26)
	{
		i++;
	}
	if(i == 26) return '0';
	KernelFS::partitions[i] = partition;
	return 'A'+i;
}

char KernelFS::unmount(char part)
{
	int ind = getMountNum(part);
	if(ind < 0) return '0';
	
	if(KernelFS::partitions[ind] != nullptr)
	{
		KernelFS::partitions[ind] = nullptr;
		return '1';		// uspeh
	}
	else return '0';	// neuspeh
}

char KernelFS::format(char part)
{
	int ind = getMountNum(part);
	if(ind < 0) return 0;
	
	int r;
	char buffer[ClusterSize] = {0};
	
	unsigned long size = KernelFS::partitions[ind]->getNumOfClusters();
	unsigned long FATsize = size*FATBSIZE;	// size in bytes
	
	if(size <= 0) return 0;
	
	unsigned char z[4];
	unsigned long j;
	ClusterNo k = 0;
	ClusterNo rootClNum = getClNumFromBsize(FATsize + 16) + 1;

	KernelFS::FATcache = new char[ClusterSize*(rootClNum)];
	// prepare buffer
	ulongTo4B(z, rootClNum + 1);	// broj ulaza pocetka liste slobodnih klastera
	memcpy(buffer, z, 4);
	ulongTo4B(z, size);	// broj ulaza u FAT tabeli
	memcpy(buffer + 4, z, 4);
	ulongTo4B(z, rootClNum);	// broj ulaza korenog direktorijuma
	memcpy(buffer + 8, z, 4);
	Bto4B(z, 0);	// broj ulaza u korenom direktorijumu
	memcpy(buffer + 12, z, 4);

	if(FATsize+16 < ClusterSize)	// FAT staje u nulti klaster
	{
		// FAT
			// preskace nulti ulaz
		Bto4B(z, 0xff);	// ulaz korenog direktorijuma
		memcpy(buffer+20, z, 4);
		FATsize -= 8;	// jer su predjena 2 ulaza
		j = 3;
	}
	else	// FAT ne staje u nulti klaster
	{		
		// FAT
		bool checkRoot = true;
		j = 1 + rootClNum;
		if(rootClNum <= (ClusterSize-16)/4)
		{
			// preskacu se FAT klasteri
			checkRoot = false;
			Bto4B(z, 0xff);	// ulaz korenog direktorijuma
			memcpy(buffer+rootClNum*4+4, z, 4);
			FATsize -= rootClNum*4;
			for(unsigned long i=rootClNum*4+8; i<ClusterSize; i+=4)	// ulancavanje slobodnih klastera
			{
				ulongTo4B(z, j);
				memcpy(buffer+4*(j+3), z, 4);
				j++;
			}
		}
		// write buffer
		r = KernelFS::partitions[ind]->writeCluster(0, buffer);
		writeFAT(0, buffer);
		if(r != 1) return 0;
		FATsize -= ClusterSize-16;
		k = 1;
		while(FATsize > ClusterSize)
		{
			// prepare buffer
			// continue FAT
			for(unsigned long i=0; i<ClusterSize; i+=4)	// ulancavanje slobodnih klastera
			{
				if(checkRoot && j-1 == rootClNum) // provera da li je ulaz korenog direktorijuma
				{
					checkRoot = false;
					Bto4B(z, 0xff);	// ulaz korenog direktorijuma
				}
				else ulongTo4B(z, j);
				memcpy(buffer+4*(j+3), z, 4);
				j++;
			}
			r = KernelFS::partitions[ind]->writeCluster(k, buffer);
			writeFAT(k, buffer);
			if(r != 1) return 0;
			FATsize -= ClusterSize;
			k++;
		}
		// last buffer
	}
	while(FATsize > 4)	// ulancavanje slobodnih klastera
	{
		ulongTo4B(z, j);
		memcpy(buffer + 4 * (j + 3), z, 4);
		FATsize -= 4;
		j++;
	}
	Bto4B(z, 0xff);	// kraj liste
	memcpy(buffer + 4 * (j + 3), z, 4);

	r = KernelFS::partitions[ind]->writeCluster(k, buffer);
	writeFAT(k, buffer);
	if(r != 1) return 0;
	k++;
	// koreni dijektorijum
	z[0] = part;
	z[1] = ':';
	z[2] = '\\';
	z[3] = ' ';
	memcpy(buffer, z, 4);
	Bto4B(z, ' ');
	memcpy(buffer + 4, z, 4);
	z[0] = 'd';
	z[1] = 'i';
	z[2] = 'r';
	z[3] = 0;
	memcpy(buffer + 8, z, 4);
	clear4B(z);
	memcpy(buffer + 12, z, 4);
	memcpy(buffer + 16, z, 4);

	r = KernelFS::partitions[ind]->writeCluster(k, buffer);
	if(r != 1) return 0;
	return 1;
}

char KernelFS::doesExist(char* fname)
{
	unsigned char z[4];
	char zz[20];
	ClusterNo clNum = 0, entryNum = 0;
	int r;
	char name[9], ext[4];

	char pathBuff[30];
	strcpy(pathBuff, fname);

	// provera particije i formata
	int ind = getMountNum(pathBuff[0]);
	if(ind < 0) return 0;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return 0;

	getFileName(name, pathBuff);
	getFileExt(ext, pathBuff);
	if(strlen(pathBuff) - 3 - strlen(name) - 1 - strlen(ext) != 0) return 0;

	memcpy(z, KernelFS::FATcache + 8, 4);
	clNum = B4toUlong(z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return 0;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	entryNum = B4toUlong(z);

	for(unsigned int i = 1; i < entryNum+1; i++)	//search root directory
	{
		if(i * 20>ClusterSize)	// cache next cluster
		{
			clNum = next(clNum);
			r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
			if(r != 1) return 0;
			currClNum = clNum;
			KernelFS::clCached = true;
		}
		memcpy(zz, KernelFS::clCache + i * 20, 20);

		bool rightE = true;
		for(int j = 0; j < 8; j++) rightE &= (zz[j] == name[j]) & (zz[8+j] == ext[j]);
		if(rightE) return 1;	// postoji
	}
	return 0;	// ne postoji
}

File* KernelFS::open(char* fname, char mode)
{
	char pathBuff[30];
	strcpy(pathBuff, fname);

	// provera moda
	if(mode != 'r' && mode != 'w' && mode != 'a') return NULL;
	// provera particije i formata
	int ind = getMountNum(pathBuff[0]);
	if(ind < 0) return NULL;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return NULL;

	// fajl
	char name[9], ext[4];
	getFileExt(ext, pathBuff);
	int fnsize = getFileName(name, pathBuff);
	if(strlen(pathBuff) - 3 - fnsize - 1 - strlen(ext) != 0) return NULL;
	int strl;
	strl = strlen(pathBuff);
	strl = strlen(name);
	strl = strlen(ext);

	unsigned char z[4];
	char zz[20];
	ClusterNo oldClNum, clNum = 0, entryNum = 0;
	int r;

	memcpy(z, KernelFS::FATcache + 8, 4);
	clNum = B4toUlong(z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return NULL;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	entryNum = B4toUlong(z);

	unsigned int i, n, br = 0;
	bool compF = false, swapped = false;
	char clCache2[ClusterSize];
	for(i = 1; i < entryNum; i++)	//search root directory
	{
		n = i % (ClusterSize / 20);
		if(i / (ClusterSize / 20) > br)	// cache next cluster
		{
			br++;
			oldClNum = clNum;
			clNum = next(clNum);
			if(compF)
			{
				r = KernelFS::partitions[ind]->readCluster(clNum, clCache2);
				if(r != 1) return 0;

				memcpy(KernelFS::clCache + (n - 1) * 20, clCache2, 20);

				r = KernelFS::partitions[ind]->writeCluster(oldClNum, KernelFS::clCache);
				if(r != 1) return 0;

				memcpy(KernelFS::clCache, clCache2, ClusterSize);
				currClNum = clNum;

				swapped = true;
			}
			else
			{
				swapped = false;

				r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
				if(r != 1) return 0;
				currClNum = clNum;
				KernelFS::clCached = true;
			}
		}
		if(!compF)
		{
			memcpy(zz, KernelFS::clCache + n * 20, 20);

			bool rightE = true;
			for(int j = 0; j < 8; j++) rightE &= (zz[j] == name[j]) & (zz[8+j] == ext[j]);
			if(rightE)
			{
				compF = true;
				if(mode != 'w') break;
			}
		}
		else if(!swapped) // kompakcija root dir (automatski se izbacujse ulaz)
		{
			memcpy(KernelFS::clCache + (n - 1) * 20, KernelFS::clCache + n * 20, 20);
		}
	}
	if(mode == 'w')
	{
		if(compF)
		{
			if(deleteFile(zz, ind) != 1) return NULL;	// fajl postoji
		}
	}
	else if(!compF) return NULL;	// ne postoji fajl

	ClusterNo firstCl = 0, size = 0;
	if(mode == 'w')
	{
		// create root dir entry
		memcpy(z, KernelFS::FATcache + 8, 4);
		clNum = B4toUlong(z);
		
		firstCl = clNum + getClNumFromBsize((entryNum + 1) * 20);

		if(!KernelFS::clCached || currClNum != firstCl)
		{
			r = KernelFS::partitions[ind]->readCluster(firstCl, KernelFS::clCache);
			if(r != 1) return NULL;
			currClNum = firstCl;
			KernelFS::clCached = true;
		}
		char zz[20];
		memcpy(zz, name, 8);
		memcpy(zz + 8, ext, 3);
		clear4B(z);
		memcpy(zz + 0xc, z, 4);
		memcpy(zz + 0x10, z, 4);

		memcpy(KernelFS::clCache + ((entryNum+1) % (ClusterSize / 20))*20, zz, 20);
		r = KernelFS::partitions[ind]->writeCluster(firstCl, KernelFS::clCache);
		if(r != 1) return 0;
			
		firstCl = 0;
		size = 0;
	}
	else
	{
		memcpy(z, zz + 0xc, 4);
		firstCl = B4toUlong(z);

		memcpy(z, zz + 0x10, 4);
		size = B4toUlong(z);
	}

	KernelFile *kfile = new KernelFile(ind+'A', mode, name, ext, firstCl, size, entryNum);
	File *file = new File();
	file->myImpl = kfile;
	r = (mode == 'a' ? file->seek(size - 1) : file->seek(0));
	if(r != 1) return NULL;
	return file;
}

ClusterNo KernelFS::next(ClusterNo c)
{
	unsigned char z[4];
	ClusterNo nextCl = 0;

	memcpy(z, KernelFS::FATcache + 16 + c*FATBSIZE, 4);
	nextCl = B4toUlong(z);
	if(nextCl == 0xff) return 0;
	return nextCl;
}

void KernelFS::writeNext(ClusterNo dest, ClusterNo sour)
{
	unsigned char z[4];

	ulongTo4B(z, sour);
	memcpy(KernelFS::FATcache + 16 + dest*FATBSIZE, z, 4);
}

char KernelFS::deleteFile(char *e, int part)
{
	unsigned char z[4];
	ClusterNo fileClNum = 0, freeClNum = 0;

	memcpy(z, e + 0xc, 4);
	fileClNum = B4toUlong(z);

	memcpy(z, KernelFS::FATcache, 4);
	freeClNum = B4toUlong(z);

	ClusterNo curr = freeClNum < fileClNum ? freeClNum : fileClNum;
	ClusterNo notIterated = freeClNum > fileClNum ? freeClNum : fileClNum;
	ClusterNo iterated = curr;
	ClusterNo p, lastClNum;

	if(next(iterated) != 0)
	{
		while(true)
		{
			if(next(curr) < notIterated)
			{
				curr = next(curr);
				iterated = next(iterated);
				if(next(iterated) == 0)
				{
					writeNext(iterated, notIterated);
					lastClNum = iterated;
					break;
				}
			}
			else
			{
				writeNext(curr, notIterated);
				curr = notIterated;
				p = notIterated;
				notIterated = iterated;
				iterated = p;
				iterated = next(iterated);
			}
		}
	}
	else
	{
		ulongTo4B(z, iterated);
		memcpy(KernelFS::FATcache, z, 4);
		writeNext(iterated, freeClNum);
		lastClNum = iterated;
	}

	ClusterNo lastCl = getClNumFromBsize((lastClNum + 4) * 4);
	int r;
	for(unsigned int i = 0; i < lastCl; i++)
	{
		r = KernelFS::partitions[part]->writeCluster(i, KernelFS::FATcache + i*ClusterSize);
		if(r != 1) return 0;
	}
	return 1;
}

char KernelFS::deleteFile(char* fname)
{
	char pathBuff[30];
	strcpy(pathBuff, fname);

	// provera particije i formata
	int ind = getMountNum(pathBuff[0]);
	if(ind < 0) return 0;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return 0;

	// fajl
	char name[9], ext[4];
	getFileName(name, fname);
	getFileExt(ext, fname);
	if(strlen(fname) - 3 - strlen(name) - 1 - strlen(ext) != 0) return 0;

	unsigned char z[4];
	char zz[20];
	ClusterNo oldClNum, clNum = 0, entryNum = 0;
	int r;

	memcpy(z, KernelFS::FATcache + 8, 4);
	clNum = B4toUlong(z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return 0;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	entryNum = B4toUlong(z);

	unsigned int i, n, br = 0;
	bool compF = false, swapped = false;
	char clCache2[ClusterSize];
	for(i = 1; i < entryNum; i++)	//search root directory
	{
		n = i % (ClusterSize / 20);
		if(i / (ClusterSize / 20) > br)	// cache next cluster
		{
			br++;
			oldClNum = clNum;
			clNum = next(clNum);
			if(compF)
			{
				r = KernelFS::partitions[ind]->readCluster(clNum, clCache2);
				if(r != 1) return 0;

				memcpy(KernelFS::clCache + (n - 1) * 20, clCache2, 20);

				r = KernelFS::partitions[ind]->writeCluster(oldClNum, KernelFS::clCache);
				if(r != 1) return 0;

				memcpy(KernelFS::clCache, clCache2, ClusterSize);
				currClNum = clNum;

				swapped = true;
			}
			else
			{
				swapped = false;

				r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
				if(r != 1) return 0;
				currClNum = clNum;
				KernelFS::clCached = true;
			}
		}
		if(!compF)
		{
			memcpy(zz, KernelFS::clCache + n * 20, 20);

			bool rightE = true;
			for(int j = 0; j < 8; j++) rightE &= (zz[j] == name[j]) & (zz[8+j] == ext[j]);
			if(rightE) compF = true;
		}
		else if(!swapped) // kompakcija root dir (automatski se izbacujse ulaz)
		{
			memcpy(KernelFS::clCache + (n - 1) * 20, KernelFS::clCache + n * 20, 20);
		}
	}
	if(!compF) return 0;	// ne postoji fajl
	else
	{
		r = deleteFile(zz, ind);
		return r;
	}
}

char KernelFS::readDir(char* dirname, EntryNum n, Entry &e)
{
	unsigned char z[4];
	ClusterNo clNum = 0, entryNum = 0;
	int r;

	char pathBuff[30];
	strcpy(pathBuff, dirname);

	// provera particije i formata
	int ind = getMountNum(pathBuff[0]);
	if(ind < 0) return 0;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return 0;

	memcpy(z, KernelFS::FATcache + 8, 4);
	clNum = B4toUlong(z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return 0;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	entryNum = B4toUlong(z);

	if(entryNum < n) return 2;

	ClusterNo lastCl = getClNumFromBsize((n + 1) * 20);
	if(!KernelFS::clCached || currClNum != lastCl)
	{
		r = KernelFS::partitions[ind]->readCluster(lastCl, KernelFS::clCache);
		if(r != 1) return 0;
		currClNum = lastCl;
		KernelFS::clCached = true;
	}

	n = (n + 1) % (ClusterSize / 20);
	memcpy(&e, KernelFS::clCache + n * 20, 20);
	return 1;
}

char KernelFS::writeFile(KernelFile *f, BytesCnt Bcnt, char* buffer)
{
	BytesCnt currFileSize = f->getFileSize();
	ClusterNo currClSize = getClNumFromBsize(currFileSize);
	ClusterNo newClSize = getClNumFromBsize(Bcnt) + 1;

	int r;
	unsigned char z[4];

	ClusterNo currFile = 0, firstFile;

	if(currClSize < newClSize)
	{
		// allocate clusters for file		
		if(f->data != 0)
		{
			memcpy(z, KernelFS::FATcache + 16 + f->data*FATBSIZE, 4);	// first file cluster
			firstFile = currFile = B4toUlong(z);
			while(next(currFile) != 0)
			{
				writeNext(currFile, (next(currFile) == 0) ? 0xffffffff : next(currFile));
			}
		}

		ClusterNo currFree = 0, nextFree = 0;

		memcpy(z, KernelFS::FATcache, 4);	// first free cluster
		currFree = B4toUlong(z);

		if(currFile == 0) firstFile = currFree;
		for(unsigned int i = 0; i < newClSize - currClSize; i++)
		{
			if(currFile != 0) writeNext(currFile, currFree);
			nextFree = next(currFree);
			writeNext(currFree, 0xffffffff);
			currFile = currFree;
			currFree = nextFree;
		}

		ulongTo4B(z, currFree);
		memcpy(KernelFS::FATcache, z, 4);	// new first free cluster

		// change size in root dir entry
		memcpy(z, KernelFS::FATcache + 8, 4);
		ClusterNo clNum = B4toUlong(z);

		ClusterNo firstCl = clNum + getClNumFromBsize((f->entryNum + 1) * 20);

		if(!KernelFS::clCached || currClNum != firstCl)
		{
			r = KernelFS::partitions[f->part - 'A']->readCluster(firstCl, KernelFS::clCache);
			if(r != 1) return NULL;
			currClNum = firstCl;
			KernelFS::clCached = true;
		}
		char zz[8];
		ulongTo4B(z, firstFile);
		memcpy(zz, z, 4);
		ulongTo4B(z, Bcnt);
		memcpy(zz + 4, z, 4);

		if(currFileSize != 0)
		{
			memcpy(KernelFS::clCache + ((f->entryNum + 1) % (ClusterSize / 20)) * 20 + 0x10, z, 4);
			f->size = Bcnt;
		}
		else
		{
			memcpy(KernelFS::clCache + ((f->entryNum + 1) % (ClusterSize / 20)) * 20 + 0xc, zz, 8);
			f->size = Bcnt;
			f->data = firstFile;
		}
		r = KernelFS::partitions[f->part - 'A']->writeCluster(firstCl, KernelFS::clCache);
		if(r != 1) return 0;		
	}
	// write to file
	ClusterNo oldClNum, clNum = f->data + getClNumFromBsize(f->filePos());
	unsigned int n, i, br = 0;
	bool firstPass = true;

	for(i = 0; i < Bcnt; i++)
	{
		n = f->filePos() % ClusterSize;
		if(f->filePos() / ClusterSize > br || firstPass)	// cache next cluster
		{
			if(!firstPass)
			{
				r = KernelFS::partitions[f->part - 'A']->writeCluster(oldClNum, KernelFS::clCache);
				if(r != 1) return 0;
			}
			if(!KernelFS::clCached || currClNum != clNum)
			{
				r = KernelFS::partitions[f->part - 'A']->readCluster(clNum, KernelFS::clCache);
				if(r != 1) return 0;
				currClNum = clNum;
				KernelFS::clCached = true;
			}
			oldClNum = clNum;
			clNum = next(clNum);
			firstPass = false;
			br++;
		}

		memcpy(KernelFS::clCache + n, buffer + i, 1);

		f->seek(f->filePos() + 1);
		if(f->eof()) return 0;
	}
	return 1;
}

BytesCnt KernelFS::readFile(KernelFile *f, BytesCnt Bcnt, char* buffer)
{
	ClusterNo clNum = f->data + getClNumFromBsize(f->filePos());
	unsigned int r, n, i, br = 0;
	bool firstPass = true;

	for(i = 0; i < Bcnt; i++)
	{
		n = f->filePos() % ClusterSize;
		if(f->filePos() / ClusterSize > br || firstPass)	// cache next cluster
		{
			if(!KernelFS::clCached || currClNum != clNum)
			{
				r = KernelFS::partitions[f->part - 'A']->readCluster(clNum, KernelFS::clCache);
				if(r != 1) return NULL;
				currClNum = clNum;
				KernelFS::clCached = true;
			}
			clNum = next(clNum);
			firstPass = false;
			br++;
		}
	
		memcpy(buffer + i, KernelFS::clCache + n, 1);
		f->seek(f->filePos()+1);
		if(f->eof()) break;
	}
	return i;
}