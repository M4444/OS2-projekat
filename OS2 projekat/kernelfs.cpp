#include "kernelfs.h"
#include "part.h"
#include "file.h"
#include "kernfile.h"
#include <cstring>
#include <stdio.h>
#include <cmath>

//KernelFS::partitions = nullptr;
Partition* KernelFS::partitions[26];
char* KernelFS::FATcache;
char KernelFS::clCache[ClusterSize];
ClusterNo KernelFS::currClNum;
bool KernelFS::clCached;

KernelFS::KernelFS()
{
	for(int i = 0; i<26; i++) KernelFS::partitions[i] = nullptr;
	//currClNum = -1
	KernelFS::clCached = false;
}

KernelFS::~KernelFS()
{
	for(int i = 0; i<26; i++) delete KernelFS::partitions[i];
	///delete[] KernelFS::partitions;
	delete[] KernelFS::FATcache;
}

void KernelFS::Bto4B(unsigned char* z, const unsigned char c)
{
	for(int i = 0; i < 4; ++i) z[i] = c;
}

void KernelFS::ulongTo4B(unsigned char* z, const unsigned long l)
{
	for(int i=0; i<4; i++) z[i] = char ((l >> i*8) & 0xff);
}

void KernelFS::B4toUlong(unsigned long l, const unsigned char* z)
{
	unsigned long p;
	l = 0;
	for(int i = 0; i < 4; ++i)
	{
		p = z[i];
		l |= p & 0xff << i * 8;
	}
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
	//if(num > size || num < 0) return 0;

	memcpy(KernelFS::FATcache + num*ClusterSize, buffer, ClusterSize);
	///fwrite(workingCl, 1, ClusterSize, f);
}

void KernelFS::getFileName(char *name, const char* fname)
{
	int cnt;
	for(cnt = 0; cnt<8; cnt++)
	{
		if(fname[cnt + 3] == '.') break;
		name[cnt] = fname[cnt + 3];
	}
	for(int i = cnt; i<8; i++) name[i] = ' ';
	name[9] = '\0';
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
	ClusterNo clNum = sizeB / ClusterSize - (sizeB % ClusterSize) == 0 ? 1 : 0;
	return clNum;
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
	int ind = 'A'-part;
	if(ind > 26 || ind < 0) return '0';
	
	if(KernelFS::partitions[ind] != nullptr)
	{
		KernelFS::partitions[ind] = nullptr;
		return '1';		// uspeh
	}
	else return '0';	// neuspeh
	
	///char r = (KernelFS::partitions[ind] == NULL) ? '0' : '1';
	///KernelFS::partitions[ind] = NULL;
	///return r;
}

