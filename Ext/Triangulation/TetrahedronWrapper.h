/*
	This file is part of the MinSG library extension Triangulation.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_TRIANGULATION

#ifndef MINSG_TRIANGULATION_TETRAHEDRONWRAPPER_H
#define MINSG_TRIANGULATION_TETRAHEDRONWRAPPER_H

#include <Geometry/Plane.h>
#include <Geometry/Tetrahedron.h>

namespace MinSG {
namespace Triangulation {

template<typename Point_t>
class TetrahedronWrapper {
	public:
		typedef typename Point_t::vector_t::value_t number_t;

		TetrahedronWrapper(const Point_t * p1, const Point_t * p2, const Point_t * p3, const Point_t * p4) {
			a = p1;
			b = p2;
			Geometry::_Plane<number_t> ab_p3(a->getPosition(), b->getPosition(), p3->getPosition());

			// if p4 is below plane ab_p3...
			if(ab_p3.planeTest(p4->getPosition()) < 0) {
				c = p3;
				d = p4;
			} else {
				c = p4;
				d = p3;
			}
			tetrahedron = Geometry::Tetrahedron<number_t>(a->getPosition(), b->getPosition(), c->getPosition(), d->getPosition());
		}

		const Point_t * getA() const {
			return a;
		}
		const Point_t * getB() const {
			return b;
		}
		const Point_t * getC() const {
			return c;
		}
		const Point_t * getD() const {
			return d;
		}

		const Geometry::Tetrahedron<number_t> & getTetrahedron() const {
			return tetrahedron;
		}

	private:
		const Point_t * a;
		const Point_t * b;
		const Point_t * c;
		const Point_t * d;
		Geometry::Tetrahedron<number_t> tetrahedron;
};

}
}
#endif /* MINSG_TRIANGULATION_TETRAHEDRONWRAPPER_H */

#endif /* MINSG_EXT_TRIANGULATION */
