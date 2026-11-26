/**
 *  @file   src/LArThreeDHitHelper.cc
 *
 *  @brief  Implementation of the 3D hit helper class.
 *
 *  $Log: $
 */

#include "LArThreeDHitHelper.h"

using namespace pandora;

namespace lar_content
{

//------------------------------------------------------------------------------------------------------------------------------------------

intptr_t LArThreeDHitHelper::GetHitIndex(const pandora::CaloHit *const pCaloHit)
{
    const pandora::CaloHit *const pParentCaloHit(static_cast<const pandora::CaloHit *>(pCaloHit->GetParentAddress()));
    return reinterpret_cast<intptr_t>(pParentCaloHit->GetParentAddress());
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArThreeDHitHelper::GetAllChild2DHits(const pandora::CaloHitList *const caloHitList3D, const pandora::CaloHitList *const caloHitListU,
    const pandora::CaloHitList *const caloHitListV, const pandora::CaloHitList *const caloHitListW, CaloHitMapping &caloHitMapping)
{
    std::map<intptr_t, const pandora::CaloHit *> uHitMap;
    std::map<intptr_t, const pandora::CaloHit *> vHitMap;
    std::map<intptr_t, const pandora::CaloHit *> wHitMap;

    for (const pandora::CaloHit *const pCaloHit : *caloHitListU)
        uHitMap.insert(std::make_pair(GetHitIndex(pCaloHit), pCaloHit));

    for (const pandora::CaloHit *const pCaloHit : *caloHitListV)
        vHitMap.insert(std::make_pair(GetHitIndex(pCaloHit), pCaloHit));

    for (const pandora::CaloHit *const pCaloHit : *caloHitListW)
        wHitMap.insert(std::make_pair(GetHitIndex(pCaloHit), pCaloHit));

    for (const pandora::CaloHit *const pCaloHit : *caloHitList3D)
    {
        const intptr_t parentAddress(GetHitIndex(pCaloHit));

        const pandora::CaloHit *pUHit(nullptr);
        const pandora::CaloHit *pVHit(nullptr);
        const pandora::CaloHit *pWHit(nullptr);

        if (uHitMap.find(parentAddress) != uHitMap.end())
            pUHit = uHitMap.at(parentAddress);

        if (vHitMap.find(parentAddress) != vHitMap.end())
            pVHit = vHitMap.at(parentAddress);

        if (wHitMap.find(parentAddress) != wHitMap.end())
            pWHit = wHitMap.at(parentAddress);

        if (!pUHit || !pVHit || !pWHit)
            continue;

        caloHitMapping.insert(std::make_pair(pCaloHit,
            std::map<const pandora::HitType, const pandora::CaloHit *>{
                {pandora::HitType::TPC_VIEW_U, pUHit}, {pandora::HitType::TPC_VIEW_V, pVHit}, {pandora::HitType::TPC_VIEW_W, pWHit}}));
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

} // namespace lar_content
