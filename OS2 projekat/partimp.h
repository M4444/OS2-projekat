// partimp.h
#pragma once

#include "part.h"
#include <string>

class PartitionImpl{
public:
	PartitionImpl(char *);
	
	ClusterNo getNumOfClusters() const;
	
	int readCluster(ClusterNo, char *buffer);
	int writeCluster(ClusterNo, const char *buffer);
private:
	char name[100];
	ClusterNo size;
	
	char workingCl[ClusterSize];
	unsigned long workingClNum;
	bool wclinit;

	bool *clusterFree;
	int rootDirIndex;
};