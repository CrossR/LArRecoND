/**
 *  @file   src/CreateTwoDClustersFromThreeDAlgorithm.cc
 *
 *  @brief  Implementation of the 3D to 2D cluster creation algorithm.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArClusterHelper.h"

#include "CreateTwoDClustersFromThreeDAlgorithm.h"
#include "LArThreeDHitHelper.h"

using namespace pandora;

namespace lar_content
{

CreateTwoDClustersFromThreeDAlgorithm::CreateTwoDClustersFromThreeDAlgorithm()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

inline std::pair<int, int> QuantizePosition(const float x, const float wirePos)
{
    // INFO: Quantize the position, to avoid floating point precision issues.
    //       Scale value was chosen after testing, and values of 1e5 and above
    //       were found to be sufficient.
    constexpr float scale{1e5};
    return std::make_pair(static_cast<int>(x * scale), static_cast<int>(wirePos * scale));
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CreateTwoDClustersFromThreeDAlgorithm::Run()
{
    const ClusterList *pClusterList3D{nullptr};
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputClusterListName3D, pClusterList3D));

    const CaloHitList *pCaloHitListU{nullptr};
    PandoraContentApi::GetList(*this, m_inputCaloHitListNames2D.at(TPC_VIEW_U), pCaloHitListU);
    const CaloHitList *pCaloHitListV{nullptr};
    PandoraContentApi::GetList(*this, m_inputCaloHitListNames2D.at(TPC_VIEW_V), pCaloHitListV);
    const CaloHitList *pCaloHitListW{nullptr};
    PandoraContentApi::GetList(*this, m_inputCaloHitListNames2D.at(TPC_VIEW_W), pCaloHitListW);
    const CaloHitList *pCaloHitList3D{nullptr};
    PandoraContentApi::GetList(*this, m_inputCaloHitListNames2D.at(TPC_3D), pCaloHitList3D);

    const auto isEmpty = [](const CaloHitList *pList) { return !pList || pList->empty(); };

    if (isEmpty(pCaloHitListU) || isEmpty(pCaloHitListV) || isEmpty(pCaloHitListW) || isEmpty(pCaloHitList3D))
        return STATUS_CODE_SUCCESS;

    std::vector<PandoraContentApi::Cluster::Parameters> clustersU;
    std::vector<PandoraContentApi::Cluster::Parameters> clustersV;
    std::vector<PandoraContentApi::Cluster::Parameters> clustersW;

    // Find the associated 2D hits for each 3D hit, to use in the creation of 2D clusters
    LArThreeDHitHelper::CaloHitMapping caloHitMapping;
    LArThreeDHitHelper::GetAllChild2DHits(pCaloHitList3D, pCaloHitListU, pCaloHitListV, pCaloHitListW, caloHitMapping);

    for (const Cluster *pCluster : *pClusterList3D)
    {
        const OrderedCaloHitList &clusterHits3DOrdered = pCluster->GetOrderedCaloHitList();
        CaloHitList clusterHits3D;
        clusterHits3DOrdered.FillCaloHitList(clusterHits3D);

        // Now we need to find the matching 2D hits and build the 2D clusters
        CaloHitList associatedHitsU, associatedHitsV, associatedHitsW;

        for (const CaloHit *pCaloHit3D : clusterHits3D)
        {
            const auto iter = caloHitMapping.find(pCaloHit3D);
            if (iter == caloHitMapping.end())
                continue;

            const auto &mappingEntry = iter->second;

            if (mappingEntry.contains(TPC_VIEW_U))
                associatedHitsU.emplace_back(mappingEntry.at(TPC_VIEW_U));
            if (mappingEntry.contains(TPC_VIEW_V))
                associatedHitsV.emplace_back(mappingEntry.at(TPC_VIEW_V));
            if (mappingEntry.contains(TPC_VIEW_W))
                associatedHitsW.emplace_back(mappingEntry.at(TPC_VIEW_W));
        }

        if (!associatedHitsU.empty())
        {
            PandoraContentApi::Cluster::Parameters parametersU;
            parametersU.m_caloHitList = std::move(associatedHitsU);
            clustersU.emplace_back(std::move(parametersU));
        }

        if (!associatedHitsV.empty())
        {
            PandoraContentApi::Cluster::Parameters parametersV;
            parametersV.m_caloHitList = std::move(associatedHitsV);
            clustersV.emplace_back(std::move(parametersV));
        }

        if (!associatedHitsW.empty())
        {
            PandoraContentApi::Cluster::Parameters parametersW;
            parametersW.m_caloHitList = std::move(associatedHitsW);
            clustersW.emplace_back(std::move(parametersW));
        }
    }

    // Create new clusters
    std::string tempClusterListName;

    // U View
    const ClusterList *pClusterListU{nullptr};
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pClusterListU, tempClusterListName));
    for (auto &parameters : clustersU)
    {
        const Cluster *pClusterU{nullptr};
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, parameters, pClusterU));
    }
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<Cluster>(*this, m_outputClusterListNames.at(TPC_VIEW_U)));

    // V View
    const ClusterList *pClusterListV{nullptr};
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pClusterListV, tempClusterListName));
    for (auto &parameters : clustersV)
    {
        const Cluster *pClusterV{nullptr};
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, parameters, pClusterV));
    }
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<Cluster>(*this, m_outputClusterListNames.at(TPC_VIEW_V)));

    // W View
    const ClusterList *pClusterListW{nullptr};
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pClusterListW, tempClusterListName));
    for (auto &parameters : clustersW)
    {
        const Cluster *pClusterW{nullptr};
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Cluster::Create(*this, parameters, pClusterW));
    }
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<Cluster>(*this, m_outputClusterListNames.at(TPC_VIEW_W)));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CreateTwoDClustersFromThreeDAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{

    std::string tempCaloHitName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName3D", tempCaloHitName));
    m_inputCaloHitListNames2D[TPC_3D] = tempCaloHitName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListNameU", tempCaloHitName));
    m_inputCaloHitListNames2D[TPC_VIEW_U] = tempCaloHitName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListNameV", tempCaloHitName));
    m_inputCaloHitListNames2D[TPC_VIEW_V] = tempCaloHitName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListNameW", tempCaloHitName));
    m_inputCaloHitListNames2D[TPC_VIEW_W] = tempCaloHitName;

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListName3D", m_inputClusterListName3D));

    std::string tempClusterName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputClusterListNameU", tempClusterName));
    m_outputClusterListNames[TPC_VIEW_U] = tempClusterName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputClusterListNameV", tempClusterName));
    m_outputClusterListNames[TPC_VIEW_V] = tempClusterName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputClusterListNameW", tempClusterName));
    m_outputClusterListNames[TPC_VIEW_W] = tempClusterName;

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
