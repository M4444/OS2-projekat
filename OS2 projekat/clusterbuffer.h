// clusterbuffer.h
#pragma once
#include "part.h"
#include "kernelfs.h"

class ClusterBuffer{
private:
	char cluster[ClusterSize];
	unsigned long it;

	int ind, k;
public:
	ClusterBuffer(int ind, int k);

	void reset();

	int getCurrClusterNum();
	void setCurrClusterNum(int k);

	int spaceLeft();

	bool writeByte(char b);
	bool write(char *array, int n);
	bool writeEntry(IndEntry entry);
	void writeAll(char b);

	bool writeToPart();
};