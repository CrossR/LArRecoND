/**
 *  @file   LArRecoND/app/LArRecoNDApp.cxx
 *
 *  @brief  Implementation of the LArRecoND application
 *
 *  $Log: $
 */

#include "LArRecoNDApp.h"

#include "MainNDPandora.h"
#include "NDParameters.h"

#ifdef MONITORING
#include "TApplication.h"
#endif

#include "json/json.hpp"

#include <iostream>

using namespace pandora;
using namespace lar_nd_reco;
using nlohmann::json;

int main(int argc, char *argv[])
{
    int errorNo(0);

    const std::string configFileName{(argc > 1) ? argv[1] : "config/LArND_TMS.json"};
    std::cout << "JSON ConfigFileName = " << configFileName << std::endl;

    try
    {

#ifdef MONITORING
        TApplication *pTApplication = new TApplication("LArRecoNDApp", &argc, argv);
        pTApplication->SetReturnFromRun(kTRUE);
#endif

        json jsonConfig;
        std::ifstream inputStr{configFileName};
        inputStr >> jsonConfig;

        // Get the main Pandora instance
        const auto mainInfo = jsonConfig["Main"];
        std::cout << "mainInfo = " << mainInfo << std::endl;

        NDParameters mainParameters;
        mainParameters.m_settingsFile = mainInfo["Settings"];
        mainParameters.m_nEventsToProcess = mainInfo["EventsToProcess"];
        mainParameters.m_nEventsToSkip = mainInfo["EventsToSkip"];
        mainParameters.m_maxNHits = mainInfo["MaxNHits"];
        mainParameters.m_minNHits = mainInfo["MinNHits"];
        mainParameters.SetViewOption(mainInfo["ViewOption"]);
        const bool gotMainRecoOption = mainParameters.SetRecoOption(mainInfo["RecoOption"]);
        if (!gotMainRecoOption)
            return 1;

        MainNDPandora mainND("Main", mainParameters);

        const std::vector<std::string> instances = mainInfo["Instances"];

        // Add the other Pandora instances
        for (const std::string &instanceName : instances)
        {
            std::cout << "Setting up Pandora instance : " << instanceName << std::endl;
            const auto info = jsonConfig[instanceName];

            NDParameters NDPars(mainParameters);
            NDPars.m_volType = NDPars.GetVolEnum(info["VolType"]);
            NDPars.m_settingsFile = info["Settings"];
            NDPars.m_inputFileName = info["InputFile"];
            NDPars.m_inputTreeName = info["InputTree"];
            NDPars.m_dataFormat = NDPars.GetDataEnum(info["DataFormat"]);
            NDPars.m_geomFileName = info["GeomFile"];
            NDPars.m_geomManagerName = info["GeomManager"];
            NDPars.m_tpcName = info["TPCName"];
            NDPars.m_lengthScale = info["LengthScale"];
            NDPars.m_energyScale = info["EnergyScale"];

            // This instance may have different reco options compared to the main one
            if (info.contains("RecoOption"))
            {
                const bool gotRecoOption = NDPars.SetRecoOption(info["RecoOption"]);
                if (!gotRecoOption)
                    return 1;
            }
            // Instance may use different projection views as well
            if (info.contains("ViewOption"))
                NDPars.SetViewOption(info["ViewOption"]);

            // Set the event info using the main Pandora instance.
            // This assumes the input files for each Pandora instance have
            // the same event number ordering
            NDPars.m_nEventsToProcess = mainParameters.m_nEventsToProcess;
            NDPars.m_nEventsToSkip = mainParameters.m_nEventsToSkip;
            NDPars.m_maxNHits = mainParameters.m_maxNHits;
            NDPars.m_minNHits = mainParameters.m_minNHits;

            std::cout << "Settings file = " << NDPars.m_settingsFile << std::endl;
            std::cout << "VolType = " << NDPars.m_volType << ", DataFormat = " << NDPars.m_dataFormat << std::endl;
            std::cout << "Input file = " << NDPars.m_inputFileName << ", tree = " << NDPars.m_inputTreeName << std::endl;
            std::cout << "Geometry file = " << NDPars.m_geomFileName << ", manager = " << NDPars.m_geomManagerName << std::endl;
            std::cout << "TPC volume name = " << NDPars.m_tpcName << std::endl;
            std::cout << "Length scale (cm) = " << NDPars.m_lengthScale << ", Energy scale (GeV) = " << NDPars.m_energyScale << std::endl;

            mainND.AddPandoraInstance(instanceName, NDPars);
        }

        // Create the TPC geometry using the tpcNames in the NDParameters geometry ROOT file.
        // This creates TPCs for both the main & added Pandora instances
        mainND.CreatePandoraTPCs();

        // Create detector gaps
        mainND.CreatePandoraDetectorGaps();

        // Setup external & algorithm parameters for all Pandora instances
        mainND.ConfigurePandoraInstances();

        // Setup the event inputs
        mainND.ConfigureEventInputs();

        // Process the events
        mainND.ProcessEvents();

        std::cout << "Done" << std::endl;
    }
    catch (const StatusCodeException &statusCodeException)
    {
        std::cerr << "Pandora StatusCodeException: " << statusCodeException.ToString() << statusCodeException.GetBackTrace() << std::endl;
        errorNo = 1;
    }
    catch (...)
    {
        std::cerr << "Misconfigurated JSON file/parameters or unknown exception" << std::endl;
        errorNo = 1;
    }

    return errorNo;
}
