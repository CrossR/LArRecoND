/**
 *  @file   src/SlicingThreeDAlgorithm.cc
 *
 *  @brief  Implementation of the 3D slicing algorithm class.
 *
 *  $Log: $
 */

#include "Api/PandoraApi.h"

#include "Objects/MCParticle.h"
#include "Pandora/AlgorithmHeaders.h"

#include "Pandora/PandoraInternal.h"
#include "larpandoracontent/LArHelpers/LArGeometryHelper.h"
#include "larpandoracontent/LArHelpers/LArMCParticleHelper.h"
#include "larpandoracontent/LArObjects/LArCaloHit.h"
#include "larpandoracontent/LArObjects/LArMCParticle.h"

#include "EventSlicingThreeDTool.h"
#include "SlicingThreeDAlgorithm.h"

#include <numeric>

using namespace pandora;

namespace lar_content
{

SlicingThreeDAlgorithm::SlicingThreeDAlgorithm() : m_pEventSlicingTool(nullptr)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode SlicingThreeDAlgorithm::Run()
{
    Slice3DList sliceList;
    m_pEventSlicingTool->RunSlicing(this, m_caloHitListNames, m_clusterListNames, sliceList);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::RunDaughterAlgorithm(*this, m_slicingListDeletionAlgorithm));

    if (sliceList.empty())
        return STATUS_CODE_SUCCESS;

