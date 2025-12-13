#include "QJpegTurboHandler.h"
#include <QImage>
#include <QDebug>

/********************************************************************************/

QJpegTurboHandler::QJpegTurboHandler()
    : mQuality(75)
{
}

/********************************************************************************/

bool QJpegTurboHandler::canRead() const
{
    return true;
}

/********************************************************************************/

bool QJpegTurboHandler::read(QImage* pOutImage)
{
    return loadJpeg(pOutImage);
}

/********************************************************************************/

bool QJpegTurboHandler::write(const QImage& pImage)
{
    return saveJpeg(pImage);
}

/********************************************************************************/

bool QJpegTurboHandler::loadJpeg(QImage* pImage)
{
    QByteArray lData = device()->readAll();

    tjhandle lHandle = tjInitDecompress();
    if (!lHandle)
        return false;

    int lWidth, lHeight, lSubsamp, lColorSpace;
    if (tjDecompressHeader3(lHandle,
        (unsigned char*)lData.data(), lData.size(),
        &lWidth, &lHeight, &lSubsamp, &lColorSpace) != 0)
    {
        tjDestroy(lHandle);
        return false;
    }

    *pImage = QImage(lWidth, lHeight, QImage::Format_RGBA8888);
    if (pImage->isNull())
    {
        tjDestroy(lHandle);
        return false;
    }

    if (tjDecompress2(
        lHandle,
        (unsigned char*)lData.data(), lData.size(),
        pImage->bits(),
        lWidth,
        pImage->bytesPerLine(),
        lHeight,
        TJPF_RGBA,
        TJFLAG_FASTDCT) != 0)
    {
        tjDestroy(lHandle);
        return false;
    }

    tjDestroy(lHandle);

    return true;
}

/********************************************************************************/

bool QJpegTurboHandler::saveJpeg(const QImage& pImage)
{
    QImage rgb = pImage.convertToFormat(QImage::Format_RGB888);

    tjhandle handle = tjInitCompress();

    unsigned char *jpegBuf = nullptr;
    unsigned long jpegSize = 0;

    if (tjCompress2(
            handle,
            rgb.bits(),
            rgb.width(), rgb.bytesPerLine(), rgb.height(),
            TJPF_RGB,
            &jpegBuf, &jpegSize,
            TJSAMP_444,
        mQuality,
            TJFLAG_FASTDCT) != 0)
    {
        tjDestroy(handle);
        return false;
    }

    device()->write((const char*)jpegBuf, jpegSize);

    tjFree(jpegBuf);
    tjDestroy(handle);
    return true;
}

/********************************************************************************/

QVariant QJpegTurboHandler::option(ImageOption pOption) const
{
    switch (pOption)
    {
    case QImageIOHandler::Quality:
        return mQuality;
    default:
        return QVariant();
    }
}

/********************************************************************************/

/*virtual*/ void QJpegTurboHandler::setOption(QImageIOHandler::ImageOption pOption, const QVariant& pValue) /*override*/
{
    switch (pOption)
    {
    case QImageIOHandler::Quality:
    {
        mQuality = pValue.toInt();
		break;
    }
    }
}

/********************************************************************************/

bool QJpegTurboHandler::supportsOption(ImageOption pOption) const
{
    switch (pOption)
    {
    case QImageIOHandler::Quality:
    {
        return true;
    }
    default:
        return false;
    }
}
