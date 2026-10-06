/**
 *  @file   include/MasterThreeDAlgorithm.h
 *
 *  @brief  Header file for the master algorithm class.
 *
 *  $Log: $
 */
#ifndef LAR_MASTER_THREE_D_ALGORITHM_H
#define LAR_MASTER_THREE_D_ALGORITHM_H 1

#include "Pandora/AlgorithmTool.h"
#include "Pandora/ExternallyConfiguredAlgorithm.h"

#include "larpandoracontent/LArControlFlow/MasterAlgorithm.h"
#include "larpandoracontent/LArControlFlow/MultiPandoraApi.h"
#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include "RockMuonTaggingTool.h"

#include <unordered_map>

namespace lar_content
{

typedef std::unordered_map<unsigned int, std::vector<const pandora::LArTPC *>> WorkerToLArTPCMap;
//------------------------------------------------------------------------------------------------------------------------------------------

/**
 *  @brief  MasterThreeDAlgorithm class
 */
class MasterThreeDAlgorithm : public MasterAlgorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    MasterThreeDAlgorithm();

protected:
    void CreateCosmicRayWorkerInstances(const pandora::LArTPCMap &larTPCMap, const pandora::DetectorGapList &gapList) override;

    std::vector<const pandora::LArTPC *> GetTPCsForWorker(const pandora::Pandora *const pCRWorker) const override;

    void RecreateClusters(const pandora::ParticleFlowObject *const pInputPfo, pandora::ClusterList &newClusterList) const override;

    pandora::StatusCode RegisterCustomContent(const pandora::Pandora *const pPandora) const override;

    /**
     *  @brief  Tag clear, unambiguous cosmic-ray pfos
     *
     *  @param  stitchedPfosToX0Map a map of cosmic-ray pfos that have been stitched between lar tpcs to the X0 shift
     *  @param  clearCosmicRayPfos to receive the list of clear cosmic-ray pfos
     *  @param  ambiguousPfos to receive the list of ambiguous cosmic-ray pfos for further analysis
     */
    pandora::StatusCode TagCosmicRayPfos(
        const PfoToFloatMap &stitchedPfosToX0Map, pandora::PfoList &clearCosmicRayPfos, pandora::PfoList &ambiguousPfos) const override;

    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle) override;

    typedef std::vector<RockMuonTaggingTool *> RockMuonTaggingToolVector;

    bool m_shouldRunRockMus_Xworkers;                      ///< Whether to run rock muons reconstruction using a columnar X worker
    bool m_tagRockMuons;                                   ///< bool to activate tagging of rock muons
    RockMuonTaggingToolVector m_rockMuonTaggingToolVector; ///< The cosmic-ray tagging tool vector
    WorkerToLArTPCMap m_workerToLArTPCMap;                 ///< mapping between worker instances and LArTPCs
};

} // namespace lar_content

#endif // #ifndef LAR_MASTER_THREE_D_ALGORITHM_H
