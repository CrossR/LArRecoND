/**
 *  @file   src/DLEventSlicingThreeDTool.cc
 *
 *  @brief  Implementation of the ND DL event slicing tool class.
 *
 *  $Log: $
 */

#ifdef LIBTORCH_DL

#include "LArThreeDHitHelper.h"
#include "Pandora/AlgorithmHeaders.h"

#include "DLEventSlicingThreeDTool.h"

using namespace pandora;

namespace lar_content
{

void DLEventSlicingThreeDTool::InitializeHitLookup(const Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames)
{
    m_hitIndexToCaloHitListMap.clear();

    for (const auto &hitListNamePair : caloHitListNames)
    {
        const CaloHitList *pCaloHitList(nullptr);
        PANDORA_THROW_RESULT_IF_AND_IF(
            STATUS_CODE_SUCCESS, STATUS_CODE_NOT_INITIALIZED, !=, PandoraContentApi::GetList(*pAlgorithm, hitListNamePair.second, pCaloHitList));

        if (!pCaloHitList)
            throw StatusCodeException(STATUS_CODE_NOT_FOUND);

        for (const CaloHit *const pCaloHit : *pCaloHitList)
        {
            m_hitIndexToCaloHitListMap[LArThreeDHitHelper::GetHitIndex(pCaloHit)].push_back(pCaloHit);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void DLEventSlicingThreeDTool::AddHitToSlice(const CaloHit *const pCaloHit, Slice &slice) const
{
    const intptr_t hitIndex(LArThreeDHitHelper::GetHitIndex(pCaloHit));

    const auto iter(m_hitIndexToCaloHitListMap.find(hitIndex));
    if (iter == m_hitIndexToCaloHitListMap.end())
        throw StatusCodeException(STATUS_CODE_NOT_FOUND);

    for (const CaloHit *const pHit : iter->second)
    {
        if (TPC_VIEW_U == pHit->GetHitType())
            slice.m_caloHitListU.push_back(pHit);
        else if (TPC_VIEW_V == pHit->GetHitType())
            slice.m_caloHitListV.push_back(pHit);
        else if (TPC_VIEW_W == pHit->GetHitType())
            slice.m_caloHitListW.push_back(pHit);
        else if (TPC_3D == pHit->GetHitType())
            slice.m_caloHitList3D.push_back(pHit);
        else
            throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);
    }
}

} // namespace lar_content

#endif // #ifdef LIBTORCH_DL
