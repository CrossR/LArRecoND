/**
 *  @file   include/SlicingAnalysisAlgorithm.h
 *
 *  @brief  Header file for the slicing analysis algorithm.
 *
 *  $Log: $
 */
#ifndef LAR_SLICING_ANALYSIS_ALGORITHM_H
#define LAR_SLICING_ANALYSIS_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

#include <string>
#include <vector>

namespace lar_content
{

/**
 *  @brief  SlicingAnalysisAlgorithm class
 *
 *          Compares the 3D hits of the slice pfos with the true neutrino interactions, and writes the results to a ROOT file.
 */
class SlicingAnalysisAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    SlicingAnalysisAlgorithm();

    /**
     *  @brief  Destructor, writes the analysis trees
     */
    ~SlicingAnalysisAlgorithm();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    /**
     *  @brief  Evaluate the slices against the true neutrino interactions and fill the analysis trees
     *
     *  @param  sliceList the 3D hits of each slice
     */
    void EvaluateSlices(const std::vector<pandora::CaloHitList> &sliceList);

    std::string m_caloHitListName3D; ///< The name of the input 3D calo hit list
    std::string m_slicePfoListName;  ///< The name of the input slice pfo list
    std::string m_analysisTreeName;  ///< The name of the analysis tree
    std::string m_analysisFileName;  ///< The name of the analysis file
};

} // namespace lar_content

#endif // #ifndef LAR_SLICING_ANALYSIS_ALGORITHM_H
