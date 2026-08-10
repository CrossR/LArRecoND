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

        // Get the main Pandora instance handle
        TiXmlHandle mainHandle(xmlConfig.FirstChildElement("Main"));
        if (!mainHandle.Element())
        {
            std::cerr << "Error: Could not find 'Main' element in XML config." << std::endl;
            return 1;
        }

        NDParameters mainParameters;
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(mainHandle, "Settings", mainParameters.m_settingsFile));
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(mainHandle, "EventsToProcess", mainParameters.m_nEventsToProcess));
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(mainHandle, "EventsToSkip", mainParameters.m_nEventsToSkip));
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(mainHandle, "MaxNHits", mainParameters.m_maxNHits));
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(mainHandle, "MinNHits", mainParameters.m_minNHits));

        std::string viewOption, recoOption;
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(mainHandle, "ViewOption", viewOption));
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(mainHandle, "RecoOption", recoOption));

        mainParameters.SetViewOption(viewOption);
        if (!mainParameters.SetRecoOption(recoOption))
            return 1;

        MainNDPandora mainND("Main", mainParameters);

        std::vector<std::string> instances;
        if (mainHandle.FirstChildElement("Instances").Element())
            XmlHelper::ReadVectorOfValues(mainHandle, "Instances", instances);
        else
            std::cout << "No additional Pandora instances specified in the XML config file." << std::endl;

        // Add the other Pandora instances
        for (const std::string &instanceName : instances)
        {
            std::cout << "Setting up Pandora instance : " << instanceName << std::endl;
            const auto instanceHandle = mainHandle.FirstChildElement(instanceName.c_str());

            NDParameters NDPars(mainParameters);

            std::string volTypeStr;
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "VolType", volTypeStr));
            NDPars.m_volType = NDPars.GetVolEnum(volTypeStr);

            std::string dataFormatStr;
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "VolType", volTypeStr));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "DataFormat", dataFormatStr));
            NDPars.m_dataFormat = NDPars.GetDataEnum(dataFormatStr);

            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "Settings", NDPars.m_settingsFile));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "InputFile", NDPars.m_inputFileName));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "InputTree", NDPars.m_inputTreeName));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "GeomFile", NDPars.m_geomFileName));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "GeomManager", NDPars.m_geomManagerName));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "TPCName", NDPars.m_tpcName));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "LengthScale", NDPars.m_lengthScale));
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(instanceHandle, "EnergyScale", NDPars.m_energyScale));

            // This instance may have different reco options compared to the main one.
            std::string instanceRecoOption;
            if (XmlHelper::ReadValue(instanceHandle, "RecoOption", instanceRecoOption) == STATUS_CODE_SUCCESS)
            {
                if (!NDPars.SetRecoOption(instanceRecoOption))
                    return 1;
            }

            // Instance may use different projection views as well
            std::string instanceViewOption;
            if (XmlHelper::ReadValue(instanceHandle, "ViewOption", instanceViewOption) == STATUS_CODE_SUCCESS)
                NDPars.SetViewOption(instanceViewOption);

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