    std::string clusterListName;
    const ClusterList *pClusterList(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pClusterList, clusterListName));

    std::string pfoListName;
    const PfoList *pPfoList(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pPfoList, pfoListName));

    if (m_evaluateSlices)
        this->EvaluateSlices(sliceList);

    for (const Slice3D &slice : sliceList)
    {
        const Cluster *pClusterU(nullptr), *pClusterV(nullptr), *pClusterW(nullptr), *pCluster3D(nullptr);
        PandoraContentApi::Cluster::Parameters clusterParametersU, clusterParametersV, clusterParametersW, clusterParameters3D;
        clusterParametersU.m_caloHitList = slice.m_caloHitListU;
        clusterParametersV.m_caloHitList = slice.m_caloHitListV;
        clusterParametersW.m_caloHitList = slice.m_caloHitListW;
        clusterParameters3D.m_caloHitList = slice.m_caloHitList3D;
        if (!clusterParametersU.m_caloHitList.empty())
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, clusterParametersU, pClusterU));
        if (!clusterParametersV.m_caloHitList.empty())
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, clusterParametersV, pClusterV));
        if (!clusterParametersW.m_caloHitList.empty())
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, clusterParametersW, pClusterW));
        if (!clusterParameters3D.m_caloHitList.empty())
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, clusterParameters3D, pCluster3D));

        if (!pClusterU && !pClusterV && !pClusterW && !pCluster3D)
            throw StatusCodeException(STATUS_CODE_FAILURE);

        const Pfo *pSlicePfo(nullptr);
        PandoraContentApi::ParticleFlowObject::Parameters pfoParameters;
        if (pClusterU)
            pfoParameters.m_clusterList.push_back(pClusterU);
        if (pClusterV)
            pfoParameters.m_clusterList.push_back(pClusterV);
        if (pClusterW)
            pfoParameters.m_clusterList.push_back(pClusterW);
        if (pCluster3D)
            pfoParameters.m_clusterList.push_back(pCluster3D);
        pfoParameters.m_charge = 0;
        pfoParameters.m_energy = 0.f;
        pfoParameters.m_mass = 0.f;
        pfoParameters.m_momentum = CartesianVector(0.f, 0.f, 0.f);
        pfoParameters.m_particleId = 0;
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::Create(*this, pfoParameters, pSlicePfo));
    }

    if (!pClusterList->empty())
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<Cluster>(*this, m_sliceClusterListName));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<Cluster>(*this, m_sliceClusterListName));
    }

    if (!pPfoList->empty())
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<ParticleFlowObject>(*this, m_slicePfoListName));
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<ParticleFlowObject>(*this, m_slicePfoListName));
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void SlicingThreeDAlgorithm::EvaluateSlices(const Slice3DList &sliceList)
{
    if (sliceList.empty())
    {
        std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: No slices to evaluate" << std::endl;
        return;
    }

    // Get the current MC particle list
    const MCParticleList *pMCParticleList(nullptr);
    if (PandoraContentApi::GetCurrentList(*this, pMCParticleList) != STATUS_CODE_SUCCESS)
    {
        std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Failed to get current MC particle list" << std::endl;
        return;
    }

    if (!pMCParticleList || pMCParticleList->empty())
    {
        std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Failed to get current MC particle list or it is empty" << std::endl;
        return;
    }

    // Get the current calo hit list
    const CaloHitList *pCaloHitList(nullptr);
    if (PandoraContentApi::GetCurrentList(*this, pCaloHitList, m_caloHitListNames[TPC_3D]) != STATUS_CODE_SUCCESS)
    {
        std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Failed to get current calo hit list" << std::endl;
        return;
    }

    if (!pCaloHitList || pCaloHitList->empty())
    {
        std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Failed to get current calo hit list or it is empty" << std::endl;
        return;
    }

    // List of all found true neutrinos.
    // Split into in detector and rock muons.
    std::set<const MCParticle *> neutrinoSet;
    std::set<const MCParticle *> rockMuonSet;

    // Geometry boundaries, used for rock muon identification.
    const auto detectorBoundaries(LArGeometryHelper::GetDetectorBoundaries(this->GetPandora()));

    // Mapping from CaloHit to its corresponding parent Neutrino, and back.
    LArMCParticleHelper::MCContributionMap nuToCaloHitMap;
    std::map<const CaloHit *, const MCParticle *> caloHitToNuMap;

    // Build up an MC map between each hit and its corresponding primary neutrino.
    for (const CaloHit *pCaloHit : *pCaloHitList)
    {
        const LArCaloHit *const pLArCaloHit(dynamic_cast<const LArCaloHit *>(pCaloHit));
        const auto mcWeights(pLArCaloHit->GetMCParticleWeightMap());

        const MCParticle *largestContributor(nullptr);
        float largestWeight(-1.f);

        for (const auto &mcWeight : mcWeights)
        {
            const MCParticle *mc{mcWeight.first};
            const auto parent(LArMCParticleHelper::GetParentMCParticle(mc));

            if (mcWeight.second > largestWeight)
                largestContributor = parent;
        }

        if (LArMCParticleHelper::IsNeutrino(largestContributor))
        {
            const auto vertex(largestContributor->GetVertex());
            if (LArGeometryHelper::IsInDetector(detectorBoundaries, vertex))
            {
                neutrinoSet.insert(largestContributor);
            }
            else
            {
                rockMuonSet.insert(largestContributor);
            }

            nuToCaloHitMap[largestContributor].push_back(pCaloHit);
            caloHitToNuMap[pCaloHit] = largestContributor;
        }
        else
        {
            std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Found a CaloHit with no neutrino parent" << std::endl;
        }
    }

    std::vector<float> purity;
    std::vector<float> completeness;
    std::vector<bool> isRockMuon;
    std::map<const MCParticle *, int> mcParticleToSliceMap;
    unsigned int sliceIndex = 0;

    // Now, we can loop through every slice, and evaluate it.
    for (const Slice3D &slice : sliceList)
    {
        const auto hits(slice.m_caloHitList3D);

        // Lets find the biggest neutrino contribution to this slice.
        // The nu that contributed the most is the target and will be used to evaluate the slice.
        std::map<const MCParticle *, float> nuContributionMap;
        for (const CaloHit *pCaloHit : hits)
        {
            const auto it(caloHitToNuMap.find(pCaloHit));
            if (it == caloHitToNuMap.end())
            {
                std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: CaloHit not found in caloHitToNuMap" << std::endl;
                continue;
            }

            nuContributionMap[it->second] += pCaloHit->GetInputEnergy();
        }

        // Find the neutrino with the largest contribution.
        const auto maxNuIt(std::max_element(nuContributionMap.begin(), nuContributionMap.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; }));
        const MCParticle* maxNu = maxNuIt->first;

        unsigned int nHitsInSlice = hits.size();
        unsigned int trueNuHits = nuToCaloHitMap[maxNu].size();
        unsigned int matchedHits = 0;
        mcParticleToSliceMap[maxNu] = sliceIndex;

        // Count the number of hits that match the neutrino or don't.
        for (const CaloHit *pCaloHit : hits)
        {
            const auto it(caloHitToNuMap.find(pCaloHit));
            if (it == caloHitToNuMap.end())
            {
                std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: CaloHit not found in caloHitToNuMap" << std::endl;
                continue;
            }

            if (it->second == maxNu)
                matchedHits++;
        }

        // Finally, calculate and store the completeness and purity of the slice.
        purity.push_back(matchedHits / static_cast<float>(nHitsInSlice));
        completeness.push_back(matchedHits / static_cast<float>(trueNuHits));
        isRockMuon.push_back(rockMuonSet.find(maxNu) != rockMuonSet.end());
    }

    // Output the results.
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Evaluated " << sliceList.size() << " slices." << std::endl;
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Found " << neutrinoSet.size() << " neutrinos in the detector." << std::endl;
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Found " << rockMuonSet.size() << " rock muons." << std::endl;
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Completeness and purity of neutrino-majority slices:" << std::endl;

    float nuTotalCompleteness = 0.0, nuTotalPurity = 0.0, nuCount = 0.0;
    float rockMuonTotalCompleteness = 0.0, rockMuonTotalPurity = 0.0, rockMuonCount = 0.0;

    for (size_t i = 0; i < sliceList.size(); ++i)
    {
        if (isRockMuon[i])
            continue;

        std::cout << "Slice: " << i << " | ";
        std::cout << "Size 3D: " << sliceList[i].m_caloHitList3D.size() << " | ";
        std::cout << "Completeness: " << completeness[i] << " | ";
        std::cout << "Purity: " << purity[i] << " | ";
        std::cout << "Rock Muon: " << (isRockMuon[i] ? "Yes" : "No") << std::endl;

        nuTotalCompleteness += completeness[i];
        nuTotalPurity += purity[i];
        nuCount += 1.0f;
    }
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Average nu-majority completeness: " << (nuCount > 0 ? nuTotalCompleteness / nuCount : 0) << std::endl;
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Average nu-majority purity: " << (nuCount > 0 ? nuTotalPurity / nuCount : 0) << std::endl;

    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Completeness and purity of rock muon-majority slices:" << std::endl;
    for (size_t i = 0; i < sliceList.size(); ++i)
    {
        if (!isRockMuon[i])
            continue;

        std::cout << "Slice: " << i << " | ";
        std::cout << "Size 3D: " << sliceList[i].m_caloHitList3D.size() << " | ";
        std::cout << "Completeness: " << completeness[i] << " | ";
        std::cout << "Purity: " << purity[i] << " | ";
        std::cout << "Rock Muon: " << (isRockMuon[i] ? "Yes" : "No") << std::endl;

        rockMuonTotalCompleteness += completeness[i];
        rockMuonTotalPurity += purity[i];
        rockMuonCount += 1.0f;
    }
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Average rock muon-majority completeness: "
              << (rockMuonCount > 0 ? rockMuonTotalCompleteness / rockMuonCount : 0) << std::endl;
    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Average rock muon-majority purity: "
              << (rockMuonCount > 0 ? rockMuonTotalPurity / rockMuonCount : 0) << std::endl;

    std::cout << "Neutrino that were not the majority in any slice:" << std::endl;
    for (const auto nu : neutrinoSet)
    {
        if (mcParticleToSliceMap.find(nu) != mcParticleToSliceMap.end())
            continue;

        std::cout << "Neutrino: " << nu->GetParticleId() << " | ";
        std::cout << "Energy: " << nu->GetEnergy() << " | ";
        std::cout << "Vertex: (" << nu->GetVertex().GetX() << ", "
                                 << nu->GetVertex().GetY() << ", "
                                 << nu->GetVertex().  GetZ() << ") | ";
        std::cout << "Num Hits: " << nuToCaloHitMap[nu].size() << std::endl;

    }

    std::cout << "Rock muon that were not the majority in any slice:" << std::endl;
    for (const auto rockMuon : rockMuonSet)
    {
        if (mcParticleToSliceMap.find(rockMuon) != mcParticleToSliceMap.end())
            continue;

        std::cout << "Rock Muon: " << rockMuon->GetParticleId() << " | ";
        std::cout << "Energy: " << rockMuon->GetEnergy() << " | ";
        std::cout << "Vertex: (" << rockMuon->GetVertex().GetX() << ", "
                                   << rockMuon->GetVertex().GetY() << ", "
                                   << rockMuon->GetVertex().  GetZ() << ") | ";
        std::cout << "Num Hits: " << nuToCaloHitMap[rockMuon].size() << std::endl;
    }

    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Done." << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode SlicingThreeDAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    AlgorithmTool *pAlgorithmTool(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithmTool(*this, xmlHandle, "SliceCreation", pAlgorithmTool));
    m_pEventSlicingTool = dynamic_cast<EventSlicingThreeDTool *>(pAlgorithmTool);

    if (!m_pEventSlicingTool)
        return STATUS_CODE_INVALID_PARAMETER;

    PANDORA_RETURN_RESULT_IF(
        STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithm(*this, xmlHandle, "SlicingListDeletion", m_slicingListDeletionAlgorithm));

    std::string caloHitListNameU, caloHitListNameV, caloHitListNameW, caloHitListName3D;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListNameU", caloHitListNameU));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListNameV", caloHitListNameV));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListNameW", caloHitListNameW));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName3D", caloHitListName3D));
    m_caloHitListNames[TPC_VIEW_U] = caloHitListNameU;
    m_caloHitListNames[TPC_VIEW_V] = caloHitListNameV;
    m_caloHitListNames[TPC_VIEW_W] = caloHitListNameW;
    m_caloHitListNames[TPC_3D] = caloHitListName3D;

    std::string clusterListNameU, clusterListNameV, clusterListNameW;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameU", clusterListNameU));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameV", clusterListNameV));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameW", clusterListNameW));
    m_clusterListNames[TPC_VIEW_U] = clusterListNameU;
    m_clusterListNames[TPC_VIEW_V] = clusterListNameV;
    m_clusterListNames[TPC_VIEW_W] = clusterListNameW;

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputClusterListName", m_sliceClusterListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputPfoListName", m_slicePfoListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "EvaluateSlices", m_evaluateSlices));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
