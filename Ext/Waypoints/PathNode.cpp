/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_WAYPOINTS

#include "PathNode.h"

#include "../../Core/FrameContext.h"
#include "../../Helper/TextAnnotation.h"
#include "../../SceneManagement/Exporter/ExporterTools.h"
#include "../../SceneManagement/Importer/ImporterTools.h"
#include <Geometry/Vec2.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/StringUtils.h>
#include <Util/Macros.h>
#include <cmath>
#include <numeric>

//#include "FollowPathBehaviour.h"
#include "Waypoint.h"

namespace MinSG {

PathNode::PathNode() : 
		GroupNode(), 
		looping(true), 
		bbValid(false), 
		metaDisplayWaypoints(true),
		metaDisplayTimes(true) {
}

PathNode::PathNode(const PathNode & source):
		GroupNode(source), 
		looping(source.looping), 
		bbValid(false),
		metaDisplayWaypoints(source.metaDisplayWaypoints),
		metaDisplayTimes(source.metaDisplayTimes) {
	for(const auto & elem : source.waypoints) {
		createWaypoint(elem.second->getSRT(), elem.second->getTime());
	}
}

PathNode::~PathNode() = default;

//! ---|> Node
const Geometry::Box& PathNode::doGetBB() const {
	if(bbValid) {
		return bb;
	}
	bb.invalidate();
	for(const auto & elem : waypoints) {
		bb.include(elem.second->accessSRT().getTranslation());
	}
	bbValid = true;
	return bb;
}

Waypoint * PathNode::createWaypoint(const Geometry::SRT & position,AbstractBehaviour::timestamp_t time) {
	auto wp=new Waypoint(position,time);
	addChild(wp);
	return wp;
}

void PathNode::closeLoop(AbstractBehaviour::timestamp_t time) {
	if (countWaypoints()==0) {
		return;
	}
	Geometry::SRT pos=(*waypoints.begin()).second->getSRT();
	createWaypoint(pos,time);
}

void PathNode::removeLastWaypoint() {
	if(countWaypoints() == 0)
		return;
	removeChild( waypoints.rbegin()->second.get() );

	std::cout << waypoints.size();
	std::cout << " " << getMaxTime() << "\n";
}

PathNode::wayPointMap_t::const_iterator PathNode::getNextWaypoint(AbstractBehaviour::timestamp_t time)const {
	auto it=waypoints.begin();
	for (;it!=waypoints.end();++it) {
		if ( (*it).first > time )
			break;
	}
	return it;
}

PathNode::wayPointMap_t::iterator PathNode::getNextWaypoint(AbstractBehaviour::timestamp_t time) {
	auto it=waypoints.begin();
	for (;it!=waypoints.end();++it) {
		if ( (*it).first > time )
			break;
	}
	return it;
}

Geometry::SRT PathNode::getPosition(AbstractBehaviour::timestamp_t time)const {
	if (countWaypoints()==0)
		return Geometry::SRT();
	float maxTime=getMaxTime();
	if (time>maxTime && looping) {
		time-=floor(time/maxTime)*maxTime;
	}
	float minTime=countWaypoints()==0 ? 0 : waypoints.begin()->second->getTime();
	if(time<minTime && looping) {
		time+=ceil(-time/maxTime)*maxTime;
	}
	auto it=getNextWaypoint(time);
	if (it==waypoints.begin()) {
		return it->second->getSRT();
	}
	if (it==waypoints.end()) {
		--it;
		return it->second->getSRT();
	}
	Waypoint * w1=it->second.get();
	--it;
	Waypoint * w2=it->second.get();
	// TODO Check if Geometry::Interpolation can be used here
	float blend=1.0f-((w2->getTime()-time)/(w2->getTime()-w1->getTime()));

	return Geometry::SRT( w1->getSRT(),w2->getSRT(),blend);
}

Geometry::SRT PathNode::getWorldPosition(AbstractBehaviour::timestamp_t time) {
	return getWorldMatrix()._toSRT() * getPosition(time);
}

void PathNode::updateWaypoint(Waypoint * wp,AbstractBehaviour::timestamp_t newTime){
	if( wp==nullptr || wp->getPath()!=this ){
		WARN("Should not happen");
		return;
	}
	if(newTime == wp->getTime())
		return;

	Waypoint * oldWP=getWaypoint(newTime);
	if(oldWP!=nullptr && oldWP!=wp){
		WARN("Two waypoints with same time!");
		// todo Handle that later?!?!
		return;
	}
	// add at new position
	waypoints[newTime] = wp;

	// remove at old position
	waypoints.erase(wp->getTime());
	// set new time
	wp->time=newTime;

	worldBBChanged();
}

Waypoint * PathNode::getWaypoint(AbstractBehaviour::timestamp_t time){
	auto it=waypoints.find(time);
	if(it==waypoints.end())
		return nullptr;
	return it->second.get();
}

AbstractBehaviour::timestamp_t PathNode::getMaxTime()const{
	if(countWaypoints()==0)
		return 0;
	return waypoints.rbegin()->second->getTime();
}

//! ---|> GroupNode
void PathNode::doAddChild(Util::Reference<Node> child){
	Waypoint * wp=dynamic_cast<Waypoint*>(child.get());
	if (!wp ) {
		throw std::invalid_argument("PathNode::addChild: no Waypoint! ");
	}

	Waypoint * oldWP = getWaypoint(wp->getTime());
	if(oldWP!=nullptr && oldWP!=wp){
		WARN("Two waypoints with same time!");
		// todo Handle that later?!?!
		wp->time = getMaxTime()+1;
	}
	waypoints[wp->getTime()] = wp;
	wp->_setParent(this);
	worldBBChanged();
}

//! ---|> GroupNode
bool PathNode::doRemoveChild(Util::Reference<Node> child){
	Waypoint * wp=dynamic_cast<Waypoint*>(child.get());
	if (!wp) return false;

	auto it=waypoints.find(wp->getTime());
	if(it==waypoints.end())
		return false;

	waypoints.erase(it);
	wp->_setParent(nullptr);
	worldBBChanged();
	return true;
}


//! ---|> [Node]
NodeVisitor::status PathNode::traverse(NodeVisitor & visitor) {
	NodeVisitor::status status = visitor.enter(this);
	if (status == NodeVisitor::EXIT_TRAVERSAL) {
		return NodeVisitor::EXIT_TRAVERSAL;
	} else if (status == NodeVisitor::CONTINUE_TRAVERSAL) {
		for (auto & elem : waypoints) {
			Node * child = elem.second.get();
			if (child->traverse(visitor) == NodeVisitor::EXIT_TRAVERSAL) {
				return NodeVisitor::EXIT_TRAVERSAL;
			}
		}
	}
	return visitor.leave(this);
}

//! ---|> [GroupNode]
void PathNode::invalidateCompoundBB() {
	bbValid=false;
	metaMesh = nullptr;
}

// ! ---|> Node
void PathNode::doDisplay(FrameContext & context, const RenderParam & rp) {
	Rendering::RenderingContext & renderingContext = context.getRenderingContext();
	if(rp.getFlag(BOUNDING_BOXES)) {
		// worldBB
		Rendering::drawAbsWireframeBox(renderingContext, getWorldBB(), Util::ColorLibrary::LIGHT_GREY);
		// BB
		Rendering::drawWireframeBox(renderingContext, getBB(), Util::ColorLibrary::BLUE);
	}

	if(rp.getFlag(SHOW_META_OBJECTS) && !waypoints.empty()) {
		if(metaMesh.isNull()) {
			Rendering::VertexDescription vertexDesc;
			vertexDesc.appendPosition3D();

			Rendering::MeshUtils::MeshBuilder builder(vertexDesc);

			for(const auto & timeWaypointPair : waypoints) {
				const Geometry::Vec3f & wpPosition = timeWaypointPair.second->accessSRT().getTranslation();

				builder.position(wpPosition);
				builder.addVertex();
			}

			metaMesh = builder.buildMesh();
			metaMesh->setDrawMode(Rendering::Mesh::DRAW_LINE_STRIP);
		}
		renderingContext.displayMesh(metaMesh.get());

		// Display the children
		if(metaDisplayWaypoints) {
			for(const auto & timeWaypointPair : waypoints) {
				context.displayNode(timeWaypointPair.second.get(), rp);
			}
		}

		if(metaDisplayTimes) {
			for(const auto & timeWaypointPair : waypoints) {
				TextAnnotation::displayText(context,
											timeWaypointPair.second->getWorldBB().getCenter(),
											Geometry::Vec2i(0, -30),
											5,
											Util::Color4f(0, 0, 0, 1),
											context.getTextRenderer(),
											Util::StringUtils::toString(static_cast<int32_t>(round(timeWaypointPair.first))),
											Util::Color4f(1, 1, 1, 1));
			}
		}
	}
}

}

#endif /* MINSG_EXT_WAYPOINTS */
