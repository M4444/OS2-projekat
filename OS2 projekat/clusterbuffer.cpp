#include "clusterbuffer.h"

#include "kernelfs.h"
#include <cstring>

ClusterBuffer::ClusterBuffer(int ind, int k)
{
	this->ind = ind;
	this->k = k;

	reset();
}

void ClusterBuffer::reset()
{
	memset(cluster, 0, ClusterSize);
	it = 0;
}

int ClusterBuffer::getCurrClusterNum()
{
	return k;
}
void ClusterBuffer::setCurrClusterNum(int k)
{
	this->k = k;
}

int ClusterBuffer::spaceLeft()
{
	return ClusterSize - it;
}

bool ClusterBuffer::writeByte(char b)
{
	if(it == ClusterSize)
	{
		int r = KernelFS::partitions[ind]->writeCluster(k, cluster);
		if(r != 1) return false;
		reset();
	}

	cluster[it] = b;
	it++;

	return true;
}

bool ClusterBuffer::write(char *array, int n)
{
	bool r;
	for(int i = 0; i < n; i++)
	{
		r = writeByte(array[i]);
		if(!r) return false;
	}

	return true;
}

bool ClusterBuffer::writeEntry(IndEntry entry)
{
	IndEntry entryBuff;
	bool r;
	for(int i = 0; i < 4; i++)
	{
		entryBuff = entry & 0x000000ff;
		r = writeByte((char)entryBuff);
		if(!r) return false;
		entry >>= 8;
	}
	return true;
}

void ClusterBuffer::writeAll(char b)
{
	memset(cluster, b, ClusterSize);
	it = ClusterSize;
}


bool ClusterBuffer::writeToPart()
{
	return KernelFS::partitions[ind]->writeCluster(k++, cluster);
}