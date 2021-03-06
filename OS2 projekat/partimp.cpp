#include "partimp.h"
#include "part.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

///using namespace std;

PartitionImpl::PartitionImpl(char *initName):wclinit(false)
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

	if(size <= 0)
	{
		std::cout << "GRESKA: Velicina particije mora biti pozitivna i veca od nule!" << std::endl;
		exit(1);
	}
	
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
	if(workingClNum != num || !wclinit)
	{
		FILE *f=fopen(name, "rb");
		if(f==0) return 0;
		fseek(f, num*ClusterSize, SEEK_SET);
		if(fread(workingCl, 1, ClusterSize, f) != ClusterSize) return 0;
		workingClNum = num;
		wclinit = true;
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
	if(workingClNum != num || !wclinit)
	{
		fseek(f, num*ClusterSize, SEEK_SET);
		if(fread(workingCl, 1, ClusterSize, f) != ClusterSize) return 0;
		workingClNum = num;
		wclinit = true;
	}
	memcpy(workingCl, buffer, ClusterSize);
	fwrite(workingCl, 1, ClusterSize, f);
	fclose(f);
	return 1;
}

//PartitionImpl::~PartitionImpl()
