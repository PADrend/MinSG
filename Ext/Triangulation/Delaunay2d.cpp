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

#include "Delaunay2d.h"

#include <Geometry/Vec2.h>
#include <Util/Macros.h>
#include <utility>

namespace MinSG {
namespace Triangulation {

class D2D_Node {
	WrapperPoint point;
	D2D_Edge * anEdge;

	public:
		D2D_Node(const WrapperPoint & _point): point(_point), anEdge(nullptr)		{	}

		D2D_Edge * getAnEdge()const		{	return anEdge;	}

		double distanceSquared(double px, double py)const {
			const double dx = px - getX();
			const double dy = py - getY();
			return dx * dx + dy * dy;
		}

		double getX()const				{	return point.first.x();	}
		double getY()const				{	return point.first.y();	}
		const Geometry::Vec2f & getPosition() const {
			return point.first;
		}
		const WrapperPoint & getPoint() const {
			return point;
		}

		void setAnEdge(D2D_Edge * e)	{	anEdge = e;	}


};

class D2D_Edge {
		// start and end point of the edge
		D2D_Node *       p1, * p2;
		// inverse edge (p2->p1)
		D2D_Edge *       invE;
		// next edge in the triangle in countclickwise
		D2D_Edge *       nextE;
		// convex hull link
		D2D_Edge *       nextH;
		// triangle containing this edge
		D2D_Triangle *   inT;
		// line equation parameters. aX+bY+c=0
		double a, b, c;

	public:
		// Constructor
		D2D_Edge(D2D_Node * _p1, D2D_Node * _p2): p1(nullptr), p2(nullptr), invE(nullptr), nextH(nullptr), inT(nullptr) {
			update(_p1, _p2);
		}

		// update method
		void update(D2D_Node * _p1, D2D_Node * _p2) {
			this->p1 = _p1;
			this->p2 = _p2;
			recalculateABC();
			useAsIndex();
		}
		D2D_Node * getP1()const					{	return p1;	}
		D2D_Node * getP2()const					{	return p2;	}
		D2D_Edge * getInverseEdge()const		{	return invE;	}
		D2D_Edge * getNextEdge()const			{	return nextE;	}
		D2D_Edge * getNextHullEdge()const		{	return nextH;	}
		D2D_Triangle * getTriangle()const		{	return inT;	}
		void setNextEdge(D2D_Edge * e) 			{	nextE = e;	}
		void setNextHullEdge(D2D_Edge * e) 		{	nextH = e;}
		void setTriangle(D2D_Triangle * t)		{	inT = t;	}
		void setInvE(D2D_Edge * e)				{	invE = e;	}

		D2D_Edge * createInverseEdge() {
			auto e = new D2D_Edge(p2, p1);
			setInverseEdge(e);
			return e;
		}

		void setInverseEdge(D2D_Edge * e) {
			this->invE = e;
			if(e != nullptr)
				e->invE = this;
		}

		int onSide(D2D_Node * node) {
			const double s = a * node->getX() + b * node->getY() + c;
			if(s > 0.0) return 1;
			if(s < 0.0) return -1;
			return 0;
		}

		// Method for setting parameters of a,b,c
		void recalculateABC() {
			a = p2->getY() - p1->getY();
			b = p1->getX() - p2->getX();
			c = p2->getX() * p1->getY() - p1->getX() * p2->getY();
		}
		void useAsIndex() {
			p1->setAnEdge( this );
		}

		D2D_Edge * getMostLeft() {
			D2D_Edge * ee, * e = this;
			while((ee = e->getNextEdge()->getNextEdge()->getInverseEdge()) != nullptr && ee != this)
				e = ee;
			return e->getNextEdge()->getNextEdge();
		}

		D2D_Edge * getMostRight() {
			D2D_Edge * ee, * e = this;
			while(e->getInverseEdge() != nullptr && (ee = e->getInverseEdge()->getNextEdge()) != this)
				e = ee;
			return e;
		}
};

class D2D_Triangle {
		D2D_Edge *  anEdge;
		double  circumCirX;
		double  circumCirY;
		double  circumCirR2;

