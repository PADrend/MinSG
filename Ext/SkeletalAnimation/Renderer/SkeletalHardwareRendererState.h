/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#ifndef SKELETELMESHRENDERER_H_
#define SKELETELMESHRENDERER_H_

namespace Util {
	class FileName;
}

namespace Geometry {
	template<typename _T> class _Matrix4x4;
	typedef _Matrix4x4<float> Matrix4x4;
}

namespace MinSG {
	class Node;
	class AbstractJoint;
	class SkeletalNode;
}

#include "SkeletalAbstractRendererState.h"

#include <Rendering/Texture/Texture.h>
#include <Util/Graphics/PixelAccessor.h>

namespace MinSG
{
	/*
	 *  @brief renderer state for calculating end position of vertex inside shader.
	 *
	 *  Provides two techniques for vertex position calculation in shader.
	 *  - Texture, Joint matrix will be send via texture data.
	 *  - Uniform, Joint matrix will be send via uniforms, a maximum of 8 joints are allowed
	 *  - NoTransformation, if texture and uniform fails the vertex position will be transformed into
	 *    skeletal space only.
	 *  - NoShader fallback when shader initialization fails. Vertices will be drawn in model space only.
	 *
	 *  State tries Texture -> Uniform -> NoTransformation -> NoShader. 
	 *
	 *  [SkeletalHardwareRendererState] ---|> [ShaderState] ---|> [State]
	 * @ingroup states
	 */
	class SkeletalHardwareRendererState : public SkeletalAbstractRendererState
	{
		PROVIDES_TYPE_NAME(SkeletalHardwareRendererState)
	private:        
		Util::Reference<Rendering::Texture> texture;
		Util::Reference<Util::PixelAccessor> pa;
		
		uint32_t shaderType;

		std::string vertexFileName;
		std::string fragmentFileName;

		void generateJointGLArray(std::vector<Geometry::Matrix4x4> *container);
		
		bool noTransformation();
		bool textureTransformation();
		bool uniformTransformation();
		
		void attachUniversalShaderFiles(Util::Reference<Rendering::Shader> _shader);
		
		bool textureUnitSet;

	protected:
		void init(uint32_t forceShaderType);
		
		stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;

	public:
		/****************************************************************
		 * enums defining the shader type. 
		 * When changing during runtime, initialization is called again.
		 ****************************************************************/
		enum shaderHandlingTypes {UNIFORM, TEXTURE, NOTSUPPORTED, NOSHADER};
		
		MINSGAPI SkeletalHardwareRendererState();
		MINSGAPI SkeletalHardwareRendererState(const uint32_t forceShaderType);
		MINSGAPI SkeletalHardwareRendererState(const SkeletalHardwareRendererState &source);
		virtual ~SkeletalHardwareRendererState() { }
		
		bool switchToHandlingType(const uint32_t _shaderType);
		uint32_t getUsingShaderType() const { return shaderType; }
		
		/// ---|> [SkeletalAbstractRendererState]
		virtual void validateMatriceOrder(Node *node) override;
		
		/// ---|> [State]
		SkeletalHardwareRendererState *clone()const override;
	};

}



#endif
#endif
