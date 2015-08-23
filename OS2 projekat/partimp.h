// partimp.h
#pragma once

#include "part.h"
#include <string>

/*
struct Cluster {
	unsigned char c [ClusterSize];
	unsigned char &operator[](int n) { return c[n]; }
};*/

class PartitionImpl{
public:
	PartitionImpl(char *);
	
	ClusterNo getNumOfClusters() const;
	
	int readCluster(ClusterNo, char *buffer);
	int writeCluster(ClusterNo, const char *buffer);
	
	//virtual ~PartitionImpl();
private:
	char name[100];
	ClusterNo size;
	
	///Cluster *workingCl;
	char workingCl[ClusterSize];
	unsigned long workingClNum;
	bool wclinit;
};