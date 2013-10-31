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
#ifndef PARTICLE_H_
#define PARTICLE_H_

#include <Geometry/Matrix3x3.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Util/Graphics/Color.h>

namespace MinSG {

/**
 * @brief Single particle in a particle system
 */
struct Particle {
	Particle(const Geometry::Vec3f & _position,
			 const Geometry::Vec3f & _direction,
			 const Geometry::Matrix3x3f & _rotation,
			 const Util::Color4ub & _color,
			 float _timeLeft,
			 float _lifeTime,
			 const Geometry::Vec2f & _size) :
		position(_position),
		direction(_direction),
		rotation(_rotation),
		color(_color),
		timeLeft(_timeLeft),
		lifeTime(_lifeTime),
		size(_size) {
	}
	Geometry::Vec3f position; //!< current position of particle
	Geometry::Vec3f direction; //!< direction of movement and movement per time unit
	Geometry::Matrix3x3f rotation; //!< rotation per time unit

	Util::Color4ub color;	//!<	Color of the particle, renderer knows if it cares

	/*!	lifeTime: total time this particle will have to live
		timeLeft: time (starting with last simulation step) this particle has
					left to live	*/
	float timeLeft, lifeTime;

	//!	Size of the particle. The renderer knows if it cares.
	Geometry::Vec2f size;
};

}

#endif /* PARTICLE_H_ */
#endif /* MINSG_EXT_PARTICLE */
