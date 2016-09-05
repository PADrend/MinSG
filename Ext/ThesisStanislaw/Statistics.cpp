#ifdef MINSG_EXT_THESISSTANISLAW

#include "Statistics.h"

namespace MinSG {
namespace ThesisStanislaw{

Statistics::Statistics(MinSG::Statistics & statistics) {
  _lightPatchKey = statistics.addCounter("LightPatches", "ms");
  _photonSamplerKey = statistics.addCounter("PhotonSampler", "ms");
  _photonRendererKey = statistics.addCounter("PhotonRenderer", "ms");
  _phongGIKey = statistics.addCounter("PhongGI", "ms");
  
}

Statistics & Statistics::instance(MinSG::Statistics & statistics) {
	static Statistics singleton(statistics);
	return singleton;
}

}
}

#endif /* MINSG_EXT_SVS */
