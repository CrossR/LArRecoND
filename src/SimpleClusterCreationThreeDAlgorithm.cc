/**
 *  @file   src/SimpleClusterCreationThreeDAlgorithm.cc
 *
 *  @brief  Implementation of the 3D cluster creation algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArClusterHelper.h"
#include "larpandoracontent/LArUtility/KDTreeLinkerToolsT.h"

#include "SimpleClusterCreationThreeDAlgorithm.h"

using namespace pandora;

namespace lar_content
{

SimpleClusterCreationThreeDAlgorithm::SimpleClusterCreationThreeDAlgorithm()
{
    m_clusteringWindowSquared = 0.25f;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void SimpleClusterCreationThreeDAlgorithm::BuildAssociationMap(const CaloHitVector &caloHitVector, HitAssociationMap &hitAssociationMap) const
{
    typedef KDTreeLinkerAlgo<const CaloHit *, 3> HitKDTree;
    typedef KDTreeNodeInfoT<const CaloHit *, 3> HitKDNode;

    const CaloHitList caloHitList(caloHitVector.begin(), caloHitVector.end());
    std::vector<HitKDNode> hitKDNodes;
    HitKDTree kdTree;
    kdTree.build(hitKDNodes, fill_and_bound_3d_kd_tree(caloHitList, hitKDNodes));

    const float window(std::sqrt(m_clusteringWindowSquared));

    for (const CaloHit *const pCaloHitI : caloHitVector)
    {
        std::vector<HitKDNode> found;
        kdTree.search(build_3d_kd_search_region(pCaloHitI, window, window, window), found);

        CaloHitVector nearbyHits;

        for (const HitKDNode &node : found)
        {
            const CaloHit *const pCaloHitJ(node.data);

            if (pCaloHitJ != pCaloHitI && (pCaloHitI->GetPositionVector() - pCaloHitJ->GetPositionVector()).GetMagnitudeSquared() < m_clusteringWindowSquared)
                nearbyHits.push_back(pCaloHitJ);
        }

        if (!nearbyHits.empty())
        {
            std::sort(nearbyHits.begin(), nearbyHits.end(), LArClusterHelper::SortHitsByPosition);
            hitAssociationMap.emplace(pCaloHitI, std::move(nearbyHits));
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode SimpleClusterCreationThreeDAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName3D", m_inputCaloHitListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputClusterListName3D", m_outputClusterListName));

    return SimpleClusterCreationAlgorithm::ReadSettings(xmlHandle);
}

} // namespace lar_content
