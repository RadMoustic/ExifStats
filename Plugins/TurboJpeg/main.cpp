// ES
#include "QJpegTurboHandler.h"

// Qt
#include <QImageIOHandler>
#include <QImageIOPlugin>

/********************************************************************************/

class QTurboJpegPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "turbojpeg.json")

public:
    Capabilities capabilities(QIODevice* device, const QByteArray& format) const override;
    QImageIOHandler* create(QIODevice* device, const QByteArray& format = QByteArray()) const override;
};

/********************************************************************************/

QImageIOPlugin::Capabilities QTurboJpegPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "jpeg" || format == "jpg" || format == "jpe")
        return CanRead | CanWrite;

    if (!format.isEmpty())
        return {};

    if (!device->isReadable())
        return {};

    QByteArray header = device->peek(2);
    if(header.startsWith("\xFF\xD8"))
        return CanRead | CanWrite;

    return {};
}

/********************************************************************************/

QImageIOHandler* QTurboJpegPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QJpegTurboHandler* lHandler = new QJpegTurboHandler();
    lHandler->setDevice(device);
    lHandler->setFormat(format);
    return lHandler;
}

#include "main.moc"