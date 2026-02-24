#ifdef IMAGETAGGER_ENABLE

#include <ESImageTagsSearchEngine.h>

// Onnxruntime
#include <dml_provider_factory.h>

// Qt
#include <QFile>
#include <QMutexLocker>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

ESImageTagsSearchEngine::ESImageTagsSearchEngine(const QString& pModelFilePath, const QString& pTokenizerJSONFilePath)
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
}

/********************************************************************************/

ESImageTagsSearchEngine::TextEncodedResult ESImageTagsSearchEngine::encode(const QString& pText)
{
    TextEncodedResult lResult;

    if(!mTokenizer)
        return lResult;

    std::vector<int32_t> lEncoding = mTokenizer->Encode(pText.toUtf8().toStdString());

    assert(lEncoding.size() >= 2 && "Set add_special_tokens to true in huggingface_tokenizer.cc encode(...) call");

    std::vector<int64_t> lIds;
    for (int32_t id : lEncoding)
        lIds.push_back(static_cast<int64_t>(id));

    std::vector<int64_t> lAttentionMask(lIds.size(), 1);     // 1 => All token important
    std::vector<int64_t> lTokenTypeIds(lIds.size(), 0);     // 0 => only one sentence

    std::vector<int64_t> lShape = { 1, static_cast<int64_t>(lIds.size()) };
    Ort::MemoryInfo lMem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<Ort::Value> lInputTensors;
    lInputTensors.push_back(Ort::Value::CreateTensor<int64_t>(lMem, lIds.data(), lIds.size(), lShape.data(), lShape.size()));
    lInputTensors.push_back(Ort::Value::CreateTensor<int64_t>(lMem, lAttentionMask.data(), lAttentionMask.size(), lShape.data(), lShape.size()));
    lInputTensors.push_back(Ort::Value::CreateTensor<int64_t>(lMem, lTokenTypeIds.data(), lTokenTypeIds.size(), lShape.data(), lShape.size()));

    const char* lInputNames[] = { "input_ids", "attention_mask", "token_type_ids" };
    //const char* lInputNames[] = { "input_ids", "attention_mask" };
    const char* lOutputNames[] = { "last_hidden_state" };

    std::vector<Ort::Value> lOutputTensors;
    {
        //QMutexLocker lLock(&mSessionRunMutex); // DirectML is not thread safe, CUDA is but the redist is too much of a hassle
        lOutputTensors = mSession.Run(Ort::RunOptions{ nullptr }, lInputNames, lInputTensors.data(), lInputTensors.size(), lOutputNames, 1);
    }
    float* lRawData = lOutputTensors[0].GetTensorMutableData<float>();

    
    const std::vector<int64_t> lOutputShape = lOutputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    const int64_t lSeqLen = lOutputShape[1];
    const int64_t lDim = lOutputShape[2];

    lResult.mEmbedding.assign(lDim, 0.0f);
    
    if(lSeqLen >= 3)
    {
        bool lOnlyFirst = true;

        if (lOnlyFirst)
        {
            for (int64_t d = 0; d < lDim; ++d)
                lResult.mEmbedding[d] = lRawData[d];
        }
        else
        {
            // Mean Pooling
            for (int64_t i = 1; i < lSeqLen - 1; ++i) // Ignore first and last: [CLS] and [SEP]
                for (int64_t d = 0; d < lDim; ++d)
                    lResult.mEmbedding[d] += lRawData[i * lDim + d];

            for (float& val : lResult.mEmbedding)
                val /= float(lSeqLen - 2);
        }
        
        // Normalize
        double lNorm = 0;
        for (float x : lResult.mEmbedding)
            lNorm += x * x;
        if(lNorm > 0.f)
        {
            lNorm = std::sqrt(lNorm);
            for (float& x : lResult.mEmbedding)
                x /= lNorm;
        }
    }

    return lResult;
}

/********************************************************************************/

float ESImageTagsSearchEngine::TextEncodedResult::computeSimilarityScore(const TextEncodedResult& pOther) const
{
    assert(mEmbedding.size() == pOther.mEmbedding.size());

    float lDot = 0;

    for (int i = 0; i < mEmbedding.size(); ++i)
        lDot += mEmbedding[i] * pOther.mEmbedding[i];

    return lDot;
}

#endif // IMAGETAGGER_ENABLE