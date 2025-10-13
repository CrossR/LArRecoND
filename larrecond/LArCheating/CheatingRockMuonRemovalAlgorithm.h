/**
 *  @file   larrecond/LArCheating/CheatingRockMuonRemovalAlgorithm.h
 *
 *  @brief  Header file for the cheating rock muon removal algorithm class.
 *
 *  $Log: $
 */
#ifndef LAR_CHEATING_ROCK_MUON_REMOVAL_ALGORITHM_H
#define LAR_CHEATING_ROCK_MUON_REMOVAL_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

#include <unordered_map>

namespace lar_content
{

/**
 *  @brief  CheatingRockMuonRemovalAlgorithm class
 */
class CheatingRockMuonRemovalAlgorithm : public pandora::Algorithm
{
private:
    pandora::StatusCode Run();

    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string m_inputCaloHitListName; ///< Name of the calo hit list to load
    std::string m_rockMuonCaloHitListName; ///< Name of the rock muon calo hit list to create
    std::string m_neutrinoCaloHitListName; ///< Name of the neutrino calo hit list to create
    std::string m_inputMCParticleListName; ///< Name of the MC particle list to use
};

} // namespace lar_content

#endif // #ifndef LAR_CHEATING_ROCK_MUON_REMOVAL_ALGORITHM_H
