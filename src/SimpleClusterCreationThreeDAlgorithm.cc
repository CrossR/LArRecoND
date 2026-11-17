/**
 *  @file   src/SimpleClusterCreationThreeDAlgorithm.cc
 *
 *  @brief  Implementation of the 3D cluster creation algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "SimpleClusterCreationThreeDAlgorithm.h"

using namespace pandora;

namespace lar_content
{

SimpleClusterCreationThreeDAlgorithm::SimpleClusterCreationThreeDAlgorithm()
{
    m_clusteringWindowSquared = 0.25f;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void SimpleClusterCreationThreeDAlgorithm::BuildAssociationMap(const CaloHitVector &caloHitVector, HitAssociationMap &hitAssociationMap) const
{
    this->BuildAssociationMapKDTree<3>(caloHitVector, hitAssociationMap);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode SimpleClusterCreationThreeDAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName3D", m_inputCaloHitListName));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputClusterListName3D", m_outputClusterListName));

    return SimpleClusterCreationAlgorithm::ReadSettings(xmlHandle);
}

} // namespace lar_content