char KernelFS::format(char part)
{
	int ind = 'A'-part;
	if(ind > 26 || ind < 0) return 0;
	if(!partIsMounted(part)) return 0;
	
	///Cluster *c = new Cluster();
	int r;
	char buffer[ClusterSize] = {0};
	
	unsigned long size = KernelFS::partitions[ind]->getNumOfClusters();
	unsigned long FATsize = size*FATBSIZE;	// size in bytes
	
	if(size <= 0) return 0;
	
	unsigned char z[4];
	unsigned long j;
	///unsigned long rootClNum = (FATsize + 16) / ClusterSize +
	///						  ((FATsize + 16) % ClusterSize) == 0 ? 0 : 1;
	ClusterNo k = 0;
	ClusterNo rootClNum = getClNumFromBsize(FATsize + 16) + 1;

	KernelFS::FATcache = new char[ClusterSize*(rootClNum - 1)];
	// prepare buffer
	ulongTo4B(z, rootClNum + 1);	// broj ulaza pocetka liste slobodnih klastera
	memcpy(buffer, z, 4);
	ulongTo4B(z, size);	// broj ulaza u FAT tabeli
	memcpy(buffer + 4, z, 4);
	ulongTo4B(z, rootClNum);	// broj ulaza korenog direktorijuma
	memcpy(buffer + 8, z, 4);
	Bto4B(z, 0);	// broj ulaza u korenom direktorijumu
	// 0? Treba da se menja kad se napravi novi fajl
	memcpy(buffer + 12, z, 4);

	if(FATsize+16 < ClusterSize)	// FAT staje u nulti klaster
	{
		// FAT
			// preskace nulti ulaz
		Bto4B(z, 0xff);	// ulaz korenog direktorijuma
		memcpy(buffer+20, z, 4);
		FATsize -= 8;	// jer si predjena 2 ulaza
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
			///unsigned long j = 1+rootClNum;
			for(unsigned long i=rootClNum*4+8; i<ClusterSize; i+=4)	// ulancavanje slobodnih klastera
				{
					ulongTo4B(z, j);
					memcpy(buffer+4*(j+3), z, 4);
					///FATsize -= 4;
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
	///FATsize -= 4;	// nepotrebno

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
	writeFAT(k, buffer);
	if(r != 1) return 0;
	return 1;
}

char KernelFS::doesExist(char* fname)
{
	unsigned char z[4];
	Entry *e = new Entry();
	ClusterNo clNum = 0, entryNum = 0;
	int r;
	char name[9], ext[4];

	char pathBuff[30];
	strcpy(pathBuff, fname);

	// provera particije i formata
	int ind = 'A' - pathBuff[0];
	if(ind > 26 || ind < 0) return 0;
	if(!partIsMounted(pathBuff[0])) return 0;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return 0;

	getFileName(name, pathBuff);
	getFileExt(ext, pathBuff);
	if(strlen(pathBuff) - 3 - strlen(name) - 1 - strlen(ext) != 0) return 0;

	memcpy(z, KernelFS::FATcache + 8, 4);
	B4toUlong(clNum, z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return 0;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	B4toUlong(entryNum, z);

	for(unsigned int i = 1; i < entryNum; i++)	//search root directory
	{
		if(i * 20>ClusterSize)	// cache next cluster
		{
			clNum++;
			r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
			if(r != 1) return 0;
			currClNum = clNum;
			KernelFS::clCached = true;
		}
		memcpy(e, KernelFS::clCache + i * 20, 20);

		bool rightE = true;
		for(int j = 0; j < 8; j++) rightE &= (e->name[j] == name[j]) & (e->ext[j] == ext[j]);
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
	int ind = 'A' - pathBuff[0];
	if(ind > 26 || ind < 0) return NULL;
	if(!partIsMounted(pathBuff[0])) return NULL;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return NULL;

	// fajl
	char name[9], ext[4];
	getFileName(name, fname);
	getFileExt(ext, fname);
	if(strlen(fname) - 3 - strlen(name) - 1 - strlen(ext) != 0) return NULL;

	unsigned char z[4];
	Entry *e = new Entry();
	ClusterNo clNum = 0, entryNum = 0;
	int r;

	memcpy(z, KernelFS::FATcache + 8, 4);
	B4toUlong(clNum, z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return NULL;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	B4toUlong(entryNum, z);

	unsigned int i, n, br = 0;
	bool compF = false, swapped = false;
	char clCache2[ClusterSize];
	for(i = 1; i < entryNum; i++)	//search root directory
	{
		n = i % (ClusterSize / 20);
		if(i / (ClusterSize / 20) > br)	// cache next cluster
		{
			br++;
			clNum++;
			if(compF)
			{
				r = KernelFS::partitions[ind]->readCluster(clNum, clCache2);
				if(r != 1) return 0;

				memcpy(KernelFS::clCache + (n - 1) * 20, clCache2, 20);

				r = KernelFS::partitions[ind]->writeCluster(clNum - 1, KernelFS::clCache);
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
			memcpy(e, KernelFS::clCache + n * 20, 20);

			bool rightE = true;
			for(int j = 0; j < 8; j++) rightE &= (e->name[j] == name[j]) & (e->ext[j] == ext[j]);
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
	if(compF && mode == 'w')	// fajl postoji
	{
		if(deleteFile(e, ind) != 1) return NULL;
	}
	else return NULL;	// ne postoji fajl

	ClusterNo firstCl = 0, size = 0;
	if(mode == 'w')
	{
		// create file
		// allocate a free cluster
		ClusterNo firstFree = 0, nextFree = 0;

		memcpy(z, KernelFS::FATcache, 4);
		B4toUlong(firstFree, z);

		nextFree = next(firstFree);
		writeNext(firstFree, 0xffffffff);

		ulongTo4B(z, nextFree);
		memcpy(KernelFS::FATcache, z, 4);

		// create root dir entry
		firstCl = entryNum + getClNumFromBsize((entryNum + 1) * 20);

		if(!KernelFS::clCached || currClNum != firstCl)
		{
			r = KernelFS::partitions[ind]->readCluster(firstCl, KernelFS::clCache);
			if(r != 1) return NULL;
			currClNum = firstCl;
			KernelFS::clCached = true;
		}
		memcpy(e, name, 8);
		memcpy(e + 8, ext, 3);
		ulongTo4B(z, firstFree);
		memcpy(e + 0xc, z, 4);
		clear4B(z);
		memcpy(e + 0x10, z, 4);

		memcpy(KernelFS::clCache + (entryNum % (ClusterSize / 20)), e, 20);
		r = KernelFS::partitions[ind]->writeCluster(firstFree, KernelFS::clCache);
		if(r != 1) return 0;
			
		firstCl = firstFree;
		size = 0;
	}
	else
	{
		memcpy(z, e + 0xc, 4);
		B4toUlong(firstCl, z);

		memcpy(z, e + 0x10, 4);
		B4toUlong(size, z);
	}

	KernelFile *kfile = new KernelFile(mode, name, ext, firstCl, size);
	File *file = new File();
	file->myImpl = kfile;
	r = (mode == 'a' ? file->seek(size - 1) : file->seek(0));
	if(r != 1) return NULL;
	return file;

	/*if(mode == 'w')
	{
		unsigned char b[8];
		for(cnt = 0; cnt<8; cnt++)
		{
			if(pathBuff[cnt + 3] == '.') break;
			b[cnt] = pathBuff[cnt + 3];
		}
		if(cnt < 1 || strlen(pathBuff) - cnt - 1 != 3) return NULL;
		for(int i = cnt; i<8; i++) b[i] = ' ';
		// da li treba da se pise?
		char buffer[ClusterSize] = { 0 };
		memcpy(buffer, b, 8);

		for(int i = 0; i<4; i++) b[i] = pathBuff[cnt + 4 + i];
		memcpy(buffer + 8, b, 4);
		clear4B(b);
		memcpy(buffer + 12, b, 4);
	}

	char m[1] = { mode };
	FILE *f = fopen(fname, m);
	if(f == 0) return NULL;

	File *file;
	file->myImpl = new KernelFile(f);*/

	// dodavanje u root dir

	//return file;
}

ClusterNo KernelFS::next(ClusterNo c)
{
	unsigned char z[4];
	ClusterNo nextCl = 0;

	memcpy(z, KernelFS::FATcache + 16 + c*FATBSIZE, 4);
	B4toUlong(nextCl, z);
	if(nextCl == 0xffffffff) return 0;
	return nextCl;
}

void KernelFS::writeNext(ClusterNo dest, ClusterNo sour)
{
	unsigned char z[4];
	///ClusterNo nextCl;

	ulongTo4B(z, sour);
	memcpy(KernelFS::FATcache + 16 + dest*FATBSIZE, z, 4);
}

char KernelFS::deleteFile(Entry *e, int part)
{
	//									PROVERA DA LI JE OTVOREN !!!
	unsigned char z[4];
	ClusterNo fileClNum = 0, freeClNum = 0;

	memcpy(z, e + 0xc, 4);
	B4toUlong(fileClNum, z);

	memcpy(z, KernelFS::FATcache, 4);
	B4toUlong(freeClNum, z);

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
	///ClusterNo lastCl = ((lastClNum + 4) * 4) / ClusterSize -
	///				   (((lastClNum + 4) * 4) % ClusterSize) == 0 ? 1 : 0;
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
	//									PROVERA DA LI JE OTVOREN !!!
	char pathBuff[30];
	strcpy(pathBuff, fname);

	// provera particije i formata
	int ind = 'A' - pathBuff[0];
	if(ind > 26 || ind < 0) return 0;
	if(!partIsMounted(pathBuff[0])) return 0;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return 0;

	// fajl
	char name[9], ext[4];
	getFileName(name, fname);
	getFileExt(ext, fname);
	if(strlen(fname) - 3 - strlen(name) - 1 - strlen(ext) != 0) return 0;

	unsigned char z[4];
	Entry *e = new Entry();
	ClusterNo clNum = 0, entryNum = 0;
	int r;

	memcpy(z, KernelFS::FATcache + 8, 4);
	B4toUlong(clNum, z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return 0;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	B4toUlong(entryNum, z);

	unsigned int i, n, br = 0;
	bool compF = false, swapped = false;
	char clCache2[ClusterSize];
	for(i = 1; i < entryNum; i++)	//search root directory
	{
		n = i % (ClusterSize / 20);
		if(i / (ClusterSize / 20) > br)	// cache next cluster
		{
			br++;
			clNum++;
			if(compF)
			{
				r = KernelFS::partitions[ind]->readCluster(clNum, clCache2);
				if(r != 1) return 0;

				memcpy(KernelFS::clCache + (n - 1) * 20, clCache2, 20);

				r = KernelFS::partitions[ind]->writeCluster(clNum - 1, KernelFS::clCache);
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
			memcpy(e, KernelFS::clCache + n * 20, 20);

			bool rightE = true;
			for(int j = 0; j < 8; j++) rightE &= (e->name[j] == name[j]) & (e->ext[j] == ext[j]);
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
		r = deleteFile(e, ind);
		// kompakcija (i brisanje) klastera
		// close file
		return r;
	}
}

///char KernelFS::createDir(char* dirname);

///char KernelFS::deleteDir(char* dirname);

char KernelFS::readDir(char* dirname, EntryNum n, Entry &e)
{
	unsigned char z[4];
	//Entry *e;
	ClusterNo clNum = 0, entryNum = 0;
	int r;

	char pathBuff[30];
	strcpy(pathBuff, dirname);

	// provera particije i formata
	int ind = 'A' - pathBuff[0];
	if(ind > 26 || ind < 0) return 0;
	if(!partIsMounted(pathBuff[0])) return 0;
	if(pathBuff[1] != ':' || pathBuff[2] != '\\') return 0;

	memcpy(z, KernelFS::FATcache + 8, 4);
	B4toUlong(clNum, z);
	if(!KernelFS::clCached || currClNum != clNum)
	{
		r = KernelFS::partitions[ind]->readCluster(clNum, KernelFS::clCache);
		if(r != 1) return 0;
		currClNum = clNum;
		KernelFS::clCached = true;
	}
	memcpy(z, KernelFS::clCache + 0x10, 4);
	B4toUlong(entryNum, z);

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
