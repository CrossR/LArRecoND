/**
 *  @file   larrecond/LArNDContent.cc
 *
 *  @brief  Factory implementations for content intended for use with particle flow reconstruction at liquid argon time projection chambers
 *
 *  $Log: $
 */

#include "Api/PandoraApi.h"

#include "Pandora/Algorithm.h"
#include "Pandora/AlgorithmTool.h"
#include "Pandora/Pandora.h"


#include "larrecond/LArCheating/CheatingEventSlicingThreeDTool.h"
#include "larrecond/LArCheating/CheatingRockMuonRemovalAlgorithm.h"

#include "larrecond/LArClusterCreation/CreateTwoDClustersFromThreeDAlgorithm.h"
#include "larrecond/LArClusterCreation/CutClusterCharacterisationThreeDAlgorithm.h"
#include "larrecond/LArClusterCreation/SimpleClusterCreationThreeDAlgorithm.h"

#include "larrecond/LArControlFlow/EventSlicingThreeDTool.h"
#include "larrecond/LArControlFlow/MasterThreeDAlgorithm.h"
#include "larrecond/LArControlFlow/PreProcessingThreeDAlgorithm.h"
#include "larrecond/LArControlFlow/ReplaceHitAndClusterListsAlgorithm.h"
#include "larrecond/LArControlFlow/SlicingThreeDAlgorithm.h"

#include "larrecond/LArMonitoring/HierarchyAnalysisAlgorithm.h"

#include "larrecond/LArPFOs/MergeClearTracksThreeDAlgorithm.h"
#include "larrecond/LArPFOs/PfoThreeDHitAssignmentAlgorithm.h"

#include "larrecond/LArVertex/CandidateVertexCreationThreeDAlgorithm.h"

#include "LArNDContent.h"

// clang-format off
#define LAR_ND_ALGORITHM_LIST(d)                                                                                                   \
    d("LArCandidateVertexCreationThreeD",       CandidateVertexCreationThreeDAlgorithm)                                            \
    d("LArCheatingRockMuonRemoval",             CheatingRockMuonRemovalAlgorithm)                                                  \
    d("LArCreateTwoDClustersFromThreeD",        CreateTwoDClustersFromThreeDAlgorithm)                                             \
    d("LArCutClusterCharacterisationThreeD",    CutClusterCharacterisationThreeDAlgorithm)                                         \
    d("LArHierarchyAnalysis",                   HierarchyAnalysisAlgorithm)                                                        \
    d("LArMasterThreeD",                        MasterThreeDAlgorithm)                                                             \
    d("LArMergeClearTracksThreeD",              MergeClearTracksThreeDAlgorithm)                                                   \
    d("LArPfoThreeDHitAssignment",              PfoThreeDHitAssignmentAlgorithm)                                                   \
    d("LArPreProcessingThreeD",                 PreProcessingThreeDAlgorithm)                                                      \
    d("LArReplaceHitAndClusterLists",           ReplaceHitAndClusterListsAlgorithm)                                                \
    d("LArSimpleClusterCreationThreeD",         SimpleClusterCreationThreeDAlgorithm)                                              \
    d("LArSlicingThreeD",                       SlicingThreeDAlgorithm)

#define LAR_ND_ALGORITHM_TOOL_LIST(d)                                                                                              \
    d("LArCheatingEventSlicingThreeD",          CheatingEventSlicingThreeDTool)                                                    \
    d("LArEventSlicingThreeD",                  EventSlicingThreeDTool)

#define FACTORY Factory

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

namespace lar_content
{

#define LAR_ND_CONTENT_CREATE_ALGORITHM_FACTORY(a, b)                                                                           \
class b##FACTORY : public pandora::AlgorithmFactory                                                                             \
{                                                                                                                               \
public:                                                                                                                         \
    pandora::Algorithm *CreateAlgorithm() const {return new b;};                                                                \
};

LAR_ND_ALGORITHM_LIST(LAR_ND_CONTENT_CREATE_ALGORITHM_FACTORY)

//------------------------------------------------------------------------------------------------------------------------------------------

#define LAR_ND_CONTENT_CREATE_ALGORITHM_TOOL_FACTORY(a, b)                                                                      \
class b##FACTORY : public pandora::AlgorithmToolFactory                                                                         \
{                                                                                                                               \
public:                                                                                                                         \
    pandora::AlgorithmTool *CreateAlgorithmTool() const {return new b;};                                                        \
};

LAR_ND_ALGORITHM_TOOL_LIST(LAR_ND_CONTENT_CREATE_ALGORITHM_TOOL_FACTORY)

} // namespace lar_content

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

#define LAR_ND_CONTENT_REGISTER_ALGORITHM(a, b)                                                                                 \
{                                                                                                                               \
    const pandora::StatusCode statusCode(PandoraApi::RegisterAlgorithmFactory(pandora, a, new lar_content::b##FACTORY));        \
    if (pandora::STATUS_CODE_SUCCESS != statusCode)                                                                             \
        return statusCode;                                                                                                      \
}

#define LAR_ND_CONTENT_REGISTER_ALGORITHM_TOOL(a, b)                                                                            \
{                                                                                                                               \
    const pandora::StatusCode statusCode(PandoraApi::RegisterAlgorithmToolFactory(pandora, a, new lar_content::b##FACTORY));    \
    if (pandora::STATUS_CODE_SUCCESS != statusCode)                                                                             \
        return statusCode;                                                                                                      \
}

pandora::StatusCode LArNDContent::RegisterAlgorithms(const pandora::Pandora &pandora)
{
    LAR_ND_ALGORITHM_LIST(LAR_ND_CONTENT_REGISTER_ALGORITHM);
    LAR_ND_ALGORITHM_TOOL_LIST(LAR_ND_CONTENT_REGISTER_ALGORITHM_TOOL);
    return pandora::STATUS_CODE_SUCCESS;
}

// clang-format on
