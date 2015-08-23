#include "part.h"
#include "partimp.h"

Partition::Partition(char *initName)
{
	myImpl = new PartitionImpl(initName);
}

ClusterNo Partition::getNumOfClusters() const
{
	myImpl->getNumOfClusters();
}

int Partition::readCluster(ClusterNo num, char *buffer)
{
	myImpl->readCluster(num, buffer);
}

int Partition::writeCluster(ClusterNo num, const char *buffer)
{
	myImpl->writeCluster(num, buffer);
}

Partition::~Partition()
{
	delete myImpl;
}