	public:
		D2D_Triangle(D2D_Edge * e1, D2D_Edge * e2, D2D_Edge * e3) {
			update(e1, e2, e3);
		}
		D2D_Triangle( D2D_Edge * e1, D2D_Edge * e2, D2D_Edge * e3, std::set<D2D_Edge *> & edgeRegistry) {
			update(e1, e2, e3);
			edgeRegistry.insert(e1);
			edgeRegistry.insert(e2);
			edgeRegistry.insert(e3);
		}
		void update(D2D_Edge * e1, D2D_Edge * e2, D2D_Edge * e3) {
			anEdge = e1;
			e1->setNextEdge(e2);
			e2->setNextEdge(e3);
			e3->setNextEdge(e1);
			e1->setTriangle(this);
			e2->setTriangle(this);
			e3->setTriangle(this);
			recalculateCircle();
		}
		D2D_Edge * getFirstEdge()const {
			return anEdge;
		}
		bool inCircle(D2D_Node * node)const {
			return node->distanceSquared(circumCirX, circumCirY) < circumCirR2;
		}
		void removeEdges(std::set<D2D_Edge *> & edgeRegistry)const {
			edgeRegistry.erase(anEdge);
			edgeRegistry.erase(anEdge->getNextEdge());
			edgeRegistry.erase(anEdge->getNextEdge()->getNextEdge());
		}
		void recalculateCircle() {
			const double x1 = anEdge->getP1()->getX();
			const double y1 = anEdge->getP1()->getY();
			const double x2 = anEdge->getP2()->getX();
			const double y2 = anEdge->getP2()->getY();
			const double x3 = anEdge->getNextEdge()->getP2()->getX();
			const double y3 = anEdge->getNextEdge()->getP2()->getY();
			const double a = (y2 - y3) * (x2 - x1) - (y2 - y1) * (x2 - x3);
			const double a1 = (x1 + x2) * (x2 - x1) + (y2 - y1) * (y1 + y2);
			const double a2 = (x2 + x3) * (x2 - x3) + (y2 - y3) * (y2 + y3);
			circumCirX = (a1 * (y2 - y3) - a2 * (y2 - y1)) / a / 2;
			circumCirY = (a2 * (x2 - x1) - a1 * (x2 - x3)) / a / 2;
			circumCirR2 = anEdge->getP1()->distanceSquared(circumCirX, circumCirY);
		}
//		void DrawCircles(Graphics g) {
//			int x0, y0, x, y;
//			x0 = (int) (circumCirX - circumCirR);
//			y0 = (int) (circumCirY - circumCirR);
//			x =  (int) (2.0 * circumCirR);
//			y =  (int) (2.0 * circumCirR);
//			g.drawOval((int)(circumCirX - circumCirR), (int)(circumCirY - circumCirR), (int)(2.0 * circumCirR), (int)(2.0 * circumCirR));
//		}
};


void D2D::clear() {
	for(auto & elem : nodes)
		delete elem;
	nodes.clear();

	for(auto & elem : edges)
		delete elem;
	edges.clear();

	for(auto & elem : triangles)
		delete elem;
	triangles.clear();
}

void D2D::addPoint(const WrapperPoint & point) {
	auto node = new D2D_Node(point);
	nodes.push_back(node);
	if(nodes.size() < 3) {
		return;
	} else if(nodes.size() == 3) { // create the first triangle
		D2D_Node * p1 = nodes[0];
		D2D_Node * p2 = nodes[1];
		D2D_Node * p3 = nodes[2];
		auto e1 = new D2D_Edge(p1, p2);
		if(e1->onSide(p3) == 0) {
			WARN("Duplicate point.");
			nodes.pop_back(); // = remove(node);
			delete e1;
			delete node;
			return;
		}
		if(e1->onSide(p3) == -1) { // right side
			p1 = nodes[1];
			p2 = nodes[0];
			e1->update(p1, p2);
		}
		auto e2 = new D2D_Edge(p2, p3);
		auto e3 = new D2D_Edge(p3, p1);
		e1->setNextHullEdge(e2);
		e2->setNextHullEdge(e3);
		e3->setNextHullEdge(e1);
		hullStart = e1;
		triangles.insert(new D2D_Triangle(e1, e2, e3, edges));
		return;
	} else {
		int eid;

		D2D_Edge * const startingEdge = *edges.begin();// start with some edge
		D2D_Edge * foundEdge = startingEdge;// edges[0];
		if(startingEdge->onSide(node) == -1) {
			if(startingEdge->getInverseEdge() == nullptr) {
				eid = -1;
			} else {
				eid = searchEdge(startingEdge->getInverseEdge(), node, foundEdge);
			}
		} else {
			eid = searchEdge(startingEdge, node, foundEdge);
		}
		if(eid == 0) {
			WARN("Duplicate point.");
			nodes.pop_back(); // = remove(node);
			delete node;
			return;
		}
		if(eid > 0) {
			expandTri(foundEdge, node, eid); // node is inside or on a triangle
		} else {
			expandHull(node);                // node is outside convex hull
		}

	}
}

void D2D::expandTri(D2D_Edge * e, D2D_Node * node, int type) {
	D2D_Edge * e1 = e;
	D2D_Edge * e2 = e1->getNextEdge();
	D2D_Edge * e3 = e2->getNextEdge();
	D2D_Node * p1 = e1->getP1();
	D2D_Node * p2 = e2->getP1();
	D2D_Node * p3 = e3->getP1();
	if(type == 2) { // node is inside of the triangle
		auto e10 = new D2D_Edge(p1, node);
		auto e20 = new D2D_Edge(p2, node);
		auto e30 = new D2D_Edge(p3, node);
		e->getTriangle()->removeEdges(edges);
		triangles.erase(e->getTriangle());     // remove old triangle
		// \todo delete the triangle

		triangles.insert(new D2D_Triangle(e1, e20, e10->createInverseEdge(), edges));
		triangles.insert(new D2D_Triangle(e2, e30, e20->createInverseEdge(), edges));
		triangles.insert(new D2D_Triangle(e3, e10, e30->createInverseEdge(), edges));
		swapTest(e1);   // swap test for the three new triangles
		swapTest(e2);
		swapTest(e3);
	} else {       // node is on the edge e
		D2D_Edge * e4 = e1->getInverseEdge();
		if(e4 == nullptr || e4->getTriangle() == nullptr) { // one triangle involved
			auto e30 = new D2D_Edge(p3, node);
			auto e02 = new D2D_Edge(node, p2);
			auto e10 = new D2D_Edge(p1, node);
			D2D_Edge * e03 = e30->createInverseEdge();
			e10->useAsIndex();
			e1->getMostLeft()->setNextHullEdge(e10);
			e10->setNextHullEdge(e02);
			e02->setNextHullEdge(e1->getNextHullEdge());
			hullStart = e02;
			triangles.erase(e1->getTriangle()); // remove oldtriangle
			// \todo delete the triangle

			// add two new triangles
			edges.erase(e1);
			edges.insert(e10);
			edges.insert(e02);
			edges.insert(e30);
			edges.insert(e03);
			triangles.insert(new D2D_Triangle(e2, e30, e02));
			triangles.insert(new D2D_Triangle(e3, e10, e03));
			swapTest(e2);   // swap test for the two new triangles
			swapTest(e3);
			swapTest(e30);
		} else {     // two triangle involved
			D2D_Edge * e5 = e4->getNextEdge();
			D2D_Edge * e6 = e5->getNextEdge();
			D2D_Node * p4 = e6->getP1();
			auto e10 = new D2D_Edge(p1, node);
			auto e20 = new D2D_Edge(p2, node);
			auto e30 = new D2D_Edge(p3, node);
			auto e40 = new D2D_Edge(p4, node);
			triangles.erase(e->getTriangle());  // remove oldtriangle
			// \todo delete the triangle

			e->getTriangle()->removeEdges(edges);
			triangles.erase(e4->getTriangle()); // remove old triangle
			// \todo delete the triangle

			e4->getTriangle()->removeEdges(edges);
			e5->useAsIndex();   // because e, e4 removed, reset edge index of node p1 and p2
			e2->useAsIndex();
			triangles.insert(new D2D_Triangle(e2, e30, e20->createInverseEdge(), edges));
			triangles.insert(new D2D_Triangle(e3, e10, e30->createInverseEdge(), edges));
			triangles.insert(new D2D_Triangle(e5, e40, e10->createInverseEdge(), edges));
			triangles.insert(new D2D_Triangle(e6, e20, e40->createInverseEdge(), edges));
			swapTest(e2);   // swap test for the three new triangles
			swapTest(e3);
			swapTest(e5);
			swapTest(e6);
			swapTest(e10);
			swapTest(e20);
			swapTest(e30);
			swapTest(e40);
		}
	}
}

void D2D::expandHull(D2D_Node * node) {
	D2D_Edge * e3 = nullptr;
	D2D_Edge * currentEdge = hullStart;
	D2D_Edge * comedge = nullptr;
	D2D_Edge * lastbe = nullptr;
	while(true) {
		D2D_Edge * edgeNext = currentEdge->getNextHullEdge();
		if(currentEdge->onSide(node) == -1) { // right side
			if(lastbe != nullptr) {
				D2D_Edge * e1 = currentEdge->createInverseEdge();
				auto e2 = new D2D_Edge(currentEdge->getP1(), node);
				e3 = new D2D_Edge(node, currentEdge->getP2());
				if(comedge == nullptr) {
					hullStart = lastbe;
					lastbe->setNextHullEdge(e2);
					lastbe = e2;
				} else {
					comedge->setInverseEdge(e2);
				}
				comedge = e3;
				triangles.insert(new D2D_Triangle(e1, e2, e3, edges));
				swapTest(currentEdge);
			}
		} else {
			if(comedge != nullptr) break;
			lastbe = currentEdge;
		}
		currentEdge = edgeNext;
	}

	lastbe->setNextHullEdge(e3);
	e3->setNextHullEdge(currentEdge);
}

int D2D::searchEdge(D2D_Edge * e, D2D_Node * node, D2D_Edge * & foundEdge)const {

	const int f2 = e->getNextEdge()->onSide(node);
	if(f2 == -1) {
		if(e->getNextEdge()->getInverseEdge() != nullptr) {
			return searchEdge(e->getNextEdge()->getInverseEdge(), node, foundEdge);
		} else {
			foundEdge = e;
			return -1;
		}
	}
	D2D_Edge * const ee = e->getNextEdge();

	const int f3 = ee->getNextEdge()->onSide(node);
	if( f3 == -1) {
		if(ee->getNextEdge()->getInverseEdge() != nullptr) {
			return searchEdge(ee->getNextEdge()->getInverseEdge(), node, foundEdge);
		} else {
			foundEdge = ee->getNextEdge();
			return -1;
		}
	}
	D2D_Edge * e0 = nullptr;
	if(f2 == 0)
		e0 = e->getNextEdge();
	if(f3 == 0)
		e0 = ee->getNextEdge();
	if(e->onSide(node) == 0)
		e0 = e;
	if(e0 != nullptr) {
		foundEdge = e0;
		if(e0->getNextEdge()->onSide(node) == 0) {
			foundEdge = e0->getNextEdge();
			return 0;
		} else if(e0->getNextEdge()->getNextEdge()->onSide(node) == 0) {
			return 0;
		} else {
			return 1;
		}
	}
	foundEdge = ee;
	return 2;
}

void D2D::swapTest(D2D_Edge * e11) {
	D2D_Edge * e21 = e11->getInverseEdge();
	if(e21 == nullptr || e21->getTriangle() == nullptr) return;
	D2D_Edge * e12 = e11->getNextEdge();
	D2D_Edge * e13 = e12->getNextEdge();
	D2D_Edge * e22 = e21->getNextEdge();
	D2D_Edge * e23 = e22->getNextEdge();
	if(e11->getTriangle()->inCircle(e22->getP2()) || e21->getTriangle()->inCircle(e12->getP2())) {
		e11->update(e22->getP2(), e12->getP2());
		e21->update(e12->getP2(), e22->getP2());
		e11->setInverseEdge(e21);
		e13->getTriangle()->update(e13, e22, e11);
		e23->getTriangle()->update(e23, e12, e21);
		e12->useAsIndex();
		e22->useAsIndex();
		swapTest(e12);
		swapTest(e22);
		swapTest(e13);
		swapTest(e23);
	}
}

void D2D::generate(D2D::generatorFunction_t generator) const {
	for(const auto & triangle : triangles) {
		generator(triangle->getFirstEdge()->getP1()->getPoint(),
				  triangle->getFirstEdge()->getNextEdge()->getP1()->getPoint(),
				  triangle->getFirstEdge()->getNextEdge()->getNextEdge()->getP1()->getPoint());
	}
}

}
}
#endif /* MINSG_EXT_TRIANGULATION */
