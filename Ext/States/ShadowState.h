/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SHADOWSTATE_H_
#define SHADOWSTATE_H_

#include "../../Core/States/State.h"
#include <Geometry/Matrix4x4.h>
#include <Util/References.h>
#include <cstdint>

namespace Rendering {
class FBO;
}
namespace MinSG {
class FrameContext;
class LightNode;
class Node;
class TextureState;
class CameraNode;

/**
 * Node to create a shadow map from the attached scene graph subtree.
 *
 * @author Benjamin Eikel
 * @date 2009-10-29
 * @ingroup states
 */
class ShadowState : public State {
	PROVIDES_TYPE_NAME(ShadowState)
	public:
		MINSGAPI ShadowState(uint16_t textureSize);
		virtual ~ShadowState() = default;

		MINSGAPI ShadowState * clone() const override;

		/**
		 * Specify the light for which the shadow should be created.
		 *
		 * @param lightNode MinSG light node.
		 */
		void setLight(LightNode * lightNode) {
			light = lightNode; 
			needsUpdate = true;
		}


		/**
		 * Get the light for which the shadow is created.
		 *
		 * @return MinSG light node.
		 */
		LightNode * getLight() const {
			return light;
		}

		/**
		 * Return the texture matrix that is used for shadow projection.
		 *
		 * @return Texture matrix
		 */
		const Geometry::Matrix4x4f & getTexMatrix() const {
			return texMatrix;
		}

		uint16_t getTextureSize() const { return texSize; }

		bool isStatic() const { return staticShadow; }
		void setStatic(bool value) { 
			staticShadow = value; 
			needsUpdate = true; 
		}
		
		void update() { needsUpdate = true; }
	private:
		//! Texture matrix for shadow projection.
		Geometry::Matrix4x4f texMatrix;

		//! Side length of shadow texture.
		uint16_t texSize;

		//! Internal texture state that is used to store and enable the shadow texture.
		Util::Reference<TextureState> shadowTexture;

		//! Handle for frame buffer object used for shadow rendering.
		Util::Reference<Rendering::FBO> fbo;

		//! The light node to create shadow map for.
		LightNode * light;
		
		//! The camera node to use for shadow map rendering.
		Util::Reference<CameraNode> camera;
		
		//! Prevents the shadow map to be generated each frame (for static scenes).
		bool staticShadow;
		
		//! Indicates if an update of the shadow map is needed.
		bool needsUpdate;

		//! Prevent usage.
		ShadowState(const ShadowState &) = delete;

		//! Prevent usage.
		ShadowState & operator=(const ShadowState &) = delete;

		MINSGAPI void updateShadowMap(FrameContext & context, Node * node, const RenderParam & rp);
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};
}

#endif /* SHADOWSTATE_H_ */
