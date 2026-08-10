/**
 *  @file   LArRecoND/include/NDEventInput.h
 *
 *  @brief  Header file for the ND event input
 *
 *  $Log: $
 */
#ifndef PANDORA_ND_EVENT_INPUT_H
#define PANDORA_ND_EVENT_INPUT_H 1

#include "Pandora/Pandora.h"
#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include "NDParameters.h"
#include "NDSimpleTPCGeom.h"

class TFile;
class TTree;

namespace lar_nd_reco
{

class NDEventInput
{
public:
    /**
     *  @brief  Default constructor
     */
    NDEventInput();

    /**
     *  @brief  Constructor
     *
     *  @param  pPandora the address of the Pandora instance
     *  @param  parameters the application parameters
     *  @param  tpcGeom the TPC geometry
     */
    NDEventInput(const pandora::Pandora *pPandora, const NDParameters &parameters, const NDSimpleTPCGeom &tpcGeom);

    /**
     *  @brief  Destructor
     */
    virtual ~NDEventInput();

    /**
     *  @brief  Create the MCParticles
     *
     *  @param  eventIndex the event number
     */
    virtual void CreateMCParticles(int eventIndex) const = 0;

    /**
     *  @brief  Create the CaloHits
     *
     *  @param  eventIndex the event number
     *  @param  startHitIndex the starting hit index
     *
     *  @return the last hit index
     */
    virtual int CreateCaloHits(int eventIndex, int startHitIndex = 0) const = 0;

    /**
     *  @brief  Create the Tracks
     *
     *  @param  eventIndex the event number
     *  @param  startTrackIndex the starting track index
     *
     *  @return the last track index
     */
    virtual int CreateTracks(int eventIndex, int startTrackIndex = 0) const = 0;

    /**
     *  @brief  Get the Pandora instance
     *
     *  @return the address of the Pandora instance
     */
    const pandora::Pandora *GetPandoraInstance() const
    {
        return m_pPandora;
    }

    /**
     *  @brief  Get the total number of event entries
     *
     *  @return the total number of events
     */
    int GetNEntries() const
    {
        return m_nEntries;
    }

protected:
    /**
     *  @brief  Initialization
     */
    void Initialize();

    /**
     *  @brief  Convert the GENIE neutrino reaction string to a Nuance-like integer code
     *
     *  @param  reaction The neutrino reaction string
     *
     *  @return The reaction integer code
     */
    int GetNuanceCode(const std::string &reaction) const;

    /**
     *  @brief  Get Nuance reaction string
     *
     *  @param  ccnc Integer specifying CCNC reaction
     *  @param  mode Integer specifying general physics mode
     *
     *  @return Name of the Nuance reaction
     */
    std::string GetNuanceReaction(const int ccnc, const int mode) const;

    const pandora::Pandora *m_pPandora;
    const pandora::Pandora *m_pMainPandora;
    NDParameters m_parameters;
    NDSimpleTPCGeom m_tpcGeom;
    TFile *m_pFile;
    TTree *m_pTree;
    int m_nEntries;
    lar_content::LArCaloHitFactory m_LArCaloHitFactory;
};

} // namespace lar_nd_reco

#endif
