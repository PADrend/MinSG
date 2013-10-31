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

#ifndef PARTICLE_AFFECTORS_H_
#define PARTICLE_AFFECTORS_H_

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <Geometry/Vec3.h>


namespace MinSG {

class ParticleSystemNode;
class Node;


// ---------------------------------------------------------------------------------------

/**
 * An affector (most likely created using ParticleSystemNode::createAffector)
 * is used to affect existing particles. After creating the affector is has to be
 * registered with an behavior manager.
 *
 *	ParticleAffector ---|> AbstractNodeBehaviour
 *
 * @author Jan Krems
 * @date 2010-06-15
 */
class ParticleAffector : public AbstractNodeBehaviour {
		PROVIDES_TYPE_NAME(ParticleAffector)
	public:

		/**
		 * Just passes the node to AbstractNodeBehaviour
		 */
		ParticleAffector(ParticleSystemNode* node);

		virtual ~ParticleAffector();
};

// ---------------------------------------------------------------------------------------

/**
 * This is the most important affector. Without it, the particle system
 * won't do anything.
 */
class ParticleAnimator: public ParticleAffector {
		PROVIDES_TYPE_NAME(ParticleAnimator)
	public:
		/** empty */
		ParticleAnimator(ParticleSystemNode* node);
		/** empty */
		virtual ~ParticleAnimator();

		/**
		 * Just calls the animate & collect particles function of the
		 * particle system.
		 */
		AbstractBehaviour::behaviourResult_t doExecute() override;
};

// ---------------------------------------------------------------------------------------

/**
 * Affects particles with a constant force, defined by <gravity>
 *
 *	ParticleGravityAffector ---|> ParticleAffector ---|> AbstractNodeBehaviour
 */
class ParticleGravityAffector: public ParticleAffector {
		PROVIDES_TYPE_NAME(ParticleGravityAffector)
	public:
		ParticleGravityAffector(ParticleSystemNode* node);
		virtual ~ParticleGravityAffector();

		/**
		 * Affects particles with a constant force, defined by <gravity>
		 */
		AbstractBehaviour::behaviourResult_t doExecute() override;

		void setGravity(const Geometry::Vec3f& g) 	{ gravity = g; }
		const Geometry::Vec3f& getGravity()const	{ return gravity; }

	protected:
		/**
		 * The force (in units per second) that should affector the particles
		 */
		Geometry::Vec3f gravity;
};

// ---------------------------------------------------------------------------------------

/**
 * Linear fade out of particles using the alpha channel of the particle color
 *
 *	ParticleFadeOutAffector ---|> ParticleAffector ---|> AbstractNodeBehaviour
 */
class ParticleFadeOutAffector: public ParticleAffector {
		PROVIDES_TYPE_NAME(ParticleFadeOutAffector)
	public:
		ParticleFadeOutAffector(ParticleSystemNode* node);
		virtual ~ParticleFadeOutAffector();

		/**
		 * Linear fade out of particles using the alpha channel of the particle color
		 */
		AbstractBehaviour::behaviourResult_t doExecute() override;
};

}

#endif /* PARTICLE_AFFECTORS_H_ */
#endif
