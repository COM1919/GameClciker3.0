#pragma once
#include "Definitons.h"

class AISimpl
{
public:
	AISimpl(const string path);
	~AISimpl();
private:
	ofstream DataFile;

	
};

AISimpl::AISimpl(const string path)
{
	DataFile = ofstream(path);
}

AISimpl::~AISimpl()
{
	DataFile.close();
}