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
		Classification = 0, // Assigning class labels to the entire image, with confidence scores
		ObjectDetection = 1, // Detection of objects in the image, with bounding boxes and class labels
		Embedding = 2, // Vectors of features extracted from the image, used for similarity search
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
	
        virtual bool loadFromJSON(const QJsonObject& pJsonObject, const QString& pFilePath);
    };

	struct FormatClassification : public Format
    {
        QStringList mLabels;
        float mScoreThreshold = 0.f;
        int mTopScoreCount = 0;

        bool loadLabels(const QString& pFilePath);
        virtual std::vector<uint16_t> getTagsFromScores(const std::vector<float>& pScores);
        QStringList getTagsLabels(const std::vector<uint16_t>& pTags);

        virtual bool loadFromJSON(const QJsonObject& pJsonObject, const QString& pFilePath) override;
    };

    struct FormatObjectDetection : public FormatClassification
    {
        int mOutputRows;
		int mOutputCols;

        void setCocoLabels();

        virtual std::vector<uint16_t> getTagsFromScores(const std::vector<float>& pScores) override;

        virtual bool loadFromJSON(const QJsonObject& pJsonObject, const QString& pFilePath) override;
    };

    struct FormatEmbedding : public Format
    {

    };

    /********************************* METHODS ***********************************/

    static std::shared_ptr<Format> CreateFormatFromType(FormatType pType);

    static std::shared_ptr<FormatClassification> CreateResNetFormat();
    static std::shared_ptr<FormatClassification> CreatePlace365Format();
    static std::shared_ptr<FormatClassification> CreateConvNextFormat();
    static std::shared_ptr<FormatObjectDetection> CreateYoloFormat();

    ESImageTagger(const QString& pModelPath, std::shared_ptr<Format> pFormat);

    void initializeSession();
    void cleanupSession();

	std::shared_ptr<Format> getFormat() const;

    std::vector<float> processImage(const QImage& pImage);

private:
    /******************************** ATTRIBUTES **********************************/

	QString mModelPath;
    std::shared_ptr<Format> mFormat;
    Ort::Env mEnv;
    std::unique_ptr<Ort::Session> mSession;
    QMutex mSessionRunMutex;
};

#endif // IMAGETAGGER_ENABLE