/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MIRRORSTATE_H_
#define MIRRORSTATE_H_

#include "../../Core/States/State.h"
#include <Geometry/Vec3.h>
#include <Util/References.h>
#include <Util/TypeNameMacro.h>
#include <cstdint>

namespace Rendering {
class FBO;
class Texture;
}
namespace MinSG {
class FrameContext;
class GroupNode;
class Node;
class TextureState;

/**
 * State for the creation of a mirror texture for the Node it is attached to.
 *
 * @author Benjamin Eikel
 * @date 2009-08-23
 * @ingroup states
 */
class MirrorState : public State {
	PROVIDES_TYPE_NAME(MirrorState)
	public:
		MINSGAPI MirrorState(uint16_t textureSize);
		MINSGAPI virtual ~MirrorState();

		MINSGAPI MirrorState * clone() const override;

		void setRoot(GroupNode * newRoot) {
			rootNode = newRoot;
		}

		uint16_t getTextureSize() const {
			return texSize;
		}

		Rendering::FBO * getFBO()const	{
			return fbo.get();
		}
	private:
		//! Side length of mirror texture.
		const uint16_t texSize;

		//! Internal texture state that is used to store and enable the mirror texture.
		Util::Reference<TextureState> mirrorTexture;

		//! Handle for convenience depth texture.
		Util::Reference<Rendering::Texture> depthTexture;

		//! Handle to frame buffer object used for mirror rendering.
		Util::Reference<Rendering::FBO> fbo;

		//! Handle to root node that is rendered as mirror image.
		GroupNode * rootNode;

		//! Cache of last camera position. Mirror image is only updated if position has been changed.
		Geometry::Vec3f lastCamPos;

		//! Prevent usage.
		MINSGAPI MirrorState(const MirrorState &);

		//! Prevent usage.
		MINSGAPI MirrorState & operator=(const MirrorState &);

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};
}

#endif /* MIRRORSTATE_H_ */
