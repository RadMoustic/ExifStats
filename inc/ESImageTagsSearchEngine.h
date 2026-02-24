#pragma once

#ifdef IMAGETAGGER_ENABLE

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

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
        std::vector<float> mEmbedding;

        float computeSimilarityScore(const TextEncodedResult& pOther) const;
	};

    /********************************* METHODS ***********************************/

    ESImageTagsSearchEngine(const QString& pModelFilePath, const QString& pTokenizerJSONFilePath);

    TextEncodedResult encode(const QString& pText);

private:
    /******************************** ATTRIBUTES **********************************/

    Ort::Env mEnv;
    Ort::Session mSession{ nullptr };
    std::unique_ptr<tokenizers::Tokenizer> mTokenizer;
    //QMutex mSessionRunMutex;
};

#endif // IMAGETAGGER_ENABLE