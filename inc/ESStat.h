#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFileInfo.h"
#include "ESStatComponent.h"


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESFilter;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESStat
{
public:
	ESStat(){}
	virtual ~ESStat(){}

	void addFileCategory(const ESFileInfo& pFile)
	{
		for (auto& lComp : mComponents)
			lComp->addFileCategory(pFile);
	}

	void addFile(const ESFileInfo& pFile)
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

	void addComponent(ESStatComponent* pComponent)
	{
		mComponents.push_back(pComponent);
	}

protected:
	std::vector<ESStatComponent*> mComponents;
};