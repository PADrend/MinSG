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

#ifndef MINSG_TRIANGULATION_DELAUNAY2D_H
#define MINSG_TRIANGULATION_DELAUNAY2D_H

#include <Geometry/Vec2.h>
#include <Util/References.h>
#include <deque>
#include <functional>
#include <vector>
#include <set>
#include <type_traits>


#include "../../Core/Nodes/ListNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/States/MaterialState.h"
#include <Geometry/Vec2.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Util/Macros.h>
#include <utility>

namespace MinSG {

class ListNode;

namespace Triangulation {

typedef std::pair<Geometry::Vec2f, size_t> WrapperPoint;

class D2D_Node;
class D2D_Edge;
class D2D_Triangle;

//! Internal class used by Delaunay2d
class D2D {
	private:
		std::vector<D2D_Node*> nodes;
		std::set<D2D_Edge*> edges;
		std::set<D2D_Triangle*> triangles;
		D2D_Edge * hullStart;

	public:
		D2D() : hullStart(nullptr) {
		}
		~D2D() {
			clear();
		}
		MINSGAPI void addPoint(const WrapperPoint & point);
		MINSGAPI void clear();
		MINSGAPI void expandTri(D2D_Edge * e, D2D_Node * node, int type);
		MINSGAPI void expandHull(D2D_Node * node);
		MINSGAPI int searchEdge(D2D_Edge * e, D2D_Node * node, D2D_Edge * & foundEdge)const;
		MINSGAPI void swapTest(D2D_Edge * e11);

		typedef std::function<void (const WrapperPoint &, const WrapperPoint &, const WrapperPoint &)> generatorFunction_t;
		MINSGAPI void generate(generatorFunction_t generator) const;
};

/*! Delaunay2d
	The implementation is based on the Java applet '2D Delaunay Triangulation Java Applet'
	by Luke Sunder Parige (parige@ews.uiuc.edu) dated August 10, 1999.
	\see http://members.fortunecity.com/lukesunder/dt2d.htm

	\todo Add efficient triangle query method.
	\todo Memory management: It is still not assured that all objects are freed properly.
	\todo Support deletion of points.
*/
template<typename Point_t>
class Delaunay2d {
	public:
		//! Function expecting a triangle consisting of three points
		typedef std::function<void (const Point_t &, const Point_t &, const Point_t &)> generatorFunction_t;
	private:
		static_assert(std::is_convertible<typename Point_t::vector_t, Geometry::Vec2f>::value, "Vector has to be two-dimensional.");
		std::deque<Point_t> points;
		D2D internal;

		Delaunay2d(const Delaunay2d &) = delete;
		Delaunay2d(Delaunay2d &&) = delete;

		struct Unwrapper {
			const std::deque<Point_t> & m_points;
			generatorFunction_t m_generator;

			Unwrapper(const std::deque<Point_t> & p_points, generatorFunction_t p_generator) :
				m_points(p_points), m_generator(p_generator) {
			}

			void operator()(const WrapperPoint & a, const WrapperPoint & b, const WrapperPoint & c) {
				m_generator(m_points[a.second], m_points[b.second], m_points[c.second]);
			}
		};
	public:
		Delaunay2d() {
		}

		/*! Add a point to the triangulation.
			\note only the x and y component of the point's position are used.	*/
		void addPoint(const Point_t & point) {
			internal.addPoint(std::make_pair(point.getPosition(), points.size()));
			points.push_back(point);
		}

		/**
		 * Call the given functor for every triangle.
		 * 
		 * @param generator Functor that will be called for every triangle
		 */
		void generate(generatorFunction_t generator) const {
			internal.generate(Unwrapper(points, generator));
		}
};

}
}

#endif // MINSG_TRIANGULATION_DELAUNAY2D_H
#endif /* MINSG_EXT_TRIANGULATION */
