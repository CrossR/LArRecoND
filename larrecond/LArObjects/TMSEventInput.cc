/**
 *  @file   src/TMSEventInput.cc
 *
 *  @brief  Implementation of the TMS event input
 *
 *  $Log: $
 */

#include "TMSEventInput.h"

#include "Api/PandoraApi.h"
#include "Managers/PluginManager.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"
#include "larpandoracontent/LArObjects/LArMCParticle.h"
#include "larpandoracontent/LArPlugins/LArPseudoLayerPlugin.h"
#include "larpandoracontent/LArPlugins/LArRotationalTransformationPlugin.h"

namespace lar_nd_reco
{

TMSEventInput::TMSEventInput(const pandora::Pandora *pPandora, const NDParameters &parameters, const NDSimpleTPCGeom &tpcGeom) :
    NDEventInput(pPandora, parameters, tpcGeom),
    m_tms{}
{
    std::cout << "TMSEventInput nEntries = " << m_nEntries << std::endl;
    m_tms = std::make_unique<TMS>(m_pTree);
}

//------------------------------------------------------------------------------------------------------------------------------------------

TMSEventInput::~TMSEventInput()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

void TMSEventInput::CreateMCParticles(int) const
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

int TMSEventInput::CreateCaloHits(int eventIndex, int startHitIndex) const
{
    std::cout << "TMSEventInput eventIndex = " << eventIndex << ", start = " << startHitIndex << std::endl;

    m_tms->GetEntry(eventIndex);

    const int nHits = m_tms->m_NRecoHits;
    std::cout << "TMS nHits = " << nHits << ", startHitIndex = " << startHitIndex << std::endl;

    int hitCounter{startHitIndex};

    // Loop over the hits and make them into CaloHits
    for (size_t iHit = 0; iHit < nHits; ++iHit)
    {
        //std::cout<<"iHit = "<<iHit<<std::endl;
        //const float voxelX = m_tms->m_RecoHitX[iHit] * m_parameters.m_lengthScale;
        //const float voxelY = m_tms->m_RecoHitY[iHit] * m_parameters.m_lengthScale;
        //const float voxelZ = m_tms->m_RecoHitZ[iHit] * m_parameters.m_lengthScale;
        //const float voxelE = m_tms->m_RecoHitE[iHit] * m_parameters.m_energyScale;

        const float voxelX = m_tms->m_TrueHitX[iHit] * m_parameters.m_lengthScale;
        const float voxelY = m_tms->m_TrueHitY[iHit] * m_parameters.m_lengthScale;
        const float voxelZ = m_tms->m_TrueHitZ[iHit] * m_parameters.m_lengthScale;
        const float voxelE = m_tms->m_TrueHitE[iHit] * m_parameters.m_energyScale;

        //std::cout<<"CaloHit: "<<voxelX<<", "<<voxelY<<", "<<voxelZ<<", "<<voxelE<<std::endl;
        //std::cout<<"CaloHit1: "<<voxelX1<<", "<<voxelY1<<", "<<voxelZ1<<", "<<voxelE1<<std::endl;

        // Skip this hit if its coordinates or energy are NaNs
        if (std::isnan(voxelX) || std::isnan(voxelY) || std::isnan(voxelZ) || std::isnan(voxelE))
        {
            std::cout << "Ignoring hit " << iHit << " which contains NaNs: (" << voxelX << ", " << voxelY << ", " << voxelZ
                      << "), E = " << voxelE << std::endl;
            continue;
        }

        const pandora::CartesianVector voxelPos(voxelX, voxelY, voxelZ);
        const float MipE{0.00075};
        const float voxelMipEquivalentE = voxelE / MipE;
        const bool ignoreY{true};
        const int tpcID(m_tpcGeom.GetTPCNumber(voxelPos, ignoreY));
        //std::cout<<"tpcID = "<<tpcID<<std::endl;
        const float voxelWidth(m_parameters.m_voxelWidth);

        lar_content::LArCaloHitParameters caloHitParameters;
        caloHitParameters.m_positionVector = voxelPos;
        caloHitParameters.m_expectedDirection = pandora::CartesianVector(0.f, 0.f, 1.f);
        caloHitParameters.m_cellNormalVector = pandora::CartesianVector(0.f, 0.f, 1.f);
        caloHitParameters.m_cellGeometry = pandora::RECTANGULAR;
        caloHitParameters.m_cellSize0 = voxelWidth;
        caloHitParameters.m_cellSize1 = voxelWidth;
        caloHitParameters.m_cellThickness = voxelWidth;
        caloHitParameters.m_nCellRadiationLengths = 1.f;
        caloHitParameters.m_nCellInteractionLengths = 1.f;
        caloHitParameters.m_time = 0.f;
        caloHitParameters.m_inputEnergy = voxelE;
        caloHitParameters.m_mipEquivalentEnergy = voxelMipEquivalentE;
        caloHitParameters.m_electromagneticEnergy = voxelE;
        caloHitParameters.m_hadronicEnergy = voxelE;
        caloHitParameters.m_isDigital = false;
        caloHitParameters.m_hitType = pandora::TPC_3D;
        caloHitParameters.m_hitRegion = pandora::SINGLE_REGION;
        caloHitParameters.m_layer = 0;
        caloHitParameters.m_isInOuterSamplingLayer = false;
        caloHitParameters.m_pParentAddress = (void *)(static_cast<uintptr_t>(++hitCounter));
        caloHitParameters.m_larTPCVolumeId = tpcID < 0 ? 0 : tpcID;
        caloHitParameters.m_daughterVolumeId = 0;

        if (m_parameters.m_use3D)
        {
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitParameters, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitParameters, m_LArCaloHitFactory));
        }

