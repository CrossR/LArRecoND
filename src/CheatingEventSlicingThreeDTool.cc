
/**
 *  @file   larrecond/src/CheatingEventSlicingThreeDTool.cc
 *
 *  @brief  Implementation of the cheating 3D event slicing tool class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "CheatingEventSlicingThreeDTool.h"

#include "Pandora/PandoraEnumeratedTypes.h"
#include "larpandoracontent/LArHelpers/LArMCParticleHelper.h"
#include <iostream>

using namespace pandora;

namespace lar_content
{

//------------------------------------------------------------------------------------------------------------------------------------------

void CheatingEventSlicingThreeDTool::RunSlicing(const Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames,
    const HitTypeToNameMap & /*clusterListNames*/, Slice3DList &sliceList)
{
    if (PandoraContentApi::GetSettings(*pAlgorithm)->ShouldDisplayAlgorithmInfo())
        std::cout << "----> Running Algorithm Tool: " << this->GetInstanceName() << ", " << this->GetType() << std::endl;

    MCParticleTo3DSliceMap mcParticleToSliceMap;
    this->InitializeMCParticleToSliceMap(pAlgorithm, caloHitListNames, mcParticleToSliceMap);

    this->FillSlices(pAlgorithm, TPC_VIEW_U, caloHitListNames, mcParticleToSliceMap);
    this->FillSlices(pAlgorithm, TPC_VIEW_V, caloHitListNames, mcParticleToSliceMap);
    this->FillSlices(pAlgorithm, TPC_VIEW_W, caloHitListNames, mcParticleToSliceMap);
    this->FillSlices(pAlgorithm, TPC_3D, caloHitListNames, mcParticleToSliceMap);

    MCParticleVector mcParticleVector;
    for (const auto &mapEntry : mcParticleToSliceMap)
        mcParticleVector.push_back(mapEntry.first);
    std::sort(mcParticleVector.begin(), mcParticleVector.end(), LArMCParticleHelper::SortByMomentum);

    for (const MCParticle *const pMCParticle : mcParticleVector)
    {
        const Slice3D &slice(mcParticleToSliceMap.at(pMCParticle));

        if (!slice.m_caloHitListU.empty() || !slice.m_caloHitListV.empty() || !slice.m_caloHitListW.empty() || !slice.m_caloHitList3D.empty())
            sliceList.push_back(slice);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void CheatingEventSlicingThreeDTool::InitializeMCParticleToSliceMap(
    const Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames, MCParticleTo3DSliceMap &mcParticleToSliceMap) const
{
    for (const auto &mapEntry : caloHitListNames)
    {
        const CaloHitList *pCaloHitList(nullptr);
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*pAlgorithm, mapEntry.second, pCaloHitList));

        for (const CaloHit *const pCaloHit : *pCaloHitList)
        {
            MCParticleVector mcParticleVector;
            for (const auto &weightMapEntry : pCaloHit->GetMCParticleWeightMap())
                mcParticleVector.push_back(weightMapEntry.first);
            std::sort(mcParticleVector.begin(), mcParticleVector.end(), LArMCParticleHelper::SortByMomentum);

            for (const MCParticle *const pMCParticle : mcParticleVector)
            {
                const MCParticle *const pParentMCParticle(LArMCParticleHelper::GetParentMCParticle(pMCParticle));

                if (mcParticleToSliceMap.count(pParentMCParticle))
                    continue;

                if (!mcParticleToSliceMap.insert(MCParticleTo3DSliceMap::value_type(pParentMCParticle, Slice3D())).second)
                    throw StatusCodeException(STATUS_CODE_FAILURE);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void CheatingEventSlicingThreeDTool::FillSlices(const Algorithm *const pAlgorithm, const HitType hitType,
    const HitTypeToNameMap &caloHitListNames, MCParticleTo3DSliceMap &mcParticleToSliceMap) const
{
    if ((TPC_3D != hitType) && (TPC_VIEW_U != hitType) && (TPC_VIEW_V != hitType) && (TPC_VIEW_W != hitType))
        throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);

    const CaloHitList *pCaloHitList(nullptr);
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*pAlgorithm, caloHitListNames.at(hitType), pCaloHitList));

    for (const CaloHit *const pCaloHit : *pCaloHitList)
    {
        try
        {
            const MCParticle *const pMainMCParticle(MCParticleHelper::GetMainMCParticle(pCaloHit));
            const MCParticle *const pParentMCParticle(LArMCParticleHelper::GetParentMCParticle(pMainMCParticle));

            MCParticleTo3DSliceMap::iterator mapIter = mcParticleToSliceMap.find(pParentMCParticle);

            if (mcParticleToSliceMap.end() == mapIter)
                throw StatusCodeException(STATUS_CODE_FAILURE);

            Slice3D &slice(mapIter->second);
            switch (hitType) {
                case TPC_3D:
                    slice.m_caloHitList3D.push_back(pCaloHit);
                    break;
                case TPC_VIEW_U:
                    slice.m_caloHitListU.push_back(pCaloHit);
                    break;
                case TPC_VIEW_V:
                    slice.m_caloHitListV.push_back(pCaloHit);
                    break;
                case TPC_VIEW_W:
                    slice.m_caloHitListW.push_back(pCaloHit);
                    break;
                default:
                    throw StatusCodeException(STATUS_CODE_INVALID_PARAMETER);
            }
        }
        catch (const StatusCodeException &statusCodeException)
        {
            if (STATUS_CODE_FAILURE == statusCodeException.GetStatusCode())
                throw statusCodeException;
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CheatingEventSlicingThreeDTool::ReadSettings(const TiXmlHandle /*xmlHandle*/)
{
    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
