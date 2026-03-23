#ifdef IMAGETAGGER_ENABLE

#include <ESImageTagger.h>

// ES
#include <ESUtils.h>

// Qt
#include <algorithm>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>

// Onnxruntime
#include <dml_provider_factory.h>

// Stl
#include <set>

/********************************************************************************/

bool toQVector3D(const QJsonValue& pVector3D, QVector3D& pResult)
{
	if(!pVector3D.isArray())
		return false;
	
	const QJsonArray lVector3DArray = pVector3D.toArray();
	if(lVector3DArray.size() != 3)
		return false;

	pResult = { float(lVector3DArray[0].toDouble(0.)), float(lVector3DArray[1].toDouble(0.)), float(lVector3DArray[2].toDouble(0.)) };
	return true;
}

/********************************************************************************/

bool toStdString(const QJsonValue& pVector3D, std::string& pResult)
{
	if (!pVector3D.isString())
		return false;

	pResult = pVector3D.toString().toStdString();
	return true;
}

/********************************************************************************/

#define READ_JSON_VALUE(jsonObj, key, value) \
	if (jsonObj.contains(key)) \
	{ \
		QVariant lValue = pJsonObject[key].toVariant(); \
		if (lValue.canConvert<decltype(value)>()) \
			value = qvariant_cast<decltype(value)>(lValue); \
	} else { \
		qWarning() << "Format JSON object does not contain '"##key##"' field."; \
		return false; \
	}

/********************************************************************************/

#define READ_JSON_VALUE_CONVERT(jsonObj, key, value, convertFct) \
	if (jsonObj.contains(key)) \
	{ \
		if(!convertFct(pJsonObject[key], value)) \
		{ \
			qWarning() << "Failed to convert '"##key##"' field."; \
			return false; \
		} \
	} else { \
		qWarning() << "Format JSON object does not contain '"##key##"' field."; \
		return false; \
	}

/********************************************************************************/

bool ESImageTagger::FormatClassification::loadLabels(const QString& pFilePath)
{
	mLabels.clear();
	QFile lFile(pFilePath);
	if (!lFile.open(QIODevice::ReadOnly))
		return false;
	
	QTextStream lStream(&lFile);
	while (!lStream.atEnd())
		mLabels << lStream.readLine();
	
	return true;
}

/********************************************************************************/

/*virtual*/ std::vector<uint16_t> ESImageTagger::FormatClassification::getTagsFromScores(const ESEmbeddings& pScores)
{
	assert(mLabels.size() > 0 && mLabels.size() == int(pScores.size()));

	std::vector<std::pair<float, int>> lResults;
	for (int i = 0; i < int(pScores.size()); ++i)
	{
		if (pScores[i] > mScoreThreshold)
			lResults.push_back({ pScores[i], i });
	}
	std::sort(lResults.begin(), lResults.end(),
		[](const auto& a, const auto& b)
		{
			return a.first > b.first;
		});
	std::vector<uint16_t> lTopTags;
	for (int i = 0; i < mTopScoreCount && i < int(lResults.size()); ++i)
	{
		int lIndex = lResults[i].second;
		if (lIndex < mLabels.size())
			lTopTags.push_back(lIndex);
	}
	return lTopTags;
}

/********************************************************************************/

QStringList ESImageTagger::FormatClassification::getTagsLabels(const std::vector<uint16_t>& pTags)
{
	QStringList lLabels;
	for (int lIndex : pTags)
	{
		if (lIndex >= 0 && lIndex < mLabels.size())
			lLabels << mLabels[lIndex];
	}
	return lLabels;
}

/********************************************************************************/

/*virtual*/ bool ESImageTagger::Format::loadFromJSON(const QJsonObject& pJsonObject, const QString& /*pFilePath*/)
{
	READ_JSON_VALUE_CONVERT(pJsonObject, "InputName", mInputName, toStdString);
	READ_JSON_VALUE_CONVERT(pJsonObject, "OutputName", mOutputName, toStdString);
	READ_JSON_VALUE(pJsonObject, "InputWidth", mInputWidth);
	READ_JSON_VALUE(pJsonObject, "InputHeight", mInputHeight);
	READ_JSON_VALUE_CONVERT(pJsonObject, "Mean", mMean, toQVector3D);
	READ_JSON_VALUE_CONVERT(pJsonObject, "StdDev", mStdDev, toQVector3D);
	READ_JSON_VALUE(pJsonObject, "KeepAspectRatio", mKeepAspectRatio);

	return true;
}

/********************************************************************************/

