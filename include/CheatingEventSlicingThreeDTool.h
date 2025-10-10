/**
 *  @file   larrecond/include/CheatingEventSlicingThreeDTool.h
 *
 *  @brief  Header file for the cheating 3D event slicing tool class.
 *
 *  $Log: $
 */
#include "Pandora/AlgorithmTool.h"
#ifndef LAR_CHEATING_EVENT_SLICING_THREE_D_TOOL_H
#define LAR_CHEATING_EVENT_SLICING_THREE_D_TOOL_H 1

#include "EventSlicingThreeDTool.h"
#include "LArSlice3D.h"

#include <unordered_map>

namespace lar_content
{

/**
 *  @brief  CheatingEventSlicingThreeDTool class
 */
class CheatingEventSlicingThreeDTool : public EventSlicingThreeDTool
{
public:
    void RunSlicing(const pandora::Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames,
        const HitTypeToNameMap &clusterListNames, Slice3DList &sliceList) override;

private:
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle) override;

    typedef std::unordered_map<const pandora::MCParticle *, Slice3D> MCParticleTo3DSliceMap;

    /**
     *  @brief  Initialize the map from parent mc particles to slice objects
     *
     *  @param  pAlgorithm address of the calling algorithm
     *  @param  caloHitListNames the hit type to calo hit list name map
     *  @param  mcParticleToSliceMap to receive the parent mc particle to slice map
     */
    void InitializeMCParticleToSliceMap(const pandora::Algorithm *const pAlgorithm, const HitTypeToNameMap &caloHitListNames,
        MCParticleTo3DSliceMap &mcParticleToSliceMap) const;

    /**
     *  @brief  Fill slices using hits from a specified view
     *
     *  @param  pAlgorithm address of the calling algorithm
     *  @param  hitType the hit type (i.e. view)
     *  @param  caloHitListNames the hit type to calo hit list name map
     *  @param  mcParticleToSliceMap to receive the parent mc particle to slice map
     */
    void FillSlices(const pandora::Algorithm *const pAlgorithm, const pandora::HitType hitType, const HitTypeToNameMap &caloHitListNames,
        MCParticleTo3DSliceMap &mcParticleToSliceMap) const;
};

} // namespace lar_content

#endif // #ifndef LAR_CHEATING_EVENT_SLICING_THREE_D_TOOL_H
