// https://github.com/jakar/qt-heif-image-plugin

#include "QHeifHandler.h"

// Qt
#include <QImageIOHandler>
#include <QStringList>
#include <QIODevice>
#include <QByteArray>

/********************************************************************************/

class QHeifPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "heif.json")

public:
    Capabilities capabilities(QIODevice* pDevice, const QByteArray& pFormat) const override;
    QImageIOHandler *create(QIODevice* pDevice, const QByteArray& pFormat = QByteArray()) const override;
};

/********************************************************************************/

QHeifPlugin::Capabilities QHeifPlugin::capabilities(QIODevice* pDevice, const QByteArray& pFormat) const
{
    const bool lFormatOK = (pFormat == "heic" || pFormat == "heics" || pFormat == "heif" || pFormat == "heifs");

    if (!lFormatOK && !pFormat.isEmpty())
        return {};

    if (!pDevice)
    {
        if (lFormatOK)
            return CanRead | CanWrite;
        else
            return {};
    }

    Capabilities lCaps;

    if (pDevice->isReadable() && QHeifHandler::canReadFrom(*pDevice) != QHeifHandler::Format::None)
        lCaps |= CanRead;

    if (pDevice->isWritable())
        lCaps |= CanWrite;

    return lCaps;
}

/********************************************************************************/

QImageIOHandler *QHeifPlugin::create(QIODevice* pDevice, const QByteArray& pFormat) const
{
    QImageIOHandler* lHandler = new QHeifHandler;
    lHandler->setDevice(pDevice);
    lHandler->setFormat(pFormat);
    return lHandler;
}

/********************************************************************************/

#include "main.moc"
