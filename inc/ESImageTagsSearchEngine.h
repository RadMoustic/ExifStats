#pragma once

#ifdef IMAGETAGGER_ENABLE

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESFileInfo.h"

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

// Tokenizers
#include "tokenizers_cpp.h"

// Qt
#include <QString>
#include <QStringList>
#include <QVector3D>
#include <QMutex>

// Stl
#include <memory>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageTagsSearchEngine
{
public:
    struct TextEncodedResult
    {
        ESEmbeddings mEmbeddings;

        float computeSimilarityScore(const TextEncodedResult& pOther) const;
        float computeSimilarityScore(const ESEmbeddings& pEmbeddings) const;
	};

    /********************************* METHODS ***********************************/

    ESImageTagsSearchEngine(const QString& pModelFilePath, const QString& pTokenizerJSONFilePath, const QString& pConfigFile);

    void loadConfigFile(const QString& pConfigFile);

    ESImageTagsSearchEngine::TextEncodedResult encode(const QString& pText);

private:
    /******************************** ATTRIBUTES **********************************/

    Ort::Env mEnv;
    Ort::Session mSession{ nullptr };
    std::unique_ptr<tokenizers::Tokenizer> mTokenizer;

    bool mEnabled = false;
	bool mId32Bit = false;
	bool mHasTokenTypeIds = false;
	std::string mInputIdsName;
    std::string mOutputEmbeddingName;

    /********************************* METHODS ***********************************/

    template<typename EmbeddingType>
    ESImageTagsSearchEngine::TextEncodedResult internalEncode(const QString& pText)
    {
        TextEncodedResult lResult;

        if (!mTokenizer)
            return lResult;

        std::vector<int32_t> lEncoding = mTokenizer->Encode(pText.toUtf8().toStdString());

        assert(lEncoding.size() >= 2 && "Set add_special_tokens to true in huggingface_tokenizer.cc encode(...) call");

        size_t lInputTensorSize = mSession.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetElementCount();

        std::vector<EmbeddingType> lIds;
        lIds.assign(std::max(lInputTensorSize, lEncoding.size()), 0);

        std::vector<EmbeddingType> lAttentionMask(lIds.size(), 0);     // 0 => All token not important by default
        std::vector<EmbeddingType> lTokenTypeIds(lIds.size(), 0);     // 0 => only one sentence

        for (int i = 0; i < lEncoding.size(); ++i)
        {
            lIds[i] = static_cast<EmbeddingType>(lEncoding[i]);
            lAttentionMask[i] = 1; // Important
        }

        std::vector<int64_t> lShape = { 1, static_cast<int64_t>(lIds.size()) };
        Ort::MemoryInfo lMem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        std::vector<Ort::Value> lInputTensors;
        lInputTensors.push_back(Ort::Value::CreateTensor<EmbeddingType>(lMem, lIds.data(), lIds.size(), lShape.data(), lShape.size()));
        lInputTensors.push_back(Ort::Value::CreateTensor<EmbeddingType>(lMem, lAttentionMask.data(), lAttentionMask.size(), lShape.data(), lShape.size()));
		if (mHasTokenTypeIds)
            lInputTensors.push_back(Ort::Value::CreateTensor<EmbeddingType>(lMem, lTokenTypeIds.data(), lTokenTypeIds.size(), lShape.data(), lShape.size()));

        std::vector<const char*> lInputNames;
        lInputNames.push_back(mInputIdsName.c_str());
        lInputNames.push_back("attention_mask");
        if(mHasTokenTypeIds)
            lInputNames.push_back("token_type_ids");
        const char* lOutputNames[] = { mOutputEmbeddingName.c_str()};

        std::vector<Ort::Value> lOutputTensors = mSession.Run(Ort::RunOptions{ nullptr }, lInputNames.data(), lInputTensors.data(), lInputTensors.size(), lOutputNames, 1);

        float* lRawData = lOutputTensors[0].GetTensorMutableData<float>();

        const std::vector<int64_t> lOutputShape = lOutputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
        const int64_t lDim = lOutputShape[lOutputShape.size() == 3 ? 2 : 1];

        lResult.mEmbeddings.assign(lDim, 0.0f);

        for (int64_t d = 0; d < lDim; ++d)
            lResult.mEmbeddings[d] = lRawData[d];

        // Normalize
        double lNorm = 0;
        for (float x : lResult.mEmbeddings)
            lNorm += x * x;
        if (lNorm > 0.f)
        {
            lNorm = std::sqrt(lNorm);
            for (float& x : lResult.mEmbeddings)
                x /= lNorm;
        }
 
        return lResult;
    }
};

#endif // IMAGETAGGER_ENABLE