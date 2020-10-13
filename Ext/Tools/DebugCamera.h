/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef DEBUGCAMERA_H
#define DEBUGCAMERA_H

#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"

#include <Util/ReferenceCounter.h>
#include <Util/References.h>

#include <Geometry/Matrix4x4.h>

namespace Rendering {
class FBO;
class Mesh;
class RenderingContext;
}
namespace MinSG {

class DebugCamera : public Util::ReferenceCounter<DebugCamera> {
public:
	MINSGAPI DebugCamera();
	MINSGAPI ~DebugCamera();

	MINSGAPI void displayMesh(Rendering::RenderingContext & rc, Rendering::Mesh * mesh);
	MINSGAPI void enable(Rendering::RenderingContext & rc, AbstractCameraNode * debug, AbstractCameraNode * original, Rendering::FBO * fbo);
	MINSGAPI void disable(Rendering::RenderingContext & rc);
	
	AbstractCameraNode::ref_t debug;
	AbstractCameraNode::ref_t original;
	Util::Reference<Rendering::FBO> fbo;
	Geometry::Matrix4x4 conversionMatrix;
};

}

#endif // DEBUGCAMERA_H
