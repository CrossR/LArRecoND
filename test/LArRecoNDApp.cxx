/**
 *  @file   LArRecoND/app/LArRecoNDApp.cxx
 *
 *  @brief  Implementation of the LArRecoND application
 *
 *  $Log: $
 */

#include <iostream>

#include "Helpers/XmlHelper.h"
#include "Xml/tinyxml.h"

#include "larrecond/LArControlFlow/MainNDPandora.h"
#include "larrecond/LArObjects/NDParameters.h"

#include "LArRecoNDApp.h"

#ifdef MONITORING
#include "TApplication.h"
#endif

using namespace pandora;
using namespace lar_nd_reco;

int main(int argc, char *argv[])
{
    int errorNo(0);

    const std::string configFileName{(argc > 1) ? argv[1] : "config/LArND_TMS.xml"};
    std::cout << "XML ConfigFileName = " << configFileName << std::endl;

    try
    {

#ifdef MONITORING
        TApplication *pTApplication = new TApplication("LArRecoNDApp", &argc, argv);
        pTApplication->SetReturnFromRun(kTRUE);
#endif

        TiXmlDocument xmlConfig;
        if (!xmlConfig.LoadFile(configFileName.c_str()))
        {
            std::cerr << "Error: Failed to load XML config file: " << configFileName << std::endl;
            return 1;
        }

        // Get the main Pandora instance
        const auto mainInfo = xmlConfig.FirstChildElement("Main");
        std::cout << "mainInfo = " << mainInfo << std::endl;

        NDParameters mainParameters;
        mainParameters.m_settingsFile = mainInfo->Attribute("Settings");
        mainParameters.m_nEventsToProcess = std::stoi(mainInfo->Attribute("EventsToProcess"));
        mainParameters.m_nEventsToSkip = std::stoi(mainInfo->Attribute("EventsToSkip"));
        mainParameters.m_maxNHits = std::stoi(mainInfo->Attribute("MaxNHits"));
        mainParameters.m_minNHits = std::stoi(mainInfo->Attribute("MinNHits"));
        mainParameters.SetViewOption(mainInfo->Attribute("ViewOption"));
        const bool gotMainRecoOption = mainParameters.SetRecoOption(mainInfo->Attribute("RecoOption"));
        if (!gotMainRecoOption)
            return 1;

        MainNDPandora mainND("Main", mainParameters);

        std::vector<std::string> instances;

        if (mainInfo->Attribute("Instances"))
            XmlHelper::ReadVectorOfValues(TiXmlHandle(mainInfo), "Instances", instances);
        else
            std::cout << "No additional Pandora instances specified in the XML config file." << std::endl;

        // Add the other Pandora instances
        for (const std::string &instanceName : instances)
        {
            std::cout << "Setting up Pandora instance : " << instanceName << std::endl;
            const auto info = xmlConfig.FirstChildElement(instanceName.c_str());

            NDParameters NDPars(mainParameters);
            NDPars.m_volType = NDPars.GetVolEnum(info->Attribute("VolType"));
            NDPars.m_settingsFile = info->Attribute("Settings");
            NDPars.m_inputFileName = info->Attribute("InputFile");
            NDPars.m_inputTreeName = info->Attribute("InputTree");
            NDPars.m_dataFormat = NDPars.GetDataEnum(info->Attribute("DataFormat"));
            NDPars.m_geomFileName = info->Attribute("GeomFile");
            NDPars.m_geomManagerName = info->Attribute("GeomManager");
            NDPars.m_tpcName = info->Attribute("TPCName");
            NDPars.m_lengthScale = std::stof(info->Attribute("LengthScale"));
            NDPars.m_energyScale = std::stof(info->Attribute("EnergyScale"));

            // This instance may have different reco options compared to the main one
            if (info->Attribute("RecoOption"))
            {
                const bool gotRecoOption = NDPars.SetRecoOption(info->Attribute("RecoOption"));
                if (!gotRecoOption)
                    return 1;
            }
            // Instance may use different projection views as well
            if (info->Attribute("ViewOption"))
                NDPars.SetViewOption(info->Attribute("ViewOption"));

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
