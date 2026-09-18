/**
 *  @file   include/CheatingStitchingTool.h
 *
 *  @brief  Header file for the cheating stitching tool.
 *
 *  $Log: $
 */
#ifndef LAR_CHEATING_STITCHING_TOOL_H
#define LAR_CHEATING_STITCHING_TOOL_H 1

#include "larpandoracontent/LArThreeDReco/LArPfoStitching/StitchingBaseTool.h"

namespace lar_content
{

/**
 *  @brief  CheatingStitchingTool class
 */
class CheatingStitchingTool : public StitchingBaseTool
{
public:
    /**
     *  @brief  Default constructor
     */
    CheatingStitchingTool();

protected:
    /**
     *  @brief  Run the stitching logic
     *
     *  @param  pAlgorithm address of the calling algorithm
     *  @param  pStitchingOperations address of the calling algorithm's stitching operations implementation
     *  @param  pMultiPfoList the list of pfos in multiple lar tpcs
     *  @param  pfoToLArTPCMap the pfo to lar tpc map
     *  @param  stitchedPfosToX0Map a map of cosmic-ray pfos that have been stitched between lar tpcs to the X0 shift
     */
    void RunStitching(const pandora::Algorithm *const pAlgorithm, const StitchingPfoOperations *const pStitchingOperations,
        const pandora::PfoList *const pMultiPfoList, PfoToLArTPCMap &pfoToLArTPCMap, PfoToFloatMap &stitchedPfosToX0Map);

private:
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);
};

} // namespace lar_content

#endif // #ifndef LAR_CHEATING_STITCHING_TOOL_H
