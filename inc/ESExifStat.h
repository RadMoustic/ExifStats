#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFileInfo.h"
#include "ESExifStatComponent.h"


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifFilter;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExifStat
{
public:
	ExifStat(){}
	virtual ~ExifStat(){}

	void addFileCategory(const FileInfo& pFile)
	{
		for (auto& lComp : mComponents)
			lComp->addFileCategory(pFile);
	}

	void addFile(const FileInfo& pFile)
	{
		for(auto& lComp : mComponents)
			lComp->addFile(pFile);
	}

	void onAllFilesAdded()
	{
		for (auto& lComp : mComponents)
			lComp->onAllFilesAdded();
	}
	void reset()
	{
		for (auto& lComp : mComponents)
			lComp->reset();
	}

	void addComponent(ExifStatComponent* pComponent)
	{
		mComponents.push_back(pComponent);
	}

protected:
	std::vector<ExifStatComponent*> mComponents;
};