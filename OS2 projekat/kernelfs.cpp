#include "kernelfs.h"
#include "part.h"
#include <cstring>

KernelFS::KernelFS()
{
	for(int i=0; i<26; i++) partitions[i] = nullptr;
}

///KernelFS::~KernelFS ();

void KernelFS::Bto4B(char* z, char c)
{
	for(int i = 0; i < 4; ++i) z[i] = c;
}

void KernelFS::ulongTo4B(char* z, unsigned long l)
{
	for(int i=0; i<4; i++) z[i] = char ((l >> i*8) & 0xff);
}

void KernelFS::clear4B(char* z)
{
	Bto4B(z, 0);
}

char KernelFS::mount(Partition* partition)
{
	int i=0;
	while ((partitions[i] != nullptr) || i = 26)
	{
		i++;
	}
	if(i == 26) return '0';
	partitions[i] = partition;
	return 'A'+i;
}

char KernelFS::unmount(char part)
{
	int ind = 'A'-part;
	if(ind > 26 || ind < 0) return '0';
	
	if (partitions[ind] != nullptr)
	{
		partitions[ind] = nullptr;
		return '1';		// uspeh
	}
	else return '0';	// neuspeh
	
	///char r = (partitions[ind] == NULL) ? '0' : '1';
	///partitions[ind] = NULL;
	///return r;
}

char KernelFS::format(char part)
{
	int ind = 'A'-part;
	if(ind > 26 || ind < 0) return 0;
	
	///Cluster *c = new Cluster();
	int r;
	char buffer[ClusterSize] = {0};
	
	unsigned long size = partitions[ind]->getNumOfClusters();
	unsigned long FATsize = size*FATBSIZE;	// size in bytes
	
	if(size <= 0) return 0;
	
	if(FATsize+16 < ClusterSize)	// FAT staje u nulti klaster
	{
		// prepare buffer
		char z[4] = { 2, 0, 0 ,0 };	// broj ulaza pocetka liste slobodnih klastera 
		memcpy(buffer, z, 4);
		ulongTo4B(z, size);	// broj ulaza u FAT tabeli
		memcpy(buffer+4, z, 4);
		for(int i=1; i<4) z[i] = 0; z[0] = 1;	// broj ulaza korenog direktorijuma
		memcpy(buffer+8, z, 4);
		Bto4B(z, 0);	// broj ulaza u korenom direktorijumu
		// 0? Treba da se menja kad se napravi novi fajl
		memcpy(buffer+12, z, 4);
		// FAT
			// preskace nulti ulaz
		Bto4B(z, 0xff);	// ulaz korenog direktorijuma
		memcpy(buffer+20, z, 4);
		FATsize -= 8;	// jer si predjena 2 ulaza
		unsigned long j = 3;
		while(FATsize > 4)	// ulancavanje slobodnih klastera
		{
			ulongTo4B(z, j);
			memcpy(buffer+4*(j+3), z, 4);
			FATsize -= 4;
			j++;
		}
		Bto4B(z, 0xff);	// kraj liste
		memcpy(buffer+4*(j+3), z, 4);
		///FATsize -= 4;	// nepotrebno
		
		r = partitions[ind]->writeCluster(0, buffer);
		if(r != 1) return 0;
		
		// koreni dijektorijum
		z[0] = part;
		z[1] = ':';
		z[2] = '\\';
		z[3] = ' ';
		memcpy(buffer, z, 4);
		Bto4B(z, ' ');
		memcpy(buffer+4, z, 4);
		z[0] = 'd';
		z[1] = 'i';
		z[2] = 'r';
		z[3] = 0;
		clear4B(z);
		memcpy(buffer+8, z, 4);
		memcpy(buffer+12, z, 4);

		r = partitions[ind]->writeCluster(1, buffer);
		if(r != 1) return 0;
	}
	else	// FAT ne staje u nulti klaster
	{
		bool checkRoot = true;
		unsigned long rootClNum = (FATsize+16)/ClusterSize + 
								  ((FATsize+16)%ClusterSize)==0?1:2;
		// prepare buffer				  
		char z[4];
		ulongTo4B(z, rootClNum+1);	// broj ulaza pocetka liste slobodnih klastera 
		memcpy(buffer, z, 4);
		ulongTo4B(z, size);	// broj ulaza u FAT tabeli
		memcpy(buffer+4, z, 4);
		ulongTo4B(z, rootClNum);	// broj ulaza korenog direktorijuma
		memcpy(buffer+8, z, 4);
		Bto4B(z, 0);	// broj ulaza u korenom direktorijumu
		// 0? Treba da se menja kad se napravi novi fajl
		memcpy(buffer+12, z, 4);
		
		// FAT
		unsigned long j = 1 + rootClNum;
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
		r = partitions[ind]->writeCluster(0, buffer);
		if(r != 1) return 0;
		FATsize -= ClusterSize-16;
		unsigned long k = 1;
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
			r = partitions[ind]->writeCluster(k, buffer);
			if(r != 1) return 0;
			FATsize -= ClusterSize;
			k++;
		}
		// last buffer
		///unsigned long j = 3;
		while(FATsize > 4)	// ulancavanje slobodnih klastera
		{
			ulongTo4B(z, j);
			memcpy(buffer+4*(j+3), z, 4);
			FATsize -= 4;
			j++;
		}
		Bto4B(z, 0xff);	// kraj liste
		memcpy(buffer+4*(j+3), z, 4);
		///FATsize -= 4;	// nepotrebno
		
		r = partitions[ind]->writeCluster(k, buffer);
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
		clear4B(z);
		memcpy(buffer + 8, z, 4);
		memcpy(buffer + 12, z, 4);

		r = partitions[ind]->writeCluster(k, buffer);
		if(r != 1) return 0;
	}
	return 1;
}

char KernelFS::doesExist(char* fname)
{
	//	TODO
}

File* KernelFS::open(char* fname, char mode)
{
	//	TODO
}

char KernelFS::deleteFile(char* fname)
{
	//	TODO
}

///char KernelFS::createDir(char* dirname);

///char KernelFS::deleteDir(char* dirname);

char KernelFS::readDir(char* dirname, EntryNum n, Entry &e)
{
	//	TODO
}
