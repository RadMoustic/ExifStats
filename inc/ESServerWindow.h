#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <qquickview.h>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESQmlBinder;
class ESDebugQmlBinder;
class ESDatabase;
class QFileSystemWatcher;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESServerWindow : public QQuickView
{
	Q_OBJECT
public:
	/******************************** ATTRIBUTES **********************************/

	/********************************* METHODS ***********************************/

	ESServerWindow();
	virtual ~ESServerWindow() override;

	void initialize();

signals:
	/********************************** SIGNALS ***********************************/


protected:
	/******************************** ATTRIBUTES **********************************/

	std::shared_ptr<QFileSystemWatcher> mQmlFileWatcher;
	std::shared_ptr<ESQmlBinder> mBinder;
	std::shared_ptr<ESDebugQmlBinder> mDebugBinder;
	bool mFallbackQmlLoaded;

	/********************************* METHODS ***********************************/

	// Qml
	virtual void initializeQml();
	QString getMainQmlFilePath() const;
	void loadMainQml();
	void logQmlErrors();

	// Slots
	void onQuit();
	void onQmlWarnings(const QList<QQmlError>& pWarnings);
	void onQmlFileChanged(const QString& pFilePath);
};

