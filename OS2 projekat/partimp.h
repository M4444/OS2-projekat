// partimp.h
#pragma once

#include <string>

struct Cluster {
	unsigned char c [ClusterSize];
	unsigned char &operator[](int n) { return c[n]; }
};

class PartitionImpl {
public:
	PartitionImpl(char *);
	
	virtual ClusterNo getNumOfClusters() const;
	
	virtual int readCluster(ClusterNo, char *buffer);
	virtual int writeCluster(ClusterNo, const char *buffer);
	
	virtual ~PartitionImpl();
private:
	//PartitionImpl * myImpl;
	char name[100];
	unsigned long size;
	
	//Cluster *workingCl;
	char workingCl[ClusterSize];
	unsigned long workingClNum;
};