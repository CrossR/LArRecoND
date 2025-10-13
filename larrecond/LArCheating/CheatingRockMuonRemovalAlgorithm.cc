/**
 *  @file   larrecond/LArCheating/CheatingRockMuonRemovalAlgorithm.cc
 *
 *  @brief  Implementation of the cheating rock muon removal algorithm class.
 *
 *  $Log: $
 */

#include "Objects/CaloHit.h"
#include "Objects/MCParticle.h"
#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArVertexHelper.h"

#include "larrecond/LArCheating/CheatingRockMuonRemovalAlgorithm.h"

using namespace pandora;

namespace lar_content
{

bool IsRockMuon(const Pandora& pandora, const MCParticle* const pMCParticle)
{

    const bool isMuon = (std::abs(pMCParticle->GetParticleId()) == 13);
    const bool vertexInsideFV = LArVertexHelper::IsInFiducialVolume(pandora, pMCParticle->GetVertex(), "dune_nd");

    return isMuon && !vertexInsideFV;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CheatingRockMuonRemovalAlgorithm::Run()
{

    // Load the input lists
    const CaloHitList *pCaloHitList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pCaloHitList, m_inputCaloHitListName));

    const MCParticleList *pMCParticleList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pMCParticleList, m_inputMCParticleListName));

    // Create the output lists
    CaloHitList pRockMuonCaloHitList;
    CaloHitList pNeutrinoCaloHitList;

    // Process the hits
    for (const CaloHit *const pCaloHit : *pCaloHitList)
    {
        const MCParticle *const pMCParticle = MCParticleHelper::GetMainMCParticle(pCaloHit);
        if (IsRockMuon(this->GetPandora(), pMCParticle))
            pRockMuonCaloHitList.push_back(pCaloHit);
        else
            pNeutrinoCaloHitList.push_back(pCaloHit);
    }

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList(*this, pRockMuonCaloHitList, m_rockMuonCaloHitListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList(*this, pNeutrinoCaloHitList, m_neutrinoCaloHitListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<CaloHit>(*this, m_rockMuonCaloHitListName));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CheatingRockMuonRemovalAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName", m_inputCaloHitListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "RockMuonCaloHitListName", m_rockMuonCaloHitListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "NeutrinoCaloHitListName", m_neutrinoCaloHitListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "MCParticleListName", m_inputMCParticleListName));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
