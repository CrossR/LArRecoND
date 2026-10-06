/**
 *  @file   src/MasterThreeDAlgorithm.cc
 *
 *  @brief  Implementation of the 3D master algorithm class.
 *
 *  $Log: $
 */

#include "Api/PandoraApi.h"

#include "Pandora/AlgorithmHeaders.h"

#include "LArNDContent.h"
#include "MasterThreeDAlgorithm.h"

#include "larpandoracontent/LArContent.h"
#include "larpandoracontent/LArHelpers/LArPfoHelper.h"

#ifdef LIBTORCH_DL
#include "larpandoradlcontent/LArDLContent.h"
#endif

using namespace pandora;

namespace lar_content
{

MasterThreeDAlgorithm::MasterThreeDAlgorithm() :
    m_shouldRunRockMus_Xworkers(false),
    m_tagRockMuons(false)
{
    m_processedHitTypes.push_back(TPC_3D);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode MasterThreeDAlgorithm::RegisterCustomContent(const Pandora *const pPandora) const
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, LArNDContent::RegisterAlgorithms(*pPandora));
#ifdef LIBTORCH_DL
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, LArDLContent::RegisterAlgorithms(*pPandora));
#endif
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode MasterThreeDAlgorithm::TagCosmicRayPfos(const PfoToFloatMap &stitchedPfosToX0Map, PfoList &clearCosmicRayPfos, PfoList &ambiguousPfos) const
{

    PfoList ambiguousPfos_wRock;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, MasterAlgorithm::TagCosmicRayPfos(stitchedPfosToX0Map, clearCosmicRayPfos, ambiguousPfos_wRock));

    if (!m_tagRockMuons)
    {
        for (const Pfo *const pPfo : ambiguousPfos_wRock)
            ambiguousPfos.push_back(pPfo);

        return STATUS_CODE_SUCCESS;
    }

    for (RockMuonTaggingTool *const pRockMuonTaggingTool : m_rockMuonTaggingToolVector)
        pRockMuonTaggingTool->FindAmbiguousPfos(ambiguousPfos_wRock, ambiguousPfos, this);

    const PfoList *pRecreatedCRPfos(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraApi::GetCurrentPfoList(this->GetPandora(), pRecreatedCRPfos));

    for (const Pfo *const pPfo : *pRecreatedCRPfos)
    {
        const bool isClearRock(ambiguousPfos.end() == std::find(ambiguousPfos.begin(), ambiguousPfos.end(), pPfo));
        PandoraContentApi::ParticleFlowObject::Metadata metadata;
        metadata.m_propertiesToAdd["IsClearCosmic"] = (isClearRock ? 1.f : 0.f); // TODO maybe decouple labels? isClearRock, good for now
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::AlterMetadata(*this, pPfo, metadata));

        if (isClearRock)
            clearCosmicRayPfos.push_back(pPfo);
    }

    if (m_visualizeOverallRecoStatus)
    {
        PANDORA_MONITORING_API(VisualizeParticleFlowObjects(this->GetPandora(), &clearCosmicRayPfos, "ClearCRPfos", RED));
        PANDORA_MONITORING_API(VisualizeParticleFlowObjects(this->GetPandora(), &ambiguousPfos, "AmbiguousPfos", BLUE));
        PANDORA_MONITORING_API(ViewEvent(this->GetPandora()));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

std::vector<const LArTPC *> MasterThreeDAlgorithm::GetTPCsForWorker(const Pandora *const pCRWorker) const
{
    const auto iter(m_workerToLArTPCMap.find(pCRWorker->GetGeometry()->GetLArTPC().GetLArTPCVolumeId()));
    return (iter == m_workerToLArTPCMap.end()) ? std::vector<const LArTPC *>() : iter->second;
}

//------------------------------------------------------------------------------------------------------------------------------------------
void MasterThreeDAlgorithm::RecreateClusters(const ParticleFlowObject *const pInputPfo, ClusterList &newClusterList) const
{
    ClusterList totalClusterList;
    LArPfoHelper::GetTwoDClusterList(pInputPfo, totalClusterList);
    LArPfoHelper::GetThreeDClusterList(pInputPfo, totalClusterList);

    for (const Cluster *const pInputCluster : totalClusterList)
    {
        CaloHitList inputCaloHitList, newCaloHitList, newIsolatedCaloHitList;
        pInputCluster->GetOrderedCaloHitList().FillCaloHitList(inputCaloHitList);

        for (const CaloHit *const pInputCaloHit : inputCaloHitList)
            newCaloHitList.push_back(static_cast<const CaloHit *>(pInputCaloHit->GetParentAddress()));

        for (const CaloHit *const pInputCaloHit : pInputCluster->GetIsolatedCaloHitList())
            newIsolatedCaloHitList.push_back(static_cast<const CaloHit *>(pInputCaloHit->GetParentAddress()));

        if (!newCaloHitList.empty())
            newClusterList.push_back(this->CreateCluster(pInputCluster, newCaloHitList, newIsolatedCaloHitList));
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void MasterThreeDAlgorithm::CreateCosmicRayWorkerInstances(const LArTPCMap &larTPCMap, const DetectorGapList &gapList)
{
    m_workerToLArTPCMap.clear();

    if (m_shouldRunRockMus_Xworkers)
    {
        // Group TPCs (proxy for drift volumes here) that share the same XY coordinates
        // in a unique "columnar" worker instance.
        // In the next lines, a map with the following structure is created:
        // (x0, y0) <---> (0: drift_volume_z0, 1: drift_volume_z1, ...)
        // (x1, y1) <---> (0: drift_volume_z0, 1: drift_volume_z1, ...)
        // where (x, y) represents the shared coordinates of drift volumes in the same column.

        std::map<std::pair<float, float>, LArTPCMap> XYgrouped;
        const unsigned int FIRST_TPC_ID = 0;
        for (const LArTPCMap::value_type &mapEntry : larTPCMap)
        {
            const LArTPC &tpc(*(mapEntry.second));
            const auto key_xy = std::make_pair(tpc.GetCenterX(), tpc.GetCenterY());
            auto it_tpcMap = XYgrouped.find(key_xy);

            if (it_tpcMap != XYgrouped.end())
            {
                // Get the current group of tpcs correspondint to key_xy
                LArTPCMap &tpcMap = it_tpcMap->second;
                // Get the tpc id of the most recent added tpc - the newest tpc
                // id is the most recent + 1
                const unsigned int current_max_id = tpcMap.empty() ? FIRST_TPC_ID : tpcMap.rbegin()->first + 1;
                XYgrouped[key_xy].emplace(current_max_id, &tpc);
            }
            else // new column of tpcs
            {
                XYgrouped[key_xy].emplace(FIRST_TPC_ID, &tpc);
            }
        }

        // Now that we have grouped drift volumes along xy in a map create a
        // worker instance for each group
        unsigned int worker_id = 0;
        for (const auto &[xy, submap] : XYgrouped)
        {
            m_crWorkerInstances.push_back(
                this->CreateWorkerInstance(submap, gapList, m_crSettingsFile, "CRWorkerInstance" + std::to_string(worker_id), worker_id));

            // Loop over the group of TPCs along the same XY and fill the map
            // worker <---> vector<TPCs>
            for (const LArTPCMap::value_type &mapEntry : submap)
                m_workerToLArTPCMap[worker_id].push_back(mapEntry.second);

            worker_id++;
        }
    }
    else
    {
        // Default: Create 1 worker instance per drift volume
        for (const LArTPCMap::value_type &mapEntry : larTPCMap)
        {
            const unsigned int volumeId(mapEntry.second->GetLArTPCVolumeId());
            m_crWorkerInstances.push_back(
                this->CreateWorkerInstance(*(mapEntry.second), gapList, m_crSettingsFile, "CRWorkerInstance" + std::to_string(volumeId)));

            m_workerToLArTPCMap[volumeId].push_back(mapEntry.second);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode MasterThreeDAlgorithm::ReadSettings(const pandora::TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
        XmlHelper::ReadValue(xmlHandle, "ShouldRunRockMus_Xworkers", m_shouldRunRockMus_Xworkers));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "TagRockMuons", m_tagRockMuons));

    if (m_shouldRunCosmicHitRemoval)
    {
        AlgorithmToolVector algorithmToolVector;
        PANDORA_RETURN_RESULT_IF(
            STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithmToolList(*this, xmlHandle, "RockMuonTaggingTools", algorithmToolVector));

        for (AlgorithmTool *const pAlgorithmTool : algorithmToolVector)
        {
            RockMuonTaggingTool *const pRockMuonTaggingTool(dynamic_cast<RockMuonTaggingTool *>(pAlgorithmTool));
            if (!pRockMuonTaggingTool)
                return STATUS_CODE_INVALID_PARAMETER;
            m_rockMuonTaggingToolVector.push_back(pRockMuonTaggingTool);
        }
    }

    return MasterAlgorithm::ReadSettings(xmlHandle);
}

} // namespace lar_content
