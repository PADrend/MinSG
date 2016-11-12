/*
	This file is part of the MinSG library.
	Copyright (C) 20016 Stanislaw Eppinger

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

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
