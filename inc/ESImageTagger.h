#pragma once

#ifdef IMAGETAGGER_ENABLE

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

// Qt
#include <QImage>
#include <QStringList>
#include <QVector3D>
#include <QMutex>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class QJsonObject;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageTagger
{
public:
    /********************************** TYPES *************************************/

    enum FormatType : int
    {
        ImageNet = 0,
        YOLO = 1
    };

    struct Format
    {
        std::string mInputName;
        std::string mOutputName;
		int mInputWidth = 0;
		int mInputHeight = 0;
		QVector3D mMean = { 0.0f, 0.0f, 0.0f };
		QVector3D mStdDev = { 1.0f, 1.0f, 1.0f };
		bool mKeepAspectRatio = false;
		QStringList mLabels;
        int mOutputSize = 0;
		float mScoreThreshold = 0.f;
		int mTopScoreCount = 0;

        bool loadLabels(const QString& pFilePath);
        virtual QVector<uint16_t> getTagsFromScores(float* pScores);

        virtual bool loadFromJSON(const QJsonObject& pJsonObject, const QString& pFilePath);
    };

    struct FormatYolo : public Format
    {
        int mOutputRows;
		int mOutputCols;

        void setCocoLabels();

        virtual QVector<uint16_t> getTagsFromScores(float* pScores) override;

        virtual bool loadFromJSON(const QJsonObject& pJsonObject, const QString& pFilePath);
    };

    /********************************* METHODS ***********************************/

    static std::shared_ptr<Format> CreateFormatFromType(FormatType pType);

    static std::shared_ptr<Format> CreateResNetFormat();
    static std::shared_ptr<Format> CreatePlace365Format();
    static std::shared_ptr<Format> CreateConvNextFormat();
    static std::shared_ptr<Format> CreateYoloFormat();

    ESImageTagger(const QString& pModelPath, std::shared_ptr<Format> pFormat);

	std::shared_ptr<Format> getFormat() const;

    QVector<uint16_t> generateImageTags(const QImage& pImage);
    QStringList getTagsLabels(const QVector<uint16_t>& pTags);

private:
    /******************************** ATTRIBUTES **********************************/

    std::shared_ptr<Format> mFormat;
    Ort::Env mEnv;
    Ort::Session mSession{ nullptr };
    QMutex mSessionRunMutex;
};

#endif // IMAGETAGGER_ENABLE