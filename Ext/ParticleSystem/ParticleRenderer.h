/*
	This file is part of the MinSG library extension ParticleSystem.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2010 Jan Krems
	Copyright (C) 2010-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PARTICLE

#ifndef PARTICLESTATES_H_
#define PARTICLESTATES_H_

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include "../../Core/RenderParam.h"

#include <Util/Graphics/Color.h>

namespace MinSG {

class ParticleSystemNode;
class Node;
class FrameContext;

/**
 * Render a billboard for every particle. For now it's only very basic, no special rules for
 * rotation around different axis implemented.
 */
struct ParticleBillboardRenderer {
	/**
	 * Render a billboard for every particle. For now it's only very basic, no special rules for
	 * rotation around different axis implemented.
	 */
	MINSGAPI void operator()(ParticleSystemNode * node, FrameContext & context, const RenderParam & rp);
};

//! Just render OpenGL-points where the particles are. Test renderer. Just works.
struct ParticlePointRenderer {
	/**
	 * Just render OpenGL-points where the particles are. Test renderer. Just works.
	 */
	MINSGAPI void operator()(ParticleSystemNode* psystem, FrameContext & context, const RenderParam & rp);
};

}

#endif /* PARTICLESTATES_H_ */
#endif
