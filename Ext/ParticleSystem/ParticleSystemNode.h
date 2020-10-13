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

#ifndef PARTICLESYSTEMNODE_H_
#define PARTICLESYSTEMNODE_H_

#include "Particle.h"
#include "ParticleAffectors.h"
#include "ParticleEmitters.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/Behaviours/AbstractBehaviour.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Box.h>
#include <functional>
#include <vector>

namespace MinSG {

/**
 * Node holding particles and being the interface for different decorators. Contains only the
 * logic for moving/animating and collecting particles, the rest is done by emitters, affectors
 * and renderers.
 *
 * Emitter: Produces and initializes particles. See ParticlePointEmitter for a simple example.
 *
 * Affector: Processes all particles each frame (or: in every run of the behaviours). Affectors
 * 				can do about everything. They could also produce output outside the particle system
 * 				(e.g. by building a path using the particle data). The most important affector
 * 				is ParticleAnimator. It calls ParticleSystemNode::collectAndAnimateParticles - without
 *				it not much will happen.
 *
 * Renderer: Called by ParticleSystemNode to display the particles. Per default a simple PointRenderer,
 * 				though you might want to use the BILLBOARD_RENDERER.
 *
 * The user of the particle system is responsable for registering the behaviours (emitter/affector) with
 * a behaviour manager, so that they are executed.
 *
 * @author Jan Krems, Benjamin Eikel
 * @date 2010-06-04
 * @ingroup nodes
 */
class ParticleSystemNode: public Node {
		PROVIDES_TYPE_NAME(ParticleSystemNode)
	public:

	typedef Util::Reference<ParticleSystemNode> ref_t;

		enum renderer_t {
			POINT_RENDERER = 1024,
			BILLBOARD_RENDERER = 1025
		};

		typedef std::function<void (ParticleSystemNode *, FrameContext &, const RenderParam &)> ParticleRenderer;

		/**
		 * set default values & renderer
		 * - no particles
		 * - max particles: 1000
		 */
		MINSGAPI ParticleSystemNode();
		ParticleSystemNode(const ParticleSystemNode&) = default;

		//! release data
		MINSGAPI virtual ~ParticleSystemNode();

		//! Set renderer based on rendererId
		MINSGAPI void setRenderer(renderer_t typeId);

		//! set custom renderer
		MINSGAPI void setRenderer(ParticleRenderer _renderer);

		//! Get type-id of renderer. Used for serialization.
		renderer_t getRendererType()const					{	return rendererType;	}

		//! ---|> Node
		MINSGAPI void doDisplay(FrameContext & context, const RenderParam & rp) override;

		/*! (internal) should only used by a ParticleEmitter
			Create a new particle and return it for initialization	*/
		void addParticle(const Particle & particle) {
			particles.push_back(particle);
		}

		//! (internal) should only used by a ParticleAffector
		std::vector<Particle> & getParticles() {
			return particles;
		}

		//! (internal) should only used by a ParticleAffector
		uint32_t getParticleCount()const  		{ 	return particles.size(); }

		uint32_t getMaxParticleCount()const  	{ 	return maxParticleCount; }
		void setMaxParticleCount(uint32_t max) 	{ 	maxParticleCount = max; }

		/*! (internal)* ONLY CALLED BY ParticleAnimator
			- age particles (subtract the elapsed time from timeLeft)
			- collect dead particles (timeLeft <= 0)
			- animate particles left	*/
		MINSGAPI void collectAndAnimateParticles(AbstractBehaviour::timestamp_t elapsed); // collect dead particles

	private:
		/// ---|> [Node]
		ParticleSystemNode * doClone()const override	{	return new ParticleSystemNode(*this);	}		
		const Geometry::Box& doGetBB() const override	{	return particleBounds;	}
		
		// std::deque was tested for particles, but it was slower.
		std::vector<Particle> particles;
		uint32_t maxParticleCount;

		Geometry::Box particleBounds;

		renderer_t rendererType;
		ParticleRenderer renderer;
};

}

#endif /* PARTICLESYSTEMNODE_H_ */

#endif
