/**
 *  @file   src/LArNDEventInput.cc
 *
 *  @brief  Implementation of the LArND event input
 *
 *  $Log: $
 */

#include "LArNDEventInput.h"

#include "Api/PandoraApi.h"
#include "Managers/PluginManager.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"
#include "larpandoracontent/LArObjects/LArMCParticle.h"
#include "larpandoracontent/LArPlugins/LArPseudoLayerPlugin.h"
#include "larpandoracontent/LArPlugins/LArRotationalTransformationPlugin.h"

#include "TTree.h"

#include <numeric>

namespace lar_nd_reco
{

LArNDEventInput::LArNDEventInput(const pandora::Pandora *pPandora, const NDParameters &parameters, const NDSimpleTPCGeom &tpcGeom) :
    NDEventInput(pPandora, parameters, tpcGeom),
    m_larsp{}
{
    std::cout << "LArNDEventInput nEntries = " << m_nEntries << std::endl;
    m_larsp = (parameters.m_dataFormat == NDParameters::DataFormat::SPMC) ? std::make_unique<LArSPMC>(m_pTree) : std::make_unique<LArSP>(m_pTree);
}

//------------------------------------------------------------------------------------------------------------------------------------------

LArNDEventInput::~LArNDEventInput()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

void LArNDEventInput::CreateMCParticles(int eventIndex) const
{
    lar_content::LArMCParticleFactory mcParticleFactory;

    LArSPMC *larspmc = dynamic_cast<LArSPMC *>(m_larsp.get());
    if (!larspmc)
        return;

    larspmc->GetEntry(eventIndex);

    const int nNeutrinos(larspmc->m_nuPDG->size());
    std::cout << "Read in " << nNeutrinos << " true neutrinos" << std::endl;

    // Create MC neutrinos. Keep track of the vertex ID's of the neutrinos
    std::map<long, int> vertexIdToIndex;

    for (size_t i = 0; i < nNeutrinos; ++i)
    {
        // Unique vertex ID for the neutrino
        const long vertexID = (*larspmc->m_vertex_id)[i];
        // Store the vertex ID for this neutrino entry
        vertexIdToIndex[vertexID] = i;

        const int neutrinoPDG = (*larspmc->m_nuPDG)[i];
        const std::string reaction = this->GetNuanceReaction((*larspmc->m_ccnc)[i], (*larspmc->m_mode)[i]);
        const int nuanceCode = this->GetNuanceCode(reaction);
        const float nuVtxX = (*larspmc->m_nuvtxx)[i] * m_parameters.m_lengthScale;
        const float nuVtxY = (*larspmc->m_nuvtxy)[i] * m_parameters.m_lengthScale;
        const float nuVtxZ = (*larspmc->m_nuvtxz)[i] * m_parameters.m_lengthScale;

        const float nuE = (*larspmc->m_nue)[i] * m_parameters.m_energyScale;
        const float nuPx = (*larspmc->m_nupx)[i];
        const float nuPy = (*larspmc->m_nupy)[i];
        const float nuPz = (*larspmc->m_nupz)[i];

        lar_content::LArMCParticleParameters mcNeutrinoParameters;
        mcNeutrinoParameters.m_nuanceCode = nuanceCode;
        mcNeutrinoParameters.m_process = lar_content::MC_PROC_INCIDENT_NU;

        mcNeutrinoParameters.m_energy = nuE;
        mcNeutrinoParameters.m_momentum = pandora::CartesianVector(nuPx, nuPy, nuPz);
        mcNeutrinoParameters.m_vertex = pandora::CartesianVector(nuVtxX, nuVtxY, nuVtxZ);
        mcNeutrinoParameters.m_endpoint = pandora::CartesianVector(nuVtxX, nuVtxY, nuVtxZ);

        mcNeutrinoParameters.m_particleId = neutrinoPDG;
        mcNeutrinoParameters.m_mcParticleType = pandora::MC_3D;
        mcNeutrinoParameters.m_pParentAddress = (void *)((intptr_t)vertexID);

        PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcNeutrinoParameters, mcParticleFactory));
    }

    // Create a map for associating the unique (file-based) MC IDs with their corresponding (vertex_id, mcp_idLocal) pairs.
    // This is needed to find the unique ID for parent MC particles, where we only know their vertex_id & mcp_idLocal's.
    // The "mcp_mother" local ID doesn't have a corresponding unique "mcp_file_mother" stored in the input ROOT file.
    // We need to do this before creating the MC particles since the ancestry could be stored in any order
    std::map<std::pair<long, long>, long> mcIDMap;

    for (size_t i = 0; i < larspmc->m_mcp_id->size(); ++i)
    {
        // Store the corresponding unique (file-based) MC ID for each <vertex_id, mcp_idLocal> key pair
        const long mcpID = (*larspmc->m_mcp_id)[i];
        const long mcpVertexID = (*larspmc->m_mcp_vertex_id)[i];
        const long mcpIDLocal = (*larspmc->m_mcp_idLocal)[i];
        mcIDMap[std::make_pair(mcpVertexID, mcpIDLocal)] = mcpID;
    }

    // Create MC particles
    for (size_t i = 0; i < larspmc->m_mcp_id->size(); ++i)
    {
        // LArMCParticle parameters
        lar_content::LArMCParticleParameters mcParticleParameters;

        // Initial momentum and energy in GeV
        const float px = (*larspmc->m_mcp_px)[i] * m_parameters.m_energyScale;
        const float py = (*larspmc->m_mcp_py)[i] * m_parameters.m_energyScale;
        const float pz = (*larspmc->m_mcp_pz)[i] * m_parameters.m_energyScale;
        const float energy = (*larspmc->m_mcp_energy)[i] * m_parameters.m_energyScale;
        mcParticleParameters.m_energy = energy;
        mcParticleParameters.m_momentum = pandora::CartesianVector(px, py, pz);

        // Particle codes
        mcParticleParameters.m_particleId = (*larspmc->m_mcp_pdg)[i];
        mcParticleParameters.m_mcParticleType = pandora::MC_3D;

        // Neutrino info
        const long mcpVertexID = (*larspmc->m_mcp_vertex_id)[i];
        const int nuIndex = vertexIdToIndex[mcpVertexID];
        const std::string reaction = this->GetNuanceReaction((*larspmc->m_ccnc)[nuIndex], (*larspmc->m_mode)[nuIndex]);
        mcParticleParameters.m_nuanceCode = this->GetNuanceCode(reaction);

        // Unique file-based ID for this MC particle
        const long mcpID = (*larspmc->m_mcp_id)[i];
        mcParticleParameters.m_pParentAddress = (void *)((intptr_t)mcpID);

        // Start and end points in cm
        const float startx = (*larspmc->m_mcp_startx)[i] * m_parameters.m_lengthScale;
        const float starty = (*larspmc->m_mcp_starty)[i] * m_parameters.m_lengthScale;
        const float startz = (*larspmc->m_mcp_startz)[i] * m_parameters.m_lengthScale;
        mcParticleParameters.m_vertex = pandora::CartesianVector(startx, starty, startz);

        const float endx = (*larspmc->m_mcp_endx)[i] * m_parameters.m_lengthScale;
        const float endy = (*larspmc->m_mcp_endy)[i] * m_parameters.m_lengthScale;
        const float endz = (*larspmc->m_mcp_endz)[i] * m_parameters.m_lengthScale;
        mcParticleParameters.m_endpoint = pandora::CartesianVector(endx, endy, endz);

        // Process ID
        mcParticleParameters.m_process = lar_content::MC_PROC_UNKNOWN;

        // Create MCParticle
        try
        {
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::MCParticle::Create(*m_pPandora, mcParticleParameters, mcParticleFactory));
        }
        catch (const pandora::StatusCodeException &)
        {
            std::cout << "Unable to create MCParticle " << i << " : invalid info supplied, e.g. non-unique trackID or NaNs" << std::endl;
            continue;
        }

        // Set parent relationship. For the parent, use its <vertex_id, mcp_idLocal> pair to get its unique ID
        const long mcpMotherID = (*larspmc->m_mcp_mother)[i];
        const std::pair<long, long> parentPair = std::make_pair(mcpVertexID, mcpMotherID);
        const long mcpParentID = (mcIDMap.find(parentPair) != mcIDMap.end()) ? mcIDMap.at(parentPair) : mcpMotherID;

        if (mcpParentID == -1) // link to mc neutrino
        {
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                PandoraApi::SetMCParentDaughterRelationship(*m_pPandora, (void *)((intptr_t)mcpVertexID), (void *)((intptr_t)mcpID)));
        }
        else
        {
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                PandoraApi::SetMCParentDaughterRelationship(*m_pPandora, (void *)((intptr_t)mcpParentID), (void *)((intptr_t)mcpID)));
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

