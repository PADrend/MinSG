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

#include "ParticleEmitters.h"
#include "ParticleSystemNode.h"
#include <Geometry/Angle.h>
#include <Geometry/Vec2.h>
#include <random>

namespace MinSG {


ParticleEmitter::ParticleEmitter(ParticleSystemNode* node) : AbstractNodeBehaviour(node),
		direction(0,1,0), directionVarianceAngle(Geometry::Angle::deg(45)),particlesPerSecond(50),
		minSpeed(5), maxSpeed(10), minLife(5), maxLife(5), minColor(1, 1, 1, 1), maxColor(1,1,1,1), numEmittedParticles(0),
		minWidth(1), maxWidth(1), minHeight(0), maxHeight(0),
		startTime(0.0), currentTime(0.0), timeOffset(0.0), started(false), enabled(true), engine() {
}

ParticleEmitter::~ParticleEmitter() {
}

Geometry::Vec3f ParticleEmitter::getADirection() {

	// do we have an up-vector?
	if(up.isZero()) {
		// calc up!
		if(direction.getX() == direction.getZ() && direction.getX() == 0) {
			// direction is y-only
			up = direction.cross(Geometry::Vec3f(1, 0, 0));
		} else {
			// direction is something not in y-direction
			up = direction.cross(Geometry::Vec3f(0, 1, 0));
		}
	}

	// rotate up by random angle around direction
	const auto upRotation = Geometry::Matrix3x3f::createRotation(Geometry::Angle::rad(std::uniform_real_distribution<float>(0, static_cast<float>(2.0 * M_PI))(engine)), direction);
	const Geometry::Vec3f rotatedUp(upRotation * up);
	const auto dirRotation = Geometry::Matrix3x3f::createRotation(Geometry::Angle::rad(std::uniform_real_distribution<float>(0, directionVarianceAngle.rad())(engine)), rotatedUp);
	// TODO variance
	return dirRotation * direction;
}

Util::Color4ub ParticleEmitter::getAColor() const {
	return Util::Color4ub(
			   static_cast<uint8_t>(std::uniform_int_distribution<uint16_t>(minColor.getR(), maxColor.getR())(engine)),
			   static_cast<uint8_t>(std::uniform_int_distribution<uint16_t>(minColor.getG(), maxColor.getG())(engine)),
			   static_cast<uint8_t>(std::uniform_int_distribution<uint16_t>(minColor.getB(), maxColor.getB())(engine)),
			   static_cast<uint8_t>(std::uniform_int_distribution<uint16_t>(minColor.getA(), maxColor.getA())(engine))
		   );
}

float ParticleEmitter::getALife() const {
	return std::uniform_real_distribution<float>(minLife, maxLife)(engine);
}

Geometry::Vec2f ParticleEmitter::getASize() const {
	const float width = std::uniform_real_distribution<float>(minWidth, maxWidth)(engine);
	const float height = (minHeight == 0.0f) ? width : std::uniform_real_distribution<float>(minHeight, maxHeight)(engine);
	return Geometry::Vec2f(width, height);
}

float ParticleEmitter::getASpeed() const {
	return std::uniform_real_distribution<float>(minSpeed, maxSpeed)(engine);
}

void ParticleEmitter::getSpawnCenter(Geometry::Vec3f& v, ParticleSystemNode* psystem)const {
	if(spawnNode.isNull()) {
		// just return zero
		v.setValue(0.0f);
	} else {
		// position is spawnNode.absolutePos - psystem.absolutePos (vec from psystem
		// to spawnNode)
		// v = spawnNode->getWorldOrigin() - psystem->getWorldOrigin();
		v = spawnNode->getWorldBB().getCenter() - psystem->getWorldOrigin();
	}
}

// ----------------------------------------------------------------------------------------------------------------
// ParticleBoxEmitter

ParticleBoxEmitter::ParticleBoxEmitter(ParticleSystemNode* node) : ParticleEmitter(node), emitBounds(-0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f) {
}

/**
 * Emits particles inside a box area around the reference point.
 */
AbstractBehaviour::behaviourResult_t ParticleBoxEmitter::doExecute() {
	ParticleEmitter::doExecute();

	// start emitting!
	ParticleSystemNode* psn = dynamic_cast<ParticleSystemNode*>(getNode());
	const uint64_t numParticlesToEmit = static_cast<uint64_t>(particlesPerSecond * getElapsed());

	Geometry::Matrix4x4f psnInv = psn->getRelTransformationMatrixPtr() ? (*psn->getRelTransformationMatrixPtr()).inverse() : Geometry::Matrix4x4f();
	Geometry::Matrix4x4f spawnNodeMatrix = (spawnNode.isNull() || spawnNode->getRelTransformationMatrixPtr() == nullptr) ? Geometry::Matrix4x4f() : spawnNode->getRelTransformationMatrix();

	for(; numEmittedParticles < numParticlesToEmit; ++numEmittedParticles) {
		// create particle!
		const float lifeTime = getALife();
		Geometry::Vec3f dir = getADirection();
		float speed = getASpeed();
		dir *= speed;

		// distribute into the box
		Geometry::Vec3f position;
		position.x(emitBounds.getExtentX() > 0.0f ? std::uniform_real_distribution<float>(emitBounds.getMinX(), emitBounds.getMaxX())(engine) : emitBounds.getMinX());
		position.y(emitBounds.getExtentY() > 0.0f ? std::uniform_real_distribution<float>(emitBounds.getMinY(), emitBounds.getMaxY())(engine) : emitBounds.getMinY());
		position.z(emitBounds.getExtentZ() > 0.0f ? std::uniform_real_distribution<float>(emitBounds.getMinZ(), emitBounds.getMaxZ())(engine) : emitBounds.getMinZ());

		if(!spawnNode.isNull()) {
			// just return zero

			position = (spawnNodeMatrix*psnInv).transformPosition(position);
		}

		if(psn->getParticleCount() < psn->getMaxParticleCount()) {
			psn->addParticle(Particle(position, dir, Geometry::Matrix3x3f(), getAColor(), lifeTime, lifeTime, getASize()));
		}
	}

	return AbstractBehaviour::CONTINUE;
}

// ----------------------------------------------------------------------------------------------------------------
// ParticlePointEmitter

ParticlePointEmitter::ParticlePointEmitter(ParticleSystemNode* node) : ParticleEmitter(node), minOffset(0.0f), maxOffset(0.0f) {
}

/**
 * Combined point and sphere emitter. If offset is set, it's an sphere emitter.
 */
AbstractBehaviour::behaviourResult_t ParticlePointEmitter::doExecute() {
	ParticleEmitter::doExecute();

	// start emitting!
	ParticleSystemNode* psn = dynamic_cast<ParticleSystemNode*>(getNode());
	const uint64_t numParticlesToEmit = static_cast<uint64_t>(particlesPerSecond * getElapsed());

	for(; numEmittedParticles < numParticlesToEmit; ++numEmittedParticles) {
		// create particle!
		const float lifeTime = getALife();
		Geometry::Vec3f dir = getADirection();
		float speed = getASpeed();
		dir *= speed;

		Geometry::Vec3f position;
		getSpawnCenter(position, psn);

		// offset
		if(maxOffset > 0.0f) {
			position += dir.getNormalized() * std::uniform_real_distribution<float>(minOffset, maxOffset)(engine);
		}

		if(psn->getParticleCount() < psn->getMaxParticleCount()) {
			psn->addParticle(Particle(position, dir, Geometry::Matrix3x3f(), getAColor(), lifeTime, lifeTime, getASize()));
		}
	}

	return AbstractBehaviour::CONTINUE;
}


}

#endif
