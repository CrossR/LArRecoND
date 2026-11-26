/**
 *  @file   include/CreateTwoDClustersFromThreeDAlgorithm.h
 *
 *  @brief  Header file for the 3D to 2D cluster creation alg
 *
 *  $Log: $
 */
#ifndef LAR_CREATE_TWO_D_CLUSTERS_FROM_THREE_D_ALGORITHM_H
#define LAR_CREATE_TWO_D_CLUSTERS_FROM_THREE_D_ALGORITHM_H 1

#include "Api/PandoraContentApi.h"
#include "Pandora/Algorithm.h"

#include <unordered_map>

namespace lar_content
{

/**
 *  @brief  CreateTwoDClustersFromThreeDAlgorithm class
 */
class CreateTwoDClustersFromThreeDAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    CreateTwoDClustersFromThreeDAlgorithm();

private:
    pandora::StatusCode Run();

    typedef std::map<pandora::HitType, std::string> HitTypeToNameMap;

    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    HitTypeToNameMap m_inputCaloHitListNames2D; ///< Names of the input 2D hit lists
    std::string m_inputClusterListName3D;       ///< Name of the input 3D cluster list
    HitTypeToNameMap m_outputClusterListNames;  ///< Names of the output 2D cluster lists
};

} // namespace lar_content

#endif // #ifndef LAR_CREATE_TWO_D_CLUSTERS_FROM_THREE_D_ALGORITHM_H
