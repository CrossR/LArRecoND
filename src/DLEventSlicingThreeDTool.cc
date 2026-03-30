/**
 *  @file   src/DLEventSlicingThreeDTool.cc
 *
 *  @brief  Implementation of the DL 3D event slicing tool class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArMCParticleHelper.h"
#include <Pandora/PandoraEnumeratedTypes.h>
#include <larpandoracontent/LArHelpers/LArClusterHelper.h>

#include "DLEventSlicingThreeDTool.h"

using namespace pandora;

namespace lar_content
{

//------------------------------------------------------------------------------------------------------------------------------------------

DLEventSlicingThreeDTool::DLEventSlicingThreeDTool()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

void DLEventSlicingThreeDTool::RunSlicing(const Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames,
    const HitTypeToNameMap &clusterListNames, Slice3DList &slice3DList)
{
    if (PandoraContentApi::GetSettings(*pAlgorithm)->ShouldDisplayAlgorithmInfo())
        std::cout << "----> Running Algorithm Tool: " << this->GetInstanceName() << ", " << this->GetType() << std::endl;

    // Get the produced 3D clusters from the DL Slicing...
    const ClusterList *pThreeDClusterList(nullptr);
    const auto threeDClusterListName(clusterListNames.at(TPC_3D));
    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_INITIALIZED, !=,
        PandoraContentApi::GetList(*pAlgorithm, threeDClusterListName, pThreeDClusterList));

    // Populate a map of HitIndex to each Hit.
    std::map<intptr_t, CaloHitList> hitIndexToCaloHitListMap;
    for (const auto &hitListNamePair : caloHitListNames)
    {
        const CaloHitList *pCaloHitList(nullptr);
        PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_INITIALIZED, !=,
            PandoraContentApi::GetList(*pAlgorithm, hitListNamePair.second, pCaloHitList));

        for (const CaloHit *const pCaloHit : *pCaloHitList)
        {
            const int hitIndex((intptr_t)pCaloHit->GetParentAddress());

            if (hitIndexToCaloHitListMap.count(hitIndex))
                throw StatusCodeException(STATUS_CODE_ALREADY_PRESENT);

            hitIndexToCaloHitListMap[hitIndex].push_back(pCaloHit);
        }
    }

    // For every 3D cluster, get all the associated 3D hits, find the
    // corresponding 2D hits and populate the slice list for all 4 views.
    for (const auto pCluster : *pThreeDClusterList)
    {

        Slice3D slice;

        CaloHitList caloHitList3D;
        LArClusterHelper::GetAllHits(pCluster, caloHitList3D);

        for (const CaloHit *const pClusterHit : caloHitList3D)
        {
            const int hitIndex((intptr_t)pClusterHit->GetParentAddress());

            if (!hitIndexToCaloHitListMap.count(hitIndex))
                throw StatusCodeException(STATUS_CODE_NOT_FOUND);

            for (const CaloHit *const pCaloHit : hitIndexToCaloHitListMap.at(hitIndex))
            {
                if (TPC_VIEW_U == pCaloHit->GetHitType())
                    slice.m_caloHitListU.push_back(pCaloHit);
                else if (TPC_VIEW_V == pCaloHit->GetHitType())
                    slice.m_caloHitListV.push_back(pCaloHit);
                else if (TPC_VIEW_W == pCaloHit->GetHitType())
                    slice.m_caloHitListW.push_back(pCaloHit);
                else if (TPC_3D == pCaloHit->GetHitType())
                    slice.m_caloHitList3D.push_back(pCaloHit);
                else
                    throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);
            }
        }

        std::cout << "DLEventSlicingThreeDTool::RunSlicing - slice has " << slice.m_caloHitListU.size() << " U hits, "
                  << slice.m_caloHitListV.size() << " V hits, " << slice.m_caloHitListW.size() << " W hits and "
                  << slice.m_caloHitList3D.size() << " 3D hits." << std::endl;

        slice3DList.push_back(slice);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DLEventSlicingThreeDTool::ReadSettings(const TiXmlHandle /*xmlHandle*/)
{
    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
