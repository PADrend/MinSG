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

#ifndef PARTICLE_EMITTERS_H_
#define PARTICLE_EMITTERS_H_

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <Geometry/Angle.h>
#include <Geometry/Box.h>
#include <Util/Graphics/Color.h>
#include <random>

namespace Geometry {
template<typename _T> class _Vec2;
typedef _Vec2<float> Vec2f;
}
namespace MinSG {

class ParticleSystemNode;
class Node;
class FrameContext;

/**
 * An emitter (most likely created using ParticleSystemNode::createEmittter)
 * is used to produce new particles. After creating the emitter is has to be
 * registered with an behavior manager.
 *
 *	ParticleEmitter ---|> AbstractNodeBehaviour
 *
 * @author Jans Krems, Benjamin Eikel
 * @date 2010-06-15
 * @ingroup behavior
 */
class ParticleEmitter : public AbstractNodeBehaviour {
		PROVIDES_TYPE_NAME(ParticleEmitter)
	public:
		MINSGAPI ParticleEmitter(ParticleSystemNode* node);
		MINSGAPI virtual ~ParticleEmitter();

		/**
		 * If this emitter currently is emitting. Some emitter may enable themselves
		 * (for example when they are emitting in intervals).
		 */
		bool isEnabled()const 						{ return enabled; }

		/**
		 * Set a node, newly spawned particles should be positioned at.
		 *
		 * If none is set, the position of the particle system node is used.
		 */
		void setSpawnNode(Node* n) 					{ spawnNode = n; }

		/**
		 * Get the node, newly spawned particles should be positioned at.
		 *
		 * If none is set, the position of the particle system node is used.
		 */
		Node* getSpawnNode()const					{ return spawnNode.get(); }

		/**
		 * Get direction in which new particles are directed
		 *
		 * @see getDirectionVarianceAngle
		 */
		const Geometry::Vec3f& getDirection()const 	{ return direction; }

		//! Set direction in which new particles are directed
		void setDirection(const Geometry::Vec3f& dir) { direction = dir; }

		//! Get angle by which the direction can differ
		const Geometry::Angle & getDirectionVarianceAngle() const {
			return directionVarianceAngle;
		}

		//! Set angle by which the direciton can differ
		void setDirectionVarianceAngle(const Geometry::Angle & angle) {
			directionVarianceAngle = angle;
		}

		//! Get how many particles should be emitted per second
		float getParticlesPerSecond()const			{ return particlesPerSecond; }

		//! Set how many particles should be emitted per second
		void setParticlesPerSecond(float pps) { particlesPerSecond = pps; reset();}

		/*
		 * Range of speed for newly created particles
		 */
		float getMinSpeed()const					{ return minSpeed; }
		float getMaxSpeed()const					{ return maxSpeed; }
		void setMinSpeed(float min) 				{ minSpeed = min; }
		void setMaxSpeed(float max) 				{ maxSpeed = max; }

		//! Range of lifetime for newly created particles
		float getMinLife()const						{ return minLife; }
		float getMaxLife()const						{ return maxLife; }
		void setMinLife(float min) 					{ minLife = min; }
		void setMaxLife(float max) 					{ maxLife = max; }

		//! Range of color for newly created particles
		const Util::Color4ub& getMinColor()const		{ return minColor; }
		const Util::Color4ub& getMaxColor()const		{ return maxColor; }
		void setMinColor(const Util::Color4ub& min) 	{ minColor = min; }
		void setMaxColor(const Util::Color4ub& max) 	{ maxColor = max; }

		//! Range of size for newly create particles
		float getMinWidth()const					{ return minWidth; }
		float getMaxWidth()const					{ return maxWidth; }
		float getMinHeight()const					{ return minHeight; }
		float getMaxHeight()const					{ return maxHeight; }
		void setMinWidth(float min) 				{ minWidth = min; }
		void setMaxWidth(float max) 				{ maxWidth = max; }
		void setMinHeight(float min) 				{ minHeight = min; }
		void setMaxHeight(float max) 				{ maxHeight = max; }

		/**
		 * Return the time offset for the starting time.
		 *
		 * @return Time offset in seconds.
		 */
		AbstractBehaviour::timestamp_t getTimeOffset() const				{ return timeOffset; }
		/**
		 * Set the time offset for the starting time.
		 *
		 * @param offset Time offset in seconds.
		 */
		void setTimeOffset(AbstractBehaviour::timestamp_t offset)			{ timeOffset = offset; reset(); }

