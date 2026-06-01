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

    if (!pThreeDClusterList)
        throw StatusCodeException(STATUS_CODE_NOT_FOUND);

    // And the 3D vertex seeds...
    const VertexList *pThreeDVertexList(nullptr);
    PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_INITIALIZED, !=,
        PandoraContentApi::GetList(*pAlgorithm, m_inputVertexListName3D, pThreeDVertexList));

    if (!pThreeDVertexList)
        throw StatusCodeException(STATUS_CODE_NOT_FOUND);

    // Populate a map of HitIndex to each Hit.
    std::map<intptr_t, CaloHitList> hitIndexToCaloHitListMap;
    std::map<HitType, int> hitTypeToHitCountMap;

    for (const auto &hitListNamePair : caloHitListNames)
    {
        const CaloHitList *pCaloHitList(nullptr);
        PANDORA_THROW_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_INITIALIZED, !=,
            PandoraContentApi::GetList(*pAlgorithm, hitListNamePair.second, pCaloHitList));

        if (!pCaloHitList)
            throw StatusCodeException(STATUS_CODE_NOT_FOUND);

        for (const CaloHit *const pCaloHit : *pCaloHitList)
        {
            const auto pParentCaloHit = static_cast<const CaloHit *>(pCaloHit->GetParentAddress());
            const int hitIndex((intptr_t)pParentCaloHit->GetParentAddress());

            hitIndexToCaloHitListMap[hitIndex].push_back(pCaloHit);

            if (!hitTypeToHitCountMap.count(pCaloHit->GetHitType()))
                hitTypeToHitCountMap[pCaloHit->GetHitType()] = 0;

            ++hitTypeToHitCountMap[pCaloHit->GetHitType()];
        }
    }

    // Build up a map of slice index to the candidate vertex indicies that fall within that slice.
    std::map<unsigned int, VertexList> sliceIndexToCandidateVertexIndicesMap;
    unsigned int vertexIdx{0};
    for (const auto &vertex : *pThreeDVertexList)
    {
        const auto vertexPos = vertex->GetPosition();

        // Loop through the slices and find the first one that contains this vertex.
        bool vertexAssignedToSlice{false};
        unsigned int sliceIdx{0};
        for (const auto &slice : *pThreeDClusterList)
        {
            CaloHitList caloHitList3D;
            LArClusterHelper::GetAllHits(slice, caloHitList3D);

            for (const auto &caloHit : caloHitList3D)
            {
                const auto hitPos = caloHit->GetPositionVector();
                if (hitPos == vertexPos)
                {
                    sliceIndexToCandidateVertexIndicesMap[sliceIdx].push_back(vertex);
                    vertexAssignedToSlice = true;
                    break;
                }
            }

            if (vertexAssignedToSlice)
                break;

            ++sliceIdx;
        }

        ++vertexIdx;
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
            const auto pParentCaloHit = static_cast<const CaloHit *>(pClusterHit->GetParentAddress());
            const int hitIndex((intptr_t)pParentCaloHit->GetParentAddress());

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

        for (const auto &vertex : sliceIndexToCandidateVertexIndicesMap[slice3DList.size()])
            slice.m_vertexList.push_back(vertex);

        std::cout << "DLEventSlicingThreeDTool::RunSlicing - Slice has " << slice.m_caloHitListU.size() << " U hits, "
                  << slice.m_caloHitListV.size() << " V hits, " << slice.m_caloHitListW.size() << " W hits and "
                  << slice.m_caloHitList3D.size() << " 3D hits." << std::endl;

        slice3DList.push_back(slice);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DLEventSlicingThreeDTool::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputVertexListName3D", m_inputVertexListName3D));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
