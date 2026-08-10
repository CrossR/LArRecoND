/**
 *  @file   src/NDEventInput.cc
 *
 *  @brief  Implementation of the ND event input
 *
 *  $Log: $
 */

#include "NDEventInput.h"

#include "Api/PandoraApi.h"
#include "larpandoracontent/LArControlFlow/MultiPandoraApi.h"

#include "TFile.h"
#include "TTree.h"

namespace lar_nd_reco
{

NDEventInput::NDEventInput() :
    m_pPandora{nullptr},
    m_pMainPandora{nullptr},
    m_parameters(),
    m_tpcGeom(),
    m_pFile{nullptr},
    m_pTree{nullptr},
    m_nEntries{0},
    m_LArCaloHitFactory()
{
}

NDEventInput::NDEventInput(const pandora::Pandora *pPandora, const NDParameters &parameters, const NDSimpleTPCGeom &tpcGeom) :
    m_pPandora{pPandora},
    m_pMainPandora{nullptr},
    m_parameters(parameters),
    m_tpcGeom(tpcGeom),
    m_pFile{nullptr},
    m_pTree{nullptr},
    m_nEntries{0},
    m_LArCaloHitFactory()
{
    this->Initialize();
}

void NDEventInput::Initialize()
{
    std::cout << "Constructing NDEventInput for " << m_pPandora->GetName() << ", " << m_parameters.m_inputFileName << ", "
              << m_parameters.m_inputTreeName << std::endl;

    // Get the main Pandora instance pointer
    m_pMainPandora = MultiPandoraApi::GetPrimaryPandoraInstance(m_pPandora);
    std::cout << "Main Pandora instance is " << m_pMainPandora->GetName() << std::endl;

    m_pFile = TFile::Open(m_parameters.m_inputFileName.c_str(), "READ");
    if (m_pFile)
    {
        m_pTree = dynamic_cast<TTree *>(m_pFile->Get(m_parameters.m_inputTreeName.c_str()));
        if (!m_pTree)
        {
            std::cout << "Error in NDEventInput: can't find tree " << m_parameters.m_inputTreeName << std::endl;
            m_pFile->Close();
        }
        else
            m_nEntries = m_pTree->GetEntries();
    }
    else
        std::cout << "Error in NDEventInput: can't open file " << m_parameters.m_inputFileName << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

NDEventInput::~NDEventInput()
{
    if (m_pFile)
        m_pFile->Close();
}

//------------------------------------------------------------------------------------------------------------------------------------------

int NDEventInput::GetNuanceCode(const std::string &reaction) const
{
    // The GENIE reaction string (also stored by edep-sim) is created using
    // https://github.com/GENIE-MC/Generator/blob/master/src/Framework/Interaction/Interaction.cxx#L249
    // String format is "nu:PDGId;tgt:PDGId;N:PDGId;proc:interactionType,scattering;", e.g.
    //                  "nu:14;tgt:1000180400;N:2112;proc:Weak[CC],QES;"

    // GENIE scattering codes:
    // https://github.com/GENIE-MC/Generator/blob/master/src/Framework/Interaction/ScatteringType.h

    // Nuance codes:
    // https://internal.dunescience.org/doxygen/MCNeutrino_8h_source.html

    // GENIE conversion code for RooTracker output files:
    // https://github.com/GENIE-MC/Generator/blob/master/src/contrib/t2k/neut_code_from_rootracker.C
    // Similar code is available here (Neut reaction code):
    // https://internal.dunescience.org/doxygen/namespacegenie_1_1utils_1_1ghep.html

    // For now, just set the basic reaction types, excluding any specific final states:
    // https://github.com/GENIE-MC/Generator/blob/master/src/contrib/t2k/neut_code_from_rootracker.C#L276
    int code(1000);

    const bool is_cc = (reaction.find("Weak[CC]") != std::string::npos); // weak charged-current
    const bool is_nc = (reaction.find("Weak[NC]") != std::string::npos); // weak neutral-current
    // const bool is_charm = (reaction.find("charm")    != std::string::npos); // charm production
    const bool is_qel = (reaction.find("QES") != std::string::npos);   // quasi-elastic scattering
    const bool is_dis = (reaction.find("DIS") != std::string::npos);   // deep inelastic scattering
    const bool is_res = (reaction.find("RES") != std::string::npos);   // resonance
    const bool is_cohpi = (reaction.find("COH") != std::string::npos); // coherent pi
    const bool is_ve = (reaction.find("NuEEL") != std::string::npos);  // nu e elastic
    const bool is_imd = (reaction.find("IMD") != std::string::npos);   // inverse mu decay
    const bool is_mec = (reaction.find("MEC") != std::string::npos);   // meson exchange current

    if (is_qel)
    {
        code = 0;
        if (is_cc)
            code = 1001;
        else if (is_nc)
            code = 1002;
    }
    else if (is_dis)
    {
        code = 2;
        if (is_cc)
            code = 1091;
        else if (is_nc)
            code = 1092;
    }
    else if (is_res)
        code = 1;
    else if (is_cohpi)
    {
        code = 3;
        if (is_qel)
            code = 4;
    }
    else if (is_ve)
        code = 1098;
    else if (is_imd)
        code = 1099;
    else if (is_mec)
        code = 10;

    // std::cout << "Reaction " << reaction << " has code = " << code << std::endl;

    return code;
}

std::string NDEventInput::GetNuanceReaction(const int ccnc, const int mode) const
{
    std::string reaction("");

    if (mode == 0)
    {
        reaction = "QES";
    }
    else if (mode == 2)
    {
        reaction = "DIS";
    }
    else if (mode == 1)
    {
        reaction = "RES";
    }
    else if (mode == 3)
    {
        reaction = "COH";
    }
    else if (mode == 5)
    {
        reaction = "NuEEL";
    }
    else if (mode == 6)
    {
        reaction = "IMD";
    }
    else if (mode == 10)
    {
        reaction = "MEC";
    }

    if (ccnc == 0)
    {
        reaction += "Weak[CC]";
    }
    else if (ccnc == 1)
    {
        reaction += "Weak[NC]";
    }

    return reaction;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

} // namespace lar_nd_reco
