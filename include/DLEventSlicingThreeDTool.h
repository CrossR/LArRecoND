/**
 *  @file   include/DLEventSlicingThreeDTool.h
 *
 *  @brief  Header file for the ND DL event slicing tool class.
 *
 *  $Log: $
 */
#ifndef LAR_DL_EVENT_SLICING_THREE_D_TOOL_H
#define LAR_DL_EVENT_SLICING_THREE_D_TOOL_H 1

#ifdef LIBTORCH_DL

#include "larpandoradlcontent/LArSlicing/DlEventSlicingTool.h"

#include <map>

namespace lar_content
{

/**
 *  @brief  DLEventSlicingThreeDTool class
 *
 *          Each hit in a DL cluster is a 3D hit, which is replaced by the 3D hit and the U, V and W hits it was projected into.
 */
class DLEventSlicingThreeDTool : public lar_dl_content::DLEventSlicingTool
{
private:
    void InitializeHitLookup(const pandora::Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames);
    void AddHitToSlice(const pandora::CaloHit *const pCaloHit, Slice &slice) const;

    std::map<intptr_t, pandora::CaloHitList> m_hitIndexToCaloHitListMap; ///< The map from hit index to the hits created from it
};

} // namespace lar_content

#endif // #ifdef LIBTORCH_DL

#endif // #ifndef LAR_DL_EVENT_SLICING_THREE_D_TOOL_H
