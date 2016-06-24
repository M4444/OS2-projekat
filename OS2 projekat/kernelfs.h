// File: kernelfs.h
#pragma once

#include "fs.h"
#include "part.h"
#include <Windows.h>

#define pToInd(x) ('A' - (x))
#define indToP(x) ('A' + (x))

const unsigned int IndEntrySize = 32/8;
const unsigned int IndEntryNum = ClusterSize / IndEntrySize;

typedef unsigned long IndEntry;

const unsigned int FATBITSIZE = 32;
const unsigned int FATBSIZE = FATBITSIZE/8;

class File;
class KernelFile;

class KernelFS {
private:
	static Partition* partitions[26];
	static char* FATcache;
	static char clCache[ClusterSize];
	static ClusterNo currClNum;
	static bool clCached;
	
public:
	KernelFS();
	~KernelFS ();
	static char mount(Partition* partition);	//montira particiju
												// vraca dodeljeno slovo
												// ili 0 u slucaju neuspeha
	
	static char unmount(char part); //demontira particiju oznacenu datim
									// slovom vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
									
	static char format(char part); 	// formatira particiju sa datim slovom;
									// vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	
	static char doesExist(char* fname); //argument je naziv fajla ili 
										//direktorijuma zadat apsolutnom putanjom
	
	static File* open(char* fname, char mode); 	//prvi argument je naziv fajla
												// zadat apsolutnom putanjom, drugi modalitet
	
	static char deleteFile(char* fname); 	// argument je naziv fajla
											// zadat apsolutnom putanjom
	
	static char createDir(char* dirname); 	//argument je naziv
											//direktorijuma zadat apsolutnom putanjom 
	
	static char deleteDir(char* dirname); 	//argument je naziv
											//direktorijuma zadat apsolutnom putanjom
	
	static char readDir(char* dirname, EntryNum n, Entry &e); 
		//prvim argumentom se zadaje apsolutna putanja, drugim redni broj
		//ulaza koji se cita, treci argument je adresa na kojoj se smesta 
		//procitani ulaz

	static char writeFile(KernelFile *f, BytesCnt Bcnt, char* buffer);
	static BytesCnt readFile(KernelFile *f, BytesCnt Bcnt, char* buffer);

	static CRITICAL_SECTION critical;
private:
	static void Bto4B(unsigned char* z, const unsigned char c);
	static void ulongTo4B(unsigned char* z, const unsigned long l);
	static unsigned long B4toUlong(const unsigned char* z);
	static void clear4B(unsigned char* z);
	static bool partIsMounted(char p);
	static void writeFAT(ClusterNo num, const char *buffer);
	static int getFileName(char *ext, const char* fname);
	static void getFileExt(char *ext, const char* fname);
	static ClusterNo getClNumFromBsize(unsigned int sizeB);
	static int getMountNum(char part); // new

	static char deleteFile(char *e, int part);
	static ClusterNo next(ClusterNo c);
	static void writeNext(ClusterNo dest, ClusterNo sour);

	static bool clearBitVectorBit(int n, int ind);

	friend class ClusterBuffer;
public:
	static char oldFormat(char part);
};