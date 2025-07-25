/**
 *  @file   include/CheatingRockMuonRemovalAlgorithm.h
 *
 *  @brief  Header file for the cheating rock muon removal algorithm
 *
 *  $Log: $
 */
#ifndef LAR_CHEATING_ROCK_MUON_REMOVAL_ALGORITHM_H
#define LAR_CHEATING_ROCK_MUON_REMOVAL_ALGORITHM_H 1

#include "larpandoracontent/LArObjects/LArThreeDSlidingFitResult.h"

#include "Pandora/Algorithm.h"

#include <unordered_map>

namespace lar_content
{

/**
 *  @brief  CheatingRockMuonRemovalAlgorithm class
 */
class CheatingRockMuonRemoval : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    CheatingRockMuonRemoval();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);


    std::string m_inputCaloHitListName3D;  ///< The name of the input calo hit list
};

} // namespace lar_content

#endif // #ifndef LAR_CHEATING_ROCK_MUON_REMOVAL_ALGORITHM_H
