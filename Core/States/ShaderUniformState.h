/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef ShaderUniformState_H
#define ShaderUniformState_H

#include "../../Core/States/State.h"
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Util/StringIdentifier.h>
#include <map>
#include <vector>
#include <stack>

namespace MinSG {


/**
 *  [ShaderUniformState] ---|> [State]
 * @ingroup states
 */
class ShaderUniformState : public State {
		PROVIDES_TYPE_NAME(ShaderUniformState)
	public:
		typedef std::map<Util::StringIdentifier, Rendering::Uniform> uniformMap_t;

		MINSGAPI ShaderUniformState();
		MINSGAPI ShaderUniformState(const ShaderUniformState & source);
		MINSGAPI virtual ~ShaderUniformState();

		/**
		 * sets the uniform value.
		 * the uniform will be applied on next enable.
		 */
		MINSGAPI void setUniform(const Rendering::Uniform & value);

		const uniformMap_t & getUniforms() const      				{   return uMap;    }
		bool hasUniform(const Util::StringIdentifier name) const	{   return uMap.count(name)!=0; }
		MINSGAPI const Rendering::Uniform & getUniform(const Util::StringIdentifier name)const;

		void removeUniform(const Util::StringIdentifier nameId)	{   uMap.erase(nameId);   }
		void removeUniform(const Rendering::Uniform & u)		{   removeUniform(u.getNameId()); }
		void removeUniforms()									{   uMap.clear(); }

		/// ---|> [Node]
		MINSGAPI ShaderUniformState * clone()const override;

	private:
		uniformMap_t uMap;

		std::stack<std::vector<Rendering::Uniform> > priorValueStack;

		MINSGAPI stateResult_t doEnableState(FrameContext & context,Node *, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context,Node *, const RenderParam & rp) override;

};
}
#endif // ShaderUniformState_H
