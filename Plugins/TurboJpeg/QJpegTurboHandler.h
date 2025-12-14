#pragma once
#include <QImageIOHandler>
#include "turbojpeg.h"

class QJpegTurboHandler : public QImageIOHandler
{
public:
	QJpegTurboHandler();
	bool canRead() const override;
	bool read(QImage* pOutImage) override;
	bool write(const QImage& pImage) override;

	QVariant option(ImageOption pOption) const override;
	virtual void setOption(QImageIOHandler::ImageOption pOption, const QVariant& pValue) override;
	bool supportsOption(ImageOption pOption) const override;

private:
	int mQuality;

	bool loadJpeg(QImage* pImage);
	bool saveJpeg(const QImage& pImage);
};