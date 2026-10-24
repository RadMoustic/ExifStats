#include "ESUtils.h"

// Qt
#include <QFile>
#include <QFileInfo>

// Stl
#ifdef Q_OS_ANDROID
#include <fstream>
#endif

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

/********************************************************************************/

#ifdef Q_OS_ANDROID
long long getAppMemoryUsage()
{
	std::ifstream lFile("/proc/self/status");
	std::string lLine;

	while (std::getline(lFile, lLine))
	{
		if (lLine.rfind("VmRSS:", 0) == 0)
		{
			size_t lPos = lLine.find_first_of("0123456789");
			if (lPos != std::string::npos)
			{
				return std::stoll(lLine.substr(lPos)) * 1024;
			}
		}
	}

	return 0;
}
#endif