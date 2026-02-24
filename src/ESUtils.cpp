#include "ESUtils.h"

// Qt
#include <QFile>
#include <QFileInfo>

/********************************************************************************/

int CeilIntDiv(int x, int y)
{
	return 1 + ((x - 1) / y);
}

/********************************************************************************/

bool getFilePathFromBase(const QString& pFilePath, const QString& pBaseFilePath, QString& pResult)
{
	if (QFile::exists(pFilePath))
	{
		pResult = pFilePath;
		return true;
	}

	QString lRelativePath = QFileInfo(pBaseFilePath).path() + "/" + pFilePath;
	if (QFile::exists(lRelativePath))
	{
		pResult = lRelativePath;
		return true;
	}

	return false;
}
