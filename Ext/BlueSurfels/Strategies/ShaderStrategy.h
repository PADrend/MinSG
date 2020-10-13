/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef BLUE_SURFELS_STRATEGIES_ShaderStrategy_H_
#define BLUE_SURFELS_STRATEGIES_ShaderStrategy_H_

#include "AbstractSurfelStrategy.h"

#include <Util/Macros.h>

#include <Util/IO/FileLocator.h>

namespace Rendering {
class Shader;
}

namespace MinSG {
namespace BlueSurfels {

class ShaderStrategy : public AbstractSurfelStrategy {
	PROVIDES_TYPE_NAME(ShaderStrategy)
	public:
		MINSGAPI ShaderStrategy();
		MINSGAPI virtual bool prepare(MinSG::FrameContext& context, MinSG::Node* node);
		MINSGAPI virtual bool beforeRendering(MinSG::FrameContext& context);
		MINSGAPI virtual void afterRendering(MinSG::FrameContext& context);
		
		GETSET(std::string, ShaderVS, "")
		GETSET(std::string, ShaderFS, "")
		GETSET(std::string, ShaderGS, "")
		
		void setSurfelCulling(bool v) {
			if(surfelCulling != v)
				needsRefresh = true;
			surfelCulling = v;
		}
		bool getSurfelCulling() const { return surfelCulling; }
		
		void setSurfelDynSize(bool v) {
			if(surfelDynSize != v)
				needsRefresh = true;
			surfelDynSize = v;
		}
		bool getSurfelDynSize() const { return surfelDynSize; }
		
		MINSGAPI void refreshShader();
		Util::FileLocator& getFileLocator() { return locator; }
	private:
		Util::FileLocator locator;
		Util::Reference<Rendering::Shader> shader;
		bool needsRefresh = true;
		bool wasActive = false;
		bool surfelCulling = true;
		bool surfelDynSize = false;
};
  
} /* BlueSurfels */
} /* MinSG */

#endif /* end of include guard: BLUE_SURFELS_STRATEGIES_ShaderStrategy_H_ */
#endif // MINSG_EXT_BLUE_SURFELS