int LArNDEventInput::CreateCaloHits(int eventIndex, int startHitIndex) const
{
    std::cout << "LArNDEventInput::CreateCaloHits eventIndex = " << eventIndex << std::endl;

    m_larsp->GetEntry(eventIndex);

    int hitCounter{startHitIndex};
    std::cout << "LArNDEventInput: startHitIndex = " << startHitIndex << std::endl;

    const int nSP = m_larsp->m_x->size();

    // Stop processing the event if we have too many hits
    if (m_parameters.m_maxNHits > 0 && nSP > m_parameters.m_maxNHits)
    {
        std::cout << "SKIPPING EVENT: number of space points " << nSP << " > " << m_parameters.m_maxNHits << std::endl;
        return hitCounter;
    }

    // Also stop processing the event if it has too few hits (we can't make CaloHits for essentially empty events)
    if (nSP < m_parameters.m_minNHits)
    {
        std::cout << "SKIPPING EVENT: number of space points " << nSP << " < " << m_parameters.m_minNHits << std::endl;
        return hitCounter;
    }

    // Loop over the space points and make them into caloHits
    for (size_t isp = 0; isp < nSP; ++isp)
    {
        const float voxelX = (*m_larsp->m_x)[isp] * m_parameters.m_lengthScale;
        const float voxelY = (*m_larsp->m_y)[isp] * m_parameters.m_lengthScale;
        const float voxelZ = (*m_larsp->m_z)[isp] * m_parameters.m_lengthScale;
        const float voxelE = (*m_larsp->m_charge)[isp] * m_parameters.m_energyScale;

        // Skip this hit if its coordinates or energy are NaNs
        if (std::isnan(voxelX) || std::isnan(voxelY) || std::isnan(voxelZ) || std::isnan(voxelE))
        {
            std::cout << "Ignoring hit " << isp << " which contains NaNs: (" << voxelX << ", " << voxelY << ", " << voxelZ
                      << "), E = " << voxelE << std::endl;
            continue;
        }

        const pandora::CartesianVector voxelPos(voxelX, voxelY, voxelZ);
        const float MipE{0.00075};
        const float voxelMipEquivalentE = voxelE / MipE;
        const int tpcID(m_tpcGeom.GetTPCNumber(voxelPos));
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

        // Only used for truth
        long trackID{0};
        float energyFrac{0.f};
        // Set calo hit to MCParticle relation using trackID
        if (m_parameters.m_dataFormat == NDParameters::DataFormat::SPMC)
        {
            LArSPMC *larspmc = dynamic_cast<LArSPMC *>(m_larsp.get());
            if (!larspmc)
                continue;

            const std::vector<float> mcContribs = (*larspmc->m_hit_packetFrac)[isp];
            const int biggestContribIndex = std::distance(mcContribs.begin(), std::max_element(mcContribs.begin(), mcContribs.end()));
            const std::vector<long> hitPartIDVect = (*larspmc->m_hit_particleID)[isp];
            trackID = (hitPartIDVect.size() > biggestContribIndex) ? hitPartIDVect[biggestContribIndex] : 0;

            // Due to the merging of hits, the contributions can sometimes add up to more than 1.
            // Normalise first
            const float sum = std::accumulate(mcContribs.begin(), mcContribs.end(), 0.f);
            energyFrac = (biggestContribIndex < mcContribs.size() && std::abs(sum) > 0.0) ? mcContribs[biggestContribIndex] / sum : 0.f;
            // Make sure the energy fraction is not larger than 1
            if (energyFrac > 1.f + std::numeric_limits<float>::epsilon())
                energyFrac = 1.f;

            if (std::find(larspmc->m_mcp_id->begin(), larspmc->m_mcp_id->end(), trackID) == larspmc->m_mcp_id->end())
                std::cout << "Problem? Could not find MC particle with file ID " << trackID << std::endl;
        }

        // Create CaloHit for the main Pandora instance and create a copy for this instance as well
        if (m_parameters.m_use3D)
        {
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitParameters, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitParameters, m_LArCaloHitFactory));
        }

        if (m_parameters.m_dataFormat == NDParameters::DataFormat::SPMC)
        {
            PandoraApi::SetCaloHitToMCParticleRelationship(*m_pMainPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
            PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
        }

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
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitPars_UView, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitPars_UView, m_LArCaloHitFactory));
            if (m_parameters.m_dataFormat == NDParameters::DataFormat::SPMC)
            {
                PandoraApi::SetCaloHitToMCParticleRelationship(*m_pMainPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
                PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
            }

            // V view
            lar_content::LArCaloHitParameters caloHitPars_VView(caloHitParameters);
            caloHitPars_VView.m_hitType = pandora::TPC_VIEW_V;
            caloHitPars_VView.m_pParentAddress = (void *)(intptr_t(++hitCounter));
            const float vpos_cm(m_pPandora->GetPlugins()->GetLArTransformationPlugin()->YZtoV(y0_cm, z0_cm));
            caloHitPars_VView.m_positionVector = pandora::CartesianVector(x0_cm, 0.f, vpos_cm);
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitPars_VView, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitPars_VView, m_LArCaloHitFactory));
            if (m_parameters.m_dataFormat == NDParameters::DataFormat::SPMC)
            {
                PandoraApi::SetCaloHitToMCParticleRelationship(*m_pMainPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
                PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
            }
            // W view
            lar_content::LArCaloHitParameters caloHitPars_WView(caloHitParameters);
            caloHitPars_WView.m_hitType = pandora::TPC_VIEW_W;
            caloHitPars_WView.m_pParentAddress = (void *)(intptr_t(++hitCounter));
            const float wpos_cm(m_pPandora->GetPlugins()->GetLArTransformationPlugin()->YZtoW(y0_cm, z0_cm));
            caloHitPars_WView.m_positionVector = pandora::CartesianVector(x0_cm, 0.f, wpos_cm);
            PANDORA_THROW_RESULT_IF(
                pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pMainPandora, caloHitPars_WView, m_LArCaloHitFactory));
            PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::CaloHit::Create(*m_pPandora, caloHitPars_WView, m_LArCaloHitFactory));
            if (m_parameters.m_dataFormat == NDParameters::DataFormat::SPMC)
            {
                PandoraApi::SetCaloHitToMCParticleRelationship(*m_pMainPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
                PandoraApi::SetCaloHitToMCParticleRelationship(*m_pPandora, (void *)((intptr_t)hitCounter), (void *)((intptr_t)trackID), energyFrac);
            }

        } // use LArTPC coords
    } // end space point loop

    std::cout << "startHitIndex = " << startHitIndex << ", hitCounter = " << hitCounter << std::endl;
    return hitCounter;
}

//------------------------------------------------------------------------------------------------------------------------------------------

int LArNDEventInput::CreateTracks(int, int) const
{
    // We don't create Track objects
    return 0;
}

} // namespace lar_nd_reco