        //if (m_parameters.m_dataFormat == NDParameters::DataFormat::TMS)
        //    PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);

        if (m_parameters.m_useLArTPC)
        {
            // Create LArCaloHits for U, V and W views assuming x is the common drift coordinate
            const float x0_cm(voxelPos.GetX());
            const float y0_cm(voxelPos.GetY());
            const float z0_cm(voxelPos.GetZ());

            // U view
            lar_content::LArCaloHitParameters caloHitPars_UView(caloHitParameters);
            caloHitPars_UView.m_hitType = pandora::TPC_VIEW_U;
            caloHitPars_UView.m_pParentAddress = (void *)(intptr_t(++hitCounter));
            const float upos_cm(m_pPandora->GetPlugins()->GetLArTransformationPlugin()->YZtoU(y0_cm, z0_cm));
            caloHitPars_UView.m_positionVector = pandora::CartesianVector(x0_cm, 0.f, upos_cm);
            //std::cout<<"U: "<<x0_cm<<", "<<upos_cm<<std::endl;
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitPars_UView, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitPars_UView, m_LArCaloHitFactory));

            //if (m_parameters.m_dataFormat == NDParameters::DataFormat::TMS)
            //        PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);

            // V view
            lar_content::LArCaloHitParameters caloHitPars_VView(caloHitParameters);
            caloHitPars_VView.m_hitType = pandora::TPC_VIEW_V;
            caloHitPars_VView.m_pParentAddress = (void *)(intptr_t(++hitCounter));
            const float vpos_cm(m_pPandora->GetPlugins()->GetLArTransformationPlugin()->YZtoV(y0_cm, z0_cm));
            //std::cout<<"V: "<<x0_cm<<", "<<vpos_cm<<std::endl;
            caloHitPars_VView.m_positionVector = pandora::CartesianVector(x0_cm, 0.f, vpos_cm);
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitPars_VView, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitPars_VView, m_LArCaloHitFactory));

            //if (m_parameters.m_dataFormat == NDParameters::DataFormat::TMS)
            //PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter),
            //						       (void *)((intptr_t)trackID), energyFrac);

            // W view
            lar_content::LArCaloHitParameters caloHitPars_WView(caloHitParameters);
            caloHitPars_WView.m_hitType = pandora::TPC_VIEW_W;
            caloHitPars_WView.m_pParentAddress = (void *)(intptr_t(++hitCounter));
            const float wpos_cm(m_pPandora->GetPlugins()->GetLArTransformationPlugin()->YZtoW(y0_cm, z0_cm));
            //std::cout<<"W: "<<x0_cm<<", "<<wpos_cm<<std::endl;
            caloHitPars_WView.m_positionVector = pandora::CartesianVector(x0_cm, 0.f, wpos_cm);
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitPars_WView, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitPars_WView, m_LArCaloHitFactory));

            //if (m_parameters.m_dataFormat == NDParameters::DataFormat::TMS)
            //	PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);

        } // use LArTPC pseudo coords
    } // hit loop

    std::cout << "startHitIndex = " << startHitIndex << ", hitCounter = " << hitCounter << std::endl;
    return hitCounter;
}

//------------------------------------------------------------------------------------------------------------------------------------------

int TMSEventInput::CreateTracks(int, int) const
{
    return 0;
}

} // namespace lar_nd_reco