/*virtual*/ bool ESImageTagger::FormatClassification::loadFromJSON(const QJsonObject& pJsonObject, const QString& pFilePath)
{
	if(!Format::loadFromJSON(pJsonObject, pFilePath))
		return false;

	READ_JSON_VALUE(pJsonObject, "ScoreThreshold", mScoreThreshold);
	READ_JSON_VALUE(pJsonObject, "TopScoreCount", mTopScoreCount);

	static const char* const scLabelsFilePathKey = "LabelsPath";
	static const char* const scLabelsKey = "Labels";

	QString lLabelsFilePath;
	if (pJsonObject.contains(scLabelsFilePathKey))
	{
		QJsonValue lLabelPathValue = pJsonObject[scLabelsFilePathKey];
		if (lLabelPathValue.isString())
			lLabelsFilePath = lLabelPathValue.toString();
		else
		{
			qWarning() << scLabelsFilePathKey << " field is not a string.";
			return false;
		}

		if (!getFilePathFromBase(lLabelsFilePath, pFilePath, lLabelsFilePath))
		{
			qWarning() << scLabelsFilePathKey << " file does not exist: " << lLabelsFilePath;
			return false;
		}

		if (!loadLabels(lLabelsFilePath))
		{
			qWarning() << "Could not read labels from file: " << lLabelsFilePath;
			return false;
		}
	}
	else if (pJsonObject.contains("Labels"))
	{
		mLabels.clear();

		QJsonValue lLabels = pJsonObject[scLabelsKey];
		if (!lLabels.isArray())
		{
			qWarning() << scLabelsKey << " field is not an array.";
			return false;
		}
		QJsonArray lLabelsArray = lLabels.toArray();
		for (const QJsonValue& lLabelValue : lLabelsArray)
		{
			if (lLabelValue.isString())
				mLabels << lLabelValue.toString();
			else
			{
				qWarning() << "Label value is not a string.";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Format JSON object does not contain '" << scLabelsFilePathKey << "' or '" << scLabelsKey << "' field.";
		return false;
	}


	return true;
}

/********************************************************************************/

void ESImageTagger::FormatObjectDetection::setCocoLabels()
{
	mLabels = {
		"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
		"fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
		"elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
		"skis", "snowboard", "ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "racket", "bottle",
		"wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
		"broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed",
		"table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven",
		"toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
	};
}

/********************************************************************************/

/*virtual*/ std::vector<uint16_t> ESImageTagger::FormatObjectDetection::getTagsFromScores(const ESEmbeddings& pScores) /*override*/
{
	assert(mLabels.size() > 0);

	std::set<int> lFoundTags;

	for (int x = 0; x < mOutputCols; ++x)
	{
		float lMaxScore = 0.0f;
		int lClassId = -1;

		// First 4 rows are bounding box coordinates, so we start from row 4
		for (int y = 4; y < mOutputRows; ++y)
		{
			float lScore = pScores[y * mOutputCols + x];
			if (lScore > lMaxScore)
			{
				lMaxScore = lScore;
				lClassId = y - 4;
			}
		}

		if (lMaxScore > mScoreThreshold && lClassId >= 0)
		{
			lFoundTags.insert(lClassId);
		}
	}
	return std::vector<uint16_t>(lFoundTags.begin(), lFoundTags.end());
}

/********************************************************************************/

/*virtual*/ bool ESImageTagger::FormatObjectDetection::loadFromJSON(const QJsonObject& pJsonObject, const QString& pFilePath)
{
	if(!Format::loadFromJSON(pJsonObject, pFilePath))
		return false;

	READ_JSON_VALUE(pJsonObject, "OutputRows", mOutputRows);
	READ_JSON_VALUE(pJsonObject, "OutputCols", mOutputCols);

	return true;
}

/********************************************************************************/

/*static*/ std::shared_ptr<ESImageTagger::Format> ESImageTagger::CreateFormatFromType(FormatType pType)
{
	if(pType == FormatType::Classification)
		return std::make_shared<FormatClassification>();
	else if(pType == FormatType::ObjectDetection)
		return std::make_shared<FormatObjectDetection>();
	else if (pType == FormatType::Embedding)
		return std::make_shared<FormatEmbedding>();
	else
		return nullptr;
}

/********************************************************************************/

/*static*/ std::shared_ptr<ESImageTagger::FormatClassification> ESImageTagger::CreateResNetFormat()
{
	std::shared_ptr<ESImageTagger::FormatClassification> lFormat = std::make_shared<FormatClassification>();
	lFormat->mInputName = "data";
	lFormat->mOutputName = "resnetv24_dense0_fwd";
	lFormat->mInputWidth = 224;
	lFormat->mInputHeight = 224;
	lFormat->mMean = { 0.485f, 0.456f, 0.406f };
	lFormat->mStdDev = { 0.229f, 0.224f, 0.225f };
	lFormat->mKeepAspectRatio = true;
	lFormat->mScoreThreshold = 0.5f;
	lFormat->mTopScoreCount = 3;
	return lFormat;
}

/********************************************************************************/

/*static*/ std::shared_ptr<ESImageTagger::FormatClassification> ESImageTagger::CreatePlace365Format()
{
	std::shared_ptr<FormatClassification> lFormat = CreateResNetFormat();
	lFormat->mInputName = "input";
	lFormat->mOutputName = "output";
	lFormat->mKeepAspectRatio = false;

	return lFormat;
}

/********************************************************************************/

/*static*/ std::shared_ptr<ESImageTagger::FormatClassification> ESImageTagger::CreateConvNextFormat()
{
	std::shared_ptr<FormatClassification> lFormat = CreateResNetFormat();
	lFormat->mInputName = "input";
	lFormat->mOutputName = "output";
	lFormat->mKeepAspectRatio = false;

	return lFormat;
}

/********************************************************************************/

/*static*/ std::shared_ptr<ESImageTagger::FormatObjectDetection> ESImageTagger::CreateYoloFormat()
{
	std::shared_ptr<ESImageTagger::FormatObjectDetection> lFormat = std::make_shared<FormatObjectDetection>();
	lFormat->mInputName = "images";
	lFormat->mOutputName = "output0";
	lFormat->mInputWidth = 640;
	lFormat->mInputHeight = 640;
	lFormat->mMean = { 0.0f, 0.0f, 0.0f };
	lFormat->mStdDev = { 1.0f, 1.0f, 1.0f };
	lFormat->mKeepAspectRatio = false;
	lFormat->mOutputCols = 8400;
	lFormat->mOutputRows = 84;
	lFormat->mScoreThreshold = 0.5f;
	lFormat->mTopScoreCount = 3;
	lFormat->setCocoLabels();
	return lFormat;
}

/********************************************************************************/

ESImageTagger::ESImageTagger(const QString& pModelPath, std::shared_ptr<Format> pFormat)
	: mModelPath(pModelPath)
	, mFormat(pFormat)
{
	mEnv = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "ESImageTagger");
}

/********************************************************************************/

std::shared_ptr<ESImageTagger::Format> ESImageTagger::getFormat() const
{
	return mFormat;
}

/********************************************************************************/

ESEmbeddings ESImageTagger::processImage(const QImage& pImage)
{
	QImage lResizedImage = pImage.scaled(mFormat->mInputWidth, mFormat->mInputHeight, mFormat->mKeepAspectRatio ? Qt::KeepAspectRatioByExpanding : Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	// Convert the QImage to tensors (NCHW, Number of data samples, Channels, Height, Width)
	int lInputSize = mFormat->mInputWidth * mFormat->mInputHeight;
	std::vector<float> lInputTensorValues(1 * 3 * mFormat->mInputHeight * mFormat->mInputWidth);
	for (int y = 0; y < mFormat->mInputHeight; y++)
	{
		for (int x = 0; x < mFormat->mInputWidth; x++)
		{
			QRgb lPixel = lResizedImage.pixel(x, y);
			// Normalize then substract the mean and divide by the std deviation
			lInputTensorValues[0 * lInputSize + y * mFormat->mInputHeight + x] = (qRed(lPixel) / 255.0f - mFormat->mMean[0]) / mFormat->mStdDev[0];
			lInputTensorValues[1 * lInputSize + y * mFormat->mInputHeight + x] = (qGreen(lPixel) / 255.0f - mFormat->mMean[1]) / mFormat->mStdDev[1];
			lInputTensorValues[2 * lInputSize + y * mFormat->mInputHeight + x] = (qBlue(lPixel) / 255.0f - mFormat->mMean[2]) / mFormat->mStdDev[2];
		}
	}

	// Execute the inference
	Ort::MemoryInfo lMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	const char* lInputNames[] = { mFormat->mInputName.c_str() };
	const char* lOutputNames[] = { mFormat->mOutputName.c_str() };

	int64_t lInputDims[] = { 1, 3, mFormat->mInputWidth, mFormat->mInputHeight };
	Ort::Value lInputTensor = Ort::Value::CreateTensor<float>(lMemoryInfo, lInputTensorValues.data(), lInputTensorValues.size(), lInputDims, 4);

	std::vector<Ort::Value> lOutputTensors;
	{
		QMutexLocker lLock(&mSessionRunMutex); // DirectML is not thread safe, CUDA is but the redist is too much of a hassle
		lOutputTensors = mSession->Run(Ort::RunOptions{ nullptr }, lInputNames, &lInputTensor, 1, lOutputNames, 1);
	}

	size_t lOutputTensorSize = mSession->GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetElementCount();

	ESEmbeddings lResult;
	lResult.resize(lOutputTensorSize);
	std::memcpy(lResult.data(), lOutputTensors.front().GetTensorMutableData<float>(), lResult.size() * sizeof(float));

	return lResult;
}

/********************************************************************************/

void ESImageTagger::initializeSession()
{
	if(!mSession)
	{
		Ort::SessionOptions lSessionOptions;
		//OrtCUDAProviderOptions lCudaOptions;
		//lSessionOptions.AppendExecutionProvider_CUDA(lCudaOptions);
		OrtSessionOptionsAppendExecutionProvider_DML(lSessionOptions, 0);
		mSession.reset(new Ort::Session(mEnv, mModelPath.toStdWString().c_str(), lSessionOptions));
	}
}

/********************************************************************************/

void ESImageTagger::cleanupSession()
{
	mSession.release();
}

#endif // IMAGETAGGER_ENABLE