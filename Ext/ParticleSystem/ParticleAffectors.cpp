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

#include "ParticleAffectors.h"
#include "../../Core/Behaviours/AbstractBehaviour.h"
#include "ParticleSystemNode.h"
#include <vector>

namespace MinSG {

// ----------------------------------------------------------------------------------------
// ParticleAffector

ParticleAffector::ParticleAffector(ParticleSystemNode* node) : AbstractNodeBehaviour(node) {
}

ParticleAffector::~ParticleAffector() = default;

// ----------------------------------------------------------------------------------------
// ParticleAnimator

ParticleAnimator::ParticleAnimator(ParticleSystemNode* node) : ParticleAffector(node) {
}

ParticleAnimator::~ParticleAnimator() = default;

/**
 * Just calls the animate & collect particles function of the
 * particle system.
 */
AbstractBehaviour::behaviourResult_t ParticleAnimator::doExecute() {
	// start affecting!
	ParticleSystemNode* psn = dynamic_cast<ParticleSystemNode*>(this->getNode());
	psn->collectAndAnimateParticles(getTimeDelta());

	return AbstractBehaviour::CONTINUE;
}


// ----------------------------------------------------------------------------------------
// ParticleGravityAffector

ParticleGravityAffector::ParticleGravityAffector(ParticleSystemNode* node) : ParticleAffector(node), gravity(0.0f, -9.81f, 0.0f) {
}

ParticleGravityAffector::~ParticleGravityAffector() = default;

/**
 * Affects particles with a constant force, defined by <gravity>
 */
AbstractBehaviour::behaviourResult_t ParticleGravityAffector::doExecute() {
	// start affecting!
	ParticleSystemNode* psn = dynamic_cast<ParticleSystemNode*>(this->getNode());

	Geometry::Vec3f scaledGravity(gravity*static_cast<float>(getTimeDelta()));

	std::vector<Particle> & particles = psn->getParticles();
	for(auto & particle : particles) {
		particle.direction += scaledGravity;
	}

	return AbstractBehaviour::CONTINUE;
}

// ----------------------------------------------------------------------------------------
// ParticleReflectionAffector

ParticleReflectionAffector::ParticleReflectionAffector(ParticleSystemNode* node) : ParticleAffector(node), plane( Geometry::Vec3(0,1,0),0), adherence(0.0), reflectiveness(1.0) {
}

ParticleReflectionAffector::~ParticleReflectionAffector() = default;

AbstractBehaviour::behaviourResult_t ParticleReflectionAffector::doExecute() {
	ParticleSystemNode* psn = dynamic_cast<ParticleSystemNode*>(this->getNode());
	for(auto & particle : psn->getParticles()) {
		if( plane.planeTest( particle.position ) < 0.0f ){
			particle.position = plane.getProjection( particle.position );
			
			if( plane.getNormal().dot( particle.direction ) < 0.0f ){
				const Geometry::Vec3 dir = particle.direction.reflect( plane.getNormal() ) * reflectiveness;
				particle.direction = dir * (1.0f-adherence) + (plane.getProjection(dir+particle.position)-particle.position) * adherence;
			}
		}
	}

	return AbstractBehaviour::CONTINUE;
}


// ----------------------------------------------------------------------------------------
// ParticleGravityAffector

ParticleFadeOutAffector::ParticleFadeOutAffector(ParticleSystemNode* node) : ParticleAffector(node) {
}

ParticleFadeOutAffector::~ParticleFadeOutAffector() = default;

/**
 * Linear fade out of particles using the alpha channel of the particle color
 */
AbstractBehaviour::behaviourResult_t ParticleFadeOutAffector::doExecute() {
	// start affecting!
	ParticleSystemNode* psn = dynamic_cast<ParticleSystemNode*>(this->getNode());

	std::vector<Particle> & particles = psn->getParticles();
	for(auto & particle : particles) {
		// be careful: Particle::color is a Color4ub!
		particle.color.setA(static_cast<uint8_t>(std::round(static_cast<double>(particle.color.getA()) * (1.0 - (getTimeDelta() / static_cast<double>(particle.timeLeft))))));
	}

	return AbstractBehaviour::CONTINUE;
}



}

#endif
