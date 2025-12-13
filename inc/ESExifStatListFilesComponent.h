#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESExifStatComponent.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct FileInfo;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESExifStatListFilesComponent : public QObject, public ExifStatComponent
{
	Q_OBJECT
public:
	std::vector<StringId> mFiles;

	virtual void addFile(const FileInfo& pFile) override
	{
		mFiles.push_back(pFile.mFilePath);
	}

	virtual void reset() override
	{
		mFiles.clear();
		emit listFilesChanged();
	}

	virtual void onAllFilesAdded() override
	{
		emit listFilesChanged();
	}

signals:
	/********************************** SIGNALS ***********************************/

	void listFilesChanged();
};