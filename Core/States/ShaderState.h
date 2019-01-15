/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef ShaderState_H
#define ShaderState_H

#include "State.h"
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <map>

namespace MinSG {

/**
 *  [ShaderState] ---|> [State]
 * @ingroup states
 */
class ShaderState : public State {
		PROVIDES_TYPE_NAME(ShaderState)
	public:
		typedef std::map<std::string, Rendering::Uniform> uniformMap_t;

		ShaderState(Rendering::Shader * _shader=nullptr);
		ShaderState(const ShaderState & source);
		virtual ~ShaderState();

		void setShader(Rendering::Shader * s)			{   shader=s;   }
		Rendering::Shader * getShader()					{	return shader.get();	}

		/**
		 * sets the uniform value for this shader state.
		 * the uniform will be applied on next enable.
		 */
		void setUniform(const Rendering::Uniform & value);

		/**
		 * sets the uniform value for this shader state.
		 * if the shader is active the uniform will be
		 * applied immediately otherwise on next enable.
		 */
		void setUniform(FrameContext & context, const Rendering::Uniform & value);

		const uniformMap_t & getUniforms() const      			{   return uMap;    }
		bool hasUniform(const std::string & name) const  		{   return uMap.count(name)!=0; }
		Rendering::Uniform getUniform(const std::string & name)const;

		inline void removeUniform(const std::string & name)     {   uMap.erase(name);   }
		inline void removeUniform(const Rendering::Uniform & u) {   removeUniform(u.getName()); }
		inline void removeUniforms()    						{   uMap.clear(); }

		/// ---|> [State]
		ShaderState * clone()const override;

	protected:
		uniformMap_t uMap;
		Util::Reference<Rendering::Shader> shader;

		stateResult_t doEnableState(FrameContext & context,Node *, const RenderParam & rp) override;
		void doDisableState(FrameContext & context,Node *, const RenderParam & rp) override;
};
}
#endif // ShaderState_H
