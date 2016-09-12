#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_STATISTICS_H
#define MINSG_EXT_THESISSTANISLAW_STATISTICS_H

//#define MINSG_THESISSTANISLAW_GATHER_STATISTICS //Define this symbol when statistics should be gathered

#include <cstdint>
#include <iostream>
#include "../../Core/Statistics.h"

namespace MinSG {
namespace ThesisStanislaw{

//! Singleton holder object for SVS related counters.
class Statistics {
  private:
    explicit Statistics(MinSG::Statistics & statistics);
    Statistics(Statistics &&) = delete;
    Statistics(const Statistics &) = delete;
    Statistics & operator=(Statistics &&) = delete;
    Statistics & operator=(const Statistics &) = delete;

    //! Key of lightpatchrenderer time
    uint32_t _lightPatchKey;
    //! Key of photonSampler time
    uint32_t _photonSamplerKey;
    //! Key of photonRenderer time
    uint32_t _photonRendererKey;
    //! Key of phongGI time
    uint32_t _phongGIKey;

  public:
    //! Return singleton instance.
    static Statistics & instance(MinSG::Statistics & statistics);

    void addLightPatchTime(MinSG::Statistics & statistics, double ms){
      statistics.addValue(_lightPatchKey, ms);
    }
    
    void addPhotonSamplerTime(MinSG::Statistics & statistics, double ms){
      statistics.addValue(_photonSamplerKey, ms);
    }
    
    void addPhotonRendererTime(MinSG::Statistics & statistics, double ms){
      statistics.addValue(_photonRendererKey, ms);
    }
    
    void addPhongGITime(MinSG::Statistics & statistics, double ms){
      statistics.addValue(_phongGIKey, ms);
    }

};

}
}

#endif /* MINSG_SVS_STATISTICS_H_ */

#endif /* MINSG_EXT_SVS */
