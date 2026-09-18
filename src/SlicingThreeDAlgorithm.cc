/**
 *  @file   src/SlicingThreeDAlgorithm.cc
 *
 *  @brief  Implementation of the 3D slicing algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArGeometryHelper.h"
#include "larpandoracontent/LArHelpers/LArMCParticleHelper.h"
#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include "SlicingThreeDAlgorithm.h"

#include <chrono>

using namespace pandora;

namespace lar_content
{

SlicingThreeDAlgorithm::SlicingThreeDAlgorithm() :
    m_pEventSlicingTool(nullptr)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

SlicingThreeDAlgorithm::~SlicingThreeDAlgorithm()
{
    if (m_evaluateSlices)
        PANDORA_MONITORING_API(SaveTree(this->GetPandora(), m_analysisTreeName.c_str(), m_analysisFileName.c_str(), "UPDATE"));
}

//------------------------------------------------------------------------------------------------------------------------------------------

const Vertex *CreateVertexCopy(const Algorithm &algorithm, const Vertex *const pInputVertex)
{
    PandoraContentApi::Vertex::Parameters vertexParameters;
    vertexParameters.m_position = pInputVertex->GetPosition();
    vertexParameters.m_vertexLabel = pInputVertex->GetVertexLabel();
    vertexParameters.m_vertexType = pInputVertex->GetVertexType();

    const Vertex *pNewVertex(nullptr);
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Vertex::Create(algorithm, vertexParameters, pNewVertex));

    return pNewVertex;
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

    std::string vertexListName;
    const VertexList *pVertexList(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pVertexList, vertexListName));

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

        for (const auto &vertex : slice.m_vertexList)
            pfoParameters.m_vertexList.push_back(CreateVertexCopy(*this, vertex));

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

    if (!pVertexList->empty())
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<Vertex>(*this, m_slicePfoListName + "Vertices"));

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
    if (PandoraContentApi::GetList(*this, m_caloHitListNames[TPC_3D], pCaloHitList) != STATUS_CODE_SUCCESS)
    {
        std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Failed to get current calo hit list" << std::endl;
        return;
    }

    if (!pCaloHitList || pCaloHitList->empty())
    {
        std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Failed to get current calo hit list or it is empty" << std::endl;
        return;
    }

    // Pull out event info...
    const unsigned int runNum(this->GetPandora().GetRun());
    const unsigned int subrunNum(this->GetPandora().GetSubrun());
    const unsigned int eventNum(this->GetPandora().GetEvent());

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

        try
        {
            if (!LArMCParticleHelper::IsNeutrino(largestContributor))
                continue;
        }
        catch (const StatusCodeException &e)
        {
            continue;
        }

        const auto vertex(largestContributor->GetVertex());
        if (LArGeometryHelper::IsInDetector(detectorBoundaries, vertex))
            neutrinoSet.insert(largestContributor);
        else
            rockMuonSet.insert(largestContributor);

        nuToCaloHitMap[largestContributor].push_back(pCaloHit);
        caloHitToNuMap[pCaloHit] = largestContributor;
    }

    // Create a super set of all neutrinos, so we can loop over them later.
    std::set<const MCParticle *> allNeutrinos;
    allNeutrinos.insert(neutrinoSet.begin(), neutrinoSet.end());
    allNeutrinos.insert(rockMuonSet.begin(), rockMuonSet.end());

    // Create a relational map to sync the two trees
    std::map<const MCParticle *, int> nuToIdMap;
    int currentNuId = 0;
    for (const MCParticle *nu : allNeutrinos)
        nuToIdMap[nu] = currentNuId++;

    int sliceIndex = 0;

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
                continue;

            nuContributionMap[it->second] += pCaloHit->GetInputEnergy();
        }

        // Find the neutrino with the largest contribution.
        const auto maxNuIt(std::max_element(
            nuContributionMap.begin(), nuContributionMap.end(), [](const auto &a, const auto &b) { return a.second < b.second; }));
        const MCParticle *maxNu = maxNuIt->first;

        // Now, we know the main neutrino for this slice.
        // Lets loop over every MC particle that contributed to this slice, and store every individual
        // calculation of completeness and purity.
        // Finally, we can store a "This slice is dominated by this neutrino" flag, to get
        // the completeness and purity of the main contributor, whilst also storing the completeness and purity
        // of every neutrino that contributed to this slice.
        std::vector<int> eventNumSlice, subrunNumSlice, runNumSlice;
        std::vector<int> sliceIndexSlice, targetNuIdSlice, nuPdgSlice;
        std::vector<float> slicingDurationSlice;
        std::vector<float> puritySlice, completenessSlice, isRockMuonSlice, isMainNuSlice;
        std::vector<float> trueNuSize, trueNuEnergy, sliceSize, sliceMatchedHits, sliceMissedHits;

        for (const auto &nuContribution : nuContributionMap)
        {
            const MCParticle *nu = nuContribution.first;
            const bool isMainNu(nu == maxNu);

            unsigned int nHitsInSlice = hits.size();
            unsigned int trueNuHits = nuToCaloHitMap[nu].size();
            unsigned int matchedHits = 0;
            unsigned int missedHits = 0;

            // Count the number of hits that match the neutrino or don't.
            for (const CaloHit *pCaloHit : hits)
            {
                const auto it(caloHitToNuMap.find(pCaloHit));
                if (it == caloHitToNuMap.end())
                    continue;

                if (it->second == nu)
                    matchedHits++;
                else
                    missedHits++;
            }

            // Finally, calculate and store the completeness and purity of the slice.
            puritySlice.push_back(matchedHits / static_cast<float>(nHitsInSlice));
            completenessSlice.push_back(matchedHits / static_cast<float>(trueNuHits));
            isRockMuonSlice.push_back(rockMuonSet.find(maxNu) != rockMuonSet.end());
            isMainNuSlice.push_back(isMainNu);

            // Store some higher level information about the slice + MC.
            runNumSlice.push_back(runNum);
            subrunNumSlice.push_back(subrunNum);
            eventNumSlice.push_back(eventNum);
            sliceIndexSlice.push_back(sliceIndex);
            trueNuSize.push_back(trueNuHits);
            trueNuEnergy.push_back(nu->GetEnergy());
            sliceSize.push_back(nHitsInSlice);
            sliceMatchedHits.push_back(matchedHits);
            sliceMissedHits.push_back(missedHits);
            targetNuIdSlice.push_back(nuToIdMap[nu]);
            nuPdgSlice.push_back(nu->GetParticleId());
        }

        // Add the results to a ROOT file.
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "run", &runNumSlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "subrun", &subrunNumSlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "event", &eventNumSlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "sliceIndex", &sliceIndexSlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "slicingDuration", &slicingDurationSlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "completeness", &completenessSlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "purity", &puritySlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "isRockMuon", &isRockMuonSlice));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "isMainNu", &isMainNuSlice));

        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "trueNuSize", &trueNuSize));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "trueNuEnergy", &trueNuEnergy));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "sliceSize", &sliceSize));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "sliceMatchedHits", &sliceMatchedHits));
        PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), m_analysisTreeName.c_str(), "sliceMissedHits", &sliceMissedHits));

        PANDORA_MONITORING_API(FillTree(this->GetPandora(), m_analysisTreeName.c_str()));

        // Finally, increment the slice index for the next slice.
        sliceIndex++;
    }

    std::cout << "SlicingThreeDAlgorithm::EvaluateSlices: Evaluated " << sliceIndex << " reco slices for this event" << std::endl;

    // Now...lets inverse. Lets loop over every neutrino, and evaluate the slices that it contributed to.
    // First, get every MC bit that contributed to this event, and store it in a set.
    std::vector<int> eventNumTruth, subrunNumTruth, runNumTruth;
    std::vector<float> truthCompleteness, truthPurity, nuEnergyTruth;
    std::vector<int> isRockMuonTruth, trueNuSizeTruth, nuIdTruth, bestSliceIndexTruth, matchedHitsTruth, bestSliceSizeTruth, nuPdgTruth;

    for (const MCParticle *nu : allNeutrinos)
    {
        const unsigned int nHitsInNu(nuToCaloHitMap[nu].size());
        if (nHitsInNu == 0)
            continue;

        unsigned int maxHitsFromNuInAnySlice(0);
        int bestSliceIdx(-1);
        unsigned int bestSliceTotalHits(0);

        int currentSliceIdx = 0;
        for (const Slice3D &slice : sliceList)
        {
            unsigned int hitsFromNuInThisSlice(0);
            for (const CaloHit *pCaloHit : slice.m_caloHitList3D)
            {
                const auto it(caloHitToNuMap.find(pCaloHit));
                if ((it != caloHitToNuMap.end()) && (it->second == nu))
                    hitsFromNuInThisSlice++;
            }

            if (hitsFromNuInThisSlice > maxHitsFromNuInAnySlice)
            {
                maxHitsFromNuInAnySlice = hitsFromNuInThisSlice;
                bestSliceIdx = currentSliceIdx;
                bestSliceTotalHits = slice.m_caloHitList3D.size();
            }
            currentSliceIdx++;
        }

        float completeness(0.f);
        float purity(0.f);

        if (bestSliceIdx != -1)
        {
            completeness = static_cast<float>(maxHitsFromNuInAnySlice) / static_cast<float>(nHitsInNu);
            purity = static_cast<float>(maxHitsFromNuInAnySlice) / static_cast<float>(bestSliceTotalHits);
        }

        runNumTruth.push_back(runNum);
        subrunNumTruth.push_back(subrunNum);
        eventNumTruth.push_back(eventNum);

        nuIdTruth.push_back(nuToIdMap[nu]);
        nuEnergyTruth.push_back(nu->GetEnergy());
        nuPdgTruth.push_back(nu->GetParticleId());
        isRockMuonTruth.push_back(rockMuonSet.find(nu) != rockMuonSet.end());
        trueNuSizeTruth.push_back(nHitsInNu);

        bestSliceIndexTruth.push_back(bestSliceIdx);
        bestSliceSizeTruth.push_back(bestSliceTotalHits);
        matchedHitsTruth.push_back(maxHitsFromNuInAnySlice);

        truthCompleteness.push_back(completeness);
        truthPurity.push_back(purity);
    }

    // Save to a truth-first tree.
    std::string truthTreeName = m_analysisTreeName + "_TruthFirst";
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "run", &runNumTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "subrun", &subrunNumTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "event", &eventNumTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "completeness", &truthCompleteness));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "purity", &truthPurity));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "isRockMuon", &isRockMuonTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "trueNuSize", &trueNuSizeTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "nuPdg", &nuPdgTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "nuId", &nuIdTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "nuEnergy", &nuEnergyTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "bestSliceIndex", &bestSliceIndexTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "bestSliceSize", &bestSliceSizeTruth));
    PANDORA_MONITORING_API(SetTreeVariable(this->GetPandora(), truthTreeName.c_str(), "matchedHits", &matchedHitsTruth));
    PANDORA_MONITORING_API(FillTree(this->GetPandora(), truthTreeName.c_str()));
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode SlicingThreeDAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    AlgorithmTool *pAlgorithmTool(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ProcessAlgorithmTool(*this, xmlHandle, "SliceCreation", pAlgorithmTool));
    m_pEventSlicingTool = dynamic_cast<EventSlicingThreeDBaseTool *>(pAlgorithmTool);

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

    std::string clusterListNameU, clusterListNameV, clusterListNameW, clusterListName3D;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameU", clusterListNameU));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameV", clusterListNameV));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameW", clusterListNameW));
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListName3D", clusterListName3D));
    m_clusterListNames[TPC_VIEW_U] = clusterListNameU;
    m_clusterListNames[TPC_VIEW_V] = clusterListNameV;
    m_clusterListNames[TPC_VIEW_W] = clusterListNameW;

    if (!clusterListName3D.empty())
        m_clusterListNames[TPC_3D] = clusterListName3D;

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputClusterListName", m_sliceClusterListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputPfoListName", m_slicePfoListName));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "EvaluateSlices", m_evaluateSlices));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
