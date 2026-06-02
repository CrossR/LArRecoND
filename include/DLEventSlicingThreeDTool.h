/**
 *  @file   include/DLEventSlicingThreeDTool.h
 *
 *  @brief  Header file for the 3D event slicing tool class.
 *
 *  $Log: $
 */
#ifndef LAR_DL_EVENT_SLICING_THREE_D_TOOL_H
#define LAR_DL_EVENT_SLICING_THREE_D_TOOL_H 1

#include "Pandora/Algorithm.h"
#include "Pandora/AlgorithmTool.h"

#include "larpandoracontent/LArObjects/LArPointingCluster.h"
#include "larpandoracontent/LArObjects/LArThreeDSlidingConeFitResult.h"

#include "EventSlicingThreeDBaseTool.h"
#include "LArSlice3D.h"
#include "SlicingThreeDAlgorithm.h"

#include <unordered_map>

namespace lar_content
{

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 *  @brief  DLEventSlicingThreeDTool class
 */
class DLEventSlicingThreeDTool : public EventSlicingThreeDBaseTool
{
public:
    /**
     *  @brief  Default constructor
     */
    DLEventSlicingThreeDTool();

    /**
     *  @brief  Run the 3D slicing tool
     *
     *  @param  pAlgorithm the address of the calling algorithm
     *  @param  caloHitListNames the hit type to calo hit list name map
     *  @param  clusterListNames the hit type to cluster list name map
     *  @param  sliceList to receive the populated slice list
     */
    void RunSlicing(const pandora::Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames,
        const HitTypeToNameMap &clusterListNames, Slice3DList &sliceList);

private:

    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string m_inputVertexContextKey; ///< Event-context key containing 3D candidate vertices used to seed slicing.

};

} // namespace lar_content

#endif // #ifndef LAR_DL_EVENT_SLICING_THREE_D_TOOL_H
