#include "partimp.h"
#include "part.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

///using namespace std;

PartitionImpl::PartitionImpl(char *initName)
{		
	std::ifstream input;
	std::string s;
	
	input.open(initName);
	if(!input.is_open())
	{
		std::cout << "GRESKA: Nije nadjen konfiguracioni fajl '" << initName << "' u os domacinu!" << std::endl;
		exit(1);
	}
	input >> s;
	strncpy(name, s.c_str(), sizeof(name));
	name[sizeof(name)-1] = '\0';

	input >> s;
	size = std::stol(s);
	
	input.close();
}

ClusterNo PartitionImpl::getNumOfClusters() const
{
	return size;
}

int PartitionImpl::readCluster(ClusterNo num, char *buffer)
{
	if(num > size || num < 0) return 0;
	// OPERZ: KONKURENTNOST!
	if(workingClNum != num)
	{
		FILE *f=fopen(name, "rb");
		if(f==0) return 0;
		if (fread(workingCl, 1, ClusterSize, f) != ClusterSize) return 0;
		workingClNum = num;
		fclose(f);
	}
	memcpy(buffer, workingCl, ClusterSize);
	return 1;
}

int PartitionImpl::writeCluster(ClusterNo num, const char *buffer)
{
	if(num > size || num < 0) return 0;
	// OPERZ: KONKURENTNOST!
	FILE *f=fopen(name, "r+b");
	if(f==0) return 0;
	if(workingClNum != num)
	{
		if (fread(workingCl, 1, ClusterSize, f) != ClusterSize) return 0;
		workingClNum = num;
	}
	memcpy(workingCl, buffer, ClusterSize);
	fwrite(workingCl, 1, ClusterSize, f);
	fclose(f);
	return 1;
}

//PartitionImpl::~PartitionImpl()