		behaviourResult_t doExecute() override {
			const timestamp_t timeSec = getCurrentTime();
			if(!started) {
				startTime = timeSec;
				started = true;
			}
			currentTime = timeSec;
			return AbstractBehaviour::CONTINUE;
		}

	protected:
		//! Return time span between the first execution and the current execution (including the time offset).
		AbstractBehaviour::timestamp_t getElapsed() const {
			if(!started) {
				return 0.0;
			}
			// Never return a negative time span.
			return std::max(0.0, currentTime - (startTime + timeOffset));
		}

		Geometry::Vec3f direction, up; // normalized
		Geometry::Angle directionVarianceAngle;
		float particlesPerSecond;

		float minSpeed, maxSpeed;

		float minLife, maxLife;

		Util::Color4ub minColor, maxColor;

		//! Number of particles that have been emitted already.
		uint64_t numEmittedParticles;

		float minWidth, maxWidth, minHeight, maxHeight;

		Util::WeakPointer<Node> spawnNode;

	private:
		//! Global time at which this emitter was first executed. Used to calculate the elapsed time.
		AbstractBehaviour::timestamp_t startTime;

		//! Current global time. Used to calculate the elapsed time.
		AbstractBehaviour::timestamp_t currentTime;

		//! Offset to the global start time to delay the start. Used to calculate the elapsed time.
		AbstractBehaviour::timestamp_t timeOffset;

		//! Status flag that is @c true if the saved starting time has been set.
		bool started;

		//! Reset the started flag and the number of emitted particles to start over.
		void reset() {
			started = false;
			numEmittedParticles = 0;
		}

	protected:
		bool enabled;

		//! Random number generator.
		mutable std::mt19937 engine;

		//! [convenience] Generate a direction.
		MINSGAPI Geometry::Vec3f getADirection();

		//! [convenience] Generate a color.
		MINSGAPI Util::Color4ub getAColor() const;

		//! [convenience] Generate a life time.
		MINSGAPI float getALife() const;

		//! [convenience] Generate a speed.
		MINSGAPI float getASpeed() const;

		//! [convenience] Generate a size.
		MINSGAPI Geometry::Vec2f getASize() const;
		/**
		 * [convenience] If spawnNode is set, position of spawn node relative to psystem,
		 * zero elsewise (= position of the particle system). Particle position always are
		 * relative to the particle system they are part of.
		 */
		MINSGAPI void getSpawnCenter(Geometry::Vec3f& v, ParticleSystemNode* psystem)const;
};

// -----------------------------------------------------------------------------------

/**
 * Emits particles inside a box area around the reference point.
 *
 * 	ParticleBoxEmitter ---|> ParticleEmitter ---|> AbstractNodeBehaviour
 */
class ParticleBoxEmitter: public ParticleEmitter {
		PROVIDES_TYPE_NAME(ParticleBoxEmitter)
	public:
		MINSGAPI ParticleBoxEmitter(ParticleSystemNode* node);

		//! Emits particles inside a box area around the reference point.
		MINSGAPI AbstractBehaviour::behaviourResult_t doExecute() override;

		void setEmitBounds(const Geometry::Box& bounds) { emitBounds = bounds; }
		const Geometry::Box& getEmitBounds()const		{ return emitBounds; }

	private:

		//! Definition of the area
		Geometry::Box emitBounds;
};

// -----------------------------------------------------------------------------------

/**
 * Combined point and sphere emitter. If offset is set, it's an sphere emitter.
 *
 * 	ParticlePointEmitter ---|> ParticleEmitter ---|> AbstractNodeBehaviour
 */
class ParticlePointEmitter: public ParticleEmitter {
		PROVIDES_TYPE_NAME(ParticlePointEmitter)
	public:

		MINSGAPI ParticlePointEmitter(ParticleSystemNode* node);

		//! Combined point and sphere emitter. If offset is set, it's an sphere emitter.
		MINSGAPI AbstractBehaviour::behaviourResult_t doExecute() override;

		float getMinOffset()const		{ return minOffset; }
		void setMinOffset(float off) 	{ minOffset = off; }
		float getMaxOffset()const		{ return maxOffset; }
		void setMaxOffset(float off) 	{ maxOffset = off; }

	private:

		//! Range of offsets relative to the reference point particles are created
		float minOffset, maxOffset;
};

}

#endif /* PARTICLE_EMITTERS_H_ */

#endif
