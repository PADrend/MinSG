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

#ifndef MINSG_TRIANGULATION_HELPER_H
#define MINSG_TRIANGULATION_HELPER_H

#include "Delaunay2d.h"
#include "Delaunay3d.h"
#include "TetrahedronWrapper.h"
#include "../../Core/Nodes/ListNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/States/MaterialState.h"
#include <Rendering/Mesh/Mesh.h>
#include <Util/Graphics/Color.h>

namespace MinSG {
//! @ingroup ext
namespace Triangulation {

//! Create a mesh for a 2D triangle
MINSGAPI Rendering::Mesh * createTriangle2DMesh(const Geometry::Vec2f & a, const Geometry::Vec2f & b, const Geometry::Vec2f & c);

//! Create a mesh for a tetrahedron
MINSGAPI Rendering::Mesh * createTetrahedronMesh(const Geometry::Tetrahedron<float> & tetrahedron);

template<typename Point_t>
struct NodeGenerator2D {
	Util::Reference<ListNode> container;
	NodeGenerator2D() :
		container(new ListNode) {
		Util::Reference<MaterialState> material = new MaterialState();
		material->changeParameters().setAmbient(Util::Color4f(0.2f, 0.2f, 0.2f, 1.0f));
		material->changeParameters().setDiffuse(Util::Color4f(0.8f, 0.8f, 0.8f, 1.0f));
		material->changeParameters().setSpecular(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
		material->changeParameters().setShininess(128.0f);
		container->addState(material.get());
	}

	void operator()(const Point_t & a, const Point_t & b, const Point_t & c) {
		Rendering::Mesh * mesh = createTriangle2DMesh(a.getPosition(), b.getPosition(), c.getPosition());
		container->addChild(new GeometryNode(mesh));
	}
};

template<typename Point_t>
ListNode * createMinSGNodes(const Delaunay2d<Point_t> & triangulation) {
	NodeGenerator2D<Point_t> generator;
	triangulation.generate(generator);
	return generator.container.detachAndDecrease();
}

template<typename Point_t>
struct NodeGenerator3D {
		Util::Reference<ListNode> container;
		bool m_skipIfDegenerated;

		NodeGenerator3D(bool p_skipIfDegenerated) :
			container(new ListNode), m_skipIfDegenerated(p_skipIfDegenerated) {
			Util::Reference<MaterialState> material = new MaterialState();
			material->changeParameters().setAmbient(Util::Color4f(0.2f, 0.2f, 0.2f, 1.0f));
			material->changeParameters().setDiffuse(Util::Color4f(0.8f, 0.8f, 0.8f, 1.0f));
			material->changeParameters().setSpecular(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
			material->changeParameters().setShininess(128.0f);
			container->addState(material.get());
		}

		void operator()(const TetrahedronWrapper<Point_t> & t) {
			if(m_skipIfDegenerated){
				Geometry::Box bb;
				bb.invalidate();
				bb.include(t.getA()->getPosition());
				bb.include(t.getB()->getPosition());
				bb.include(t.getC()->getPosition());
				bb.include(t.getD()->getPosition());
				
				const float bbSide = bb.getDiameter();
				if(bb.getVolume()==0)
					return;
				
				const float quality = (t.getTetrahedron().calcVolume()/bbSide);
				if(quality<0.0002 || bb.getExtentMax() > bb.getExtentMin()*10.0)
					return;
			} 
			container->addChild(new GeometryNode(createTetrahedronMesh(t.getTetrahedron())));
		}
	};

template<typename Point_t>
ListNode * createMinSGNodes(Delaunay3d<Point_t> & triangulation, bool skipIfDegenerated) {
	NodeGenerator3D<Point_t> generator(skipIfDegenerated);
	triangulation.generate(generator);
	return generator.container.detachAndDecrease();
}

}
}

#endif /* MINSG_TRIANGULATION_HELPER_H */
#endif /* MINSG_EXT_TRIANGULATION */
