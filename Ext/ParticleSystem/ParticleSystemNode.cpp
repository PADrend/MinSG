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

#include "ParticleSystemNode.h"

#include "ParticleAffectors.h"
#include "ParticleEmitters.h"
#include "ParticleRenderer.h"

#include "../../Core/Behaviours/BehaviourManager.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/FrameContext.h"

#include <Rendering/RenderingContext/ParameterStructs.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>

#include <Util/Graphics/ColorLibrary.h>
#include <Util/Macros.h>

#include <algorithm>

namespace MinSG {

/**
 * set default values & renderer
 * - no particles
 * - max particles: 1000
 */
ParticleSystemNode::ParticleSystemNode() :
		Node(), particles(), maxParticleCount(1000), particleBounds(), renderer() {
	// default: set point renderer
	setRenderer(ParticleSystemNode::POINT_RENDERER);
}

//! release data
ParticleSystemNode::~ParticleSystemNode() {
}

//!  Set renderer by typeId
void ParticleSystemNode::setRenderer(renderer_t typeId) {
	if(typeId == ParticleSystemNode::POINT_RENDERER) {
		setRenderer(ParticlePointRenderer());
		rendererType = typeId;
	} else if(typeId == ParticleSystemNode::BILLBOARD_RENDERER) {
		setRenderer(ParticleBillboardRenderer());
		rendererType = typeId;
	}else WARN("setRenderer: Wrong renderer id");
}

void ParticleSystemNode::setRenderer(ParticleRenderer _renderer) {
	renderer = _renderer;
}

/**
 * ONLY CALLED BY ParticleAnimator
 *
 * - age particles (subtract the elapsed time from timeLeft)
 * - collect dead particles (timeLeft <= 0)
 * - animate particles left
 */
void ParticleSystemNode::collectAndAnimateParticles(AbstractBehaviour::timestamp_t elapsed) {
	particleBounds.invalidate();
	const Geometry::Matrix3x3f id;

	// Remove dead particles first.
	particles.erase(std::remove_if(particles.begin(),
								   particles.end(),
								   [](const Particle & particle) {
										return particle.lifeTime > 0.0f && particle.timeLeft < 0.0f;
								   }),
					particles.end());

	// Only update and copy the particles that are alive.
	for(auto & elem : particles) {
		// Age
		if(elem.lifeTime > 0.0f) {
			elem.timeLeft -= elapsed;
		}
		// Translation
		elem.position += elem.direction * elapsed;
		// Rotation
		const Geometry::Matrix3x3f blendedRotation(id, elem.rotation, elapsed);
		elem.direction = blendedRotation * elem.direction;

		particleBounds.include(elem.position);
	}

	worldBBChanged();
}

/**
 * render using the current renderer
 *  ---|> Node
 */
void ParticleSystemNode::doDisplay(FrameContext & context, const RenderParam & rp /* =0 */) {

	if(rp.getFlag(SHOW_META_OBJECTS)) {
		context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::LESS));
		context.getRenderingContext().pushAndSetLighting(Rendering::LightingParameters(false));
		context.getRenderingContext().applyChanges();

		Geometry::Box box(Geometry::Vec3f(0,0,0), 1);
		Rendering::drawBox(context.getRenderingContext(), box,Util::ColorLibrary::CYAN);

		context.getRenderingContext().popLighting();
		context.getRenderingContext().popDepthBuffer();
	}

	// render particles
	renderer(this, context, rp);
}

}

#endif
