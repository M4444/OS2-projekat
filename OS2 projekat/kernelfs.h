// File: kernelfs.h
#pragma once

#include "fs.h"
#include "part.h"

const unsigned int FATBITSIZE = 32;
const unsigned int FATBSIZE = FATBITSIZE/8;

class File;

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
private:
	//static void clFormat(Claster *c);
	static void Bto4B(unsigned char* z, const unsigned char c);
	static void ulongTo4B(unsigned char* z, const unsigned long l);
	static void B4toUlong(unsigned long l, const unsigned char* z);
	static void clear4B(unsigned char* z);
	static bool partIsMounted(char p);
	static void writeFAT(ClusterNo num, const char *buffer);
	static void getFileName(char *ext, const char* fname);
	static void getFileExt(char *ext, const char* fname);
	static ClusterNo getClNumFromBsize(unsigned int sizeB);

	static char deleteFile(Entry *e, int part);
	static ClusterNo next(ClusterNo c);
	static void writeNext(ClusterNo dest, ClusterNo sour);
};