/**
 *  @file   src/CheatingRockMuonRemovalAlgorithm.cc
 *
 *  @brief  Implementation of the cheating rock muon removal algorithm.
 *
 *  $Log: $
 */

#include "Api/PandoraContentApi.h"
#include "Objects/CaloHit.h"
#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"
#include "larpandoracontent/LArHelpers/LArGeometryHelper.h"
#include "larpandoracontent/LArHelpers/LArMCParticleHelper.h"

#include "CheatingRockMuonRemovalAlgorithm.h"

using namespace pandora;

namespace lar_content
{

//------------------------------------------------------------------------------------------------------------------------------------------

CheatingRockMuonRemoval::CheatingRockMuonRemoval() {}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CheatingRockMuonRemoval::Run()
{

    // Get the input CaloHitList and MC particles...
    const CaloHitList *pCaloHitList(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputCaloHitListName3D, pCaloHitList));
    const MCParticleList *pMCParticleList(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pMCParticleList));

    // Check every hit, to see it it comes from a parent, in-detector neutrino, or an OOFV rock muon.
    CaloHitList nuCaloHitList;

    // Get the detector boundaries, used for rock muon identification.
    const auto detectorBoundaries(LArGeometryHelper::GetDetectorBoundaries(this->GetPandora()));

    for (const CaloHit *pCaloHit : *pCaloHitList)
    {
        const LArCaloHit *const pLArCaloHit(dynamic_cast<const LArCaloHit *>(pCaloHit));
        const auto mcWeights(pLArCaloHit->GetMCParticleWeightMap());

        if (mcWeights.empty())
            continue;

        const MCParticle *largestContributor(nullptr);
        float largestWeight(-1.f);

        for (const auto &mcWeight : mcWeights)
        {
            const MCParticle *mc{mcWeight.first};
            const auto parent(LArMCParticleHelper::GetParentMCParticle(mc));

            if (mcWeight.second > largestWeight)
                largestContributor = parent;
        }

        const auto vertex(largestContributor->GetVertex());
        if (!LArGeometryHelper::IsInDetector(detectorBoundaries, vertex))
            continue;

        nuCaloHitList.push_back(pCaloHit);
    }

    if (nuCaloHitList.empty())
        return STATUS_CODE_SUCCESS;

    // We now have a list of hits that mostly came from in-detector neutrinos.
    // We want to replace the original CaloHitList with this one, so that we can
    // remove the rock muon hits from the original list.

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::RenameList<CaloHitList>(*this, m_inputCaloHitListName3D, "Input" + m_inputCaloHitListName3D));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList(*this, nuCaloHitList, m_inputCaloHitListName3D));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<CaloHit>(*this, m_inputCaloHitListName3D));

    std::cout << "CheatingRockMuonRemoval: Replaced " << pCaloHitList->size() << " hits with " << nuCaloHitList.size() << " hits from in-detector neutrinos." << std::endl;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CheatingRockMuonRemoval::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName3D", m_inputCaloHitListName3D));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
