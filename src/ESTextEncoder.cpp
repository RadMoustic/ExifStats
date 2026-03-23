#ifdef IMAGETAGGER_ENABLE

#include <ESTextEncoder.h>

// Onnxruntime
#include <dml_provider_factory.h>

// Qt
#include <QFile>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESTextEncoder::ESTextEncoder(const QString& pModelFilePath, const QString& pTokenizerJSONFilePath, const QString& pConfigFile)
{
	mEnv = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "ESImageTagsSearchEngine");
	Ort::SessionOptions lSessionOptions;
	//OrtCUDAProviderOptions lCudaOptions;
	//lSessionOptions.AppendExecutionProvider_CUDA(lCudaOptions);
	//OrtSessionOptionsAppendExecutionProvider_DML(lSessionOptions, 0);
	mSession = Ort::Session(mEnv, pModelFilePath.toStdWString().c_str(), lSessionOptions);

	QFile lTokenizerFile(pTokenizerJSONFilePath);
	if (lTokenizerFile.open(QIODevice::ReadOnly))
	{
		mTokenizer = tokenizers::Tokenizer::FromBlobJSON(lTokenizerFile.readAll().toStdString());
	}

	loadConfigFile(pConfigFile);
}

/********************************************************************************/

void ESTextEncoder::loadConfigFile(const QString& pConfigFile)
{
	QFile lFile(pConfigFile);
	if (!lFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Failed to open tokenizer config file '" << pConfigFile << "' with error: " << lFile.errorString();
		return;
	}

	QByteArray lJsonData = lFile.readAll();
	lFile.close();
	QJsonParseError lJsonParseError;
	QJsonDocument lJsonDoc = QJsonDocument::fromJson(lJsonData, &lJsonParseError);
	if (lJsonDoc.isNull())
	{
		qWarning() << "Failed to parse Tokenizer JSON config file '" << pConfigFile << "': char " << lJsonParseError.offset << " : " << lJsonParseError.errorString() << "";
		return;
	}

	QJsonObject lRoot = lJsonDoc.object();

	mId32Bit = lRoot.value("idFormat").toString() == "int32";
	mHasTokenTypeIds = lRoot.value("hasTokenTypeIds").toBool(false);
	mInputIdsName = lRoot.value("inputIdsName").toString("").toStdString();
	mOutputEmbeddingName = lRoot.value("outputEmbeddingName").toString("").toStdString();

	mEnabled = !mInputIdsName.empty() && !mOutputEmbeddingName.empty();
}

/********************************************************************************/

ESTextEncoder::TextEncodedResult ESTextEncoder::encode(const QString& pText)
{
	ESTextEncoder::TextEncodedResult lResult;

	if(mEnabled)
	{
		if(mId32Bit)
			lResult = internalEncode<int32_t>(pText);
		else
			lResult = internalEncode<int64_t>(pText);
	}

	return lResult;
}

/********************************************************************************/

float ESTextEncoder::TextEncodedResult::computeSimilarityScore(const TextEncodedResult& pOther) const
{
	return computeSimilarityScore(pOther.mEmbeddings);
}

/********************************************************************************/

float ESTextEncoder::TextEncodedResult::computeSimilarityScore(const ESEmbeddings& pEmbeddings) const
{
	assert(mEmbeddings.size() == pEmbeddings.size());

	float lDot = 0;

	for (int i = 0; i < mEmbeddings.size(); ++i)
		lDot += mEmbeddings[i] * pEmbeddings[i];

	return lDot;
}

#endif // IMAGETAGGER_ENABLE