#include "part.h"
#include "partimp.h"

Partition::Partition(char *initName)
{
	myImpl = new PartitionImpl(initName);
}

ClusterNo Partition::getNumOfClusters() const
{
	return myImpl->getNumOfClusters();
}

int Partition::readCluster(ClusterNo num, char *buffer)
{
	return myImpl->readCluster(num, buffer);
}

int Partition::writeCluster(ClusterNo num, const char *buffer)
{
	return myImpl->writeCluster(num, buffer);
}

Partition::~Partition()
{
	delete myImpl;
}