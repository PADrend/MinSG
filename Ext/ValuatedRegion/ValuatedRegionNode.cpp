/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ValuatedRegionNode.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include "../../Core/FrameContext.h"
#include "../../Core/States/State.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Geometry/BoxHelper.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <sstream>
#include <stack>
#include <stdexcept>

using namespace Util;

namespace MinSG {
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
using namespace VisibilitySubdivision;
#endif

ValuatedRegionNode::ValuatedRegionNode(Geometry::Box  _region, Geometry::Vec3i  _resolution):
		ListNode(),
		region(std::move(_region)), value(), resolution(std::move(_resolution)), additionalData() {
	setClosed(true);
	//ctor
}

ValuatedRegionNode::ValuatedRegionNode(const ValuatedRegionNode & cn) : ListNode(cn),
		region(cn.region), value(), resolution(cn.resolution), additionalData() {
	setClosed(true);
	setValue(cn.value.get());
	if(cn.additionalData) {
		additionalData.reset(new additional_data_t);
		additionalData->colors.assign(cn.additionalData->colors.begin(), cn.additionalData->colors.end());
		additionalData->heightScale = cn.additionalData->heightScale;
	}
}

ValuatedRegionNode::~ValuatedRegionNode() = default;

void ValuatedRegionNode::setValue(GenericAttribute * newValue) {
	if(newValue != value.get()) {
		value.reset(newValue);
	}
}

void ValuatedRegionNode::addColor(float r, float g, float b, float a) {
	if(!additionalData) {
		additionalData.reset(new additional_data_t);
		additionalData->heightScale = 1.0f;
	}
	additionalData->colors.emplace_back(r, g, b, a);
}
void ValuatedRegionNode::clearColors() {
	if(additionalData) {
		additionalData->colors.clear();
	}
}
void ValuatedRegionNode::setHeightScale(float s) {
	if(!additionalData) {
		additionalData.reset(new additional_data_t);
	}
	additionalData->heightScale = s;
}
float ValuatedRegionNode::getHeightScale() const {
	if (additionalData == nullptr) {
		return 1.0f;
	}
	return additionalData->heightScale;
}

//! ---o
ValuatedRegionNode * ValuatedRegionNode::createNewNode(const Geometry::Box & _region, const Geometry::Vec3i & _resolution) const {
	return new ValuatedRegionNode(_region ,_resolution);
}

void ValuatedRegionNode::splitUp(unsigned int regionsX, unsigned int regionsY, unsigned int regionsZ) {
	if (!isLeaf()) {
		WARN("Cannot split inner node.");
		return;
	}
	if (regionsX == 0 || regionsY == 0 || regionsZ == 0) {
		WARN("Number of regions requested in at least one direction is zero.");
		return;
	}
	if (static_cast<int>(regionsX) > resolution.x()) {
		WARN("Number of regions requested in X direction is greater than X resolution.");
		return;
	}
	if (static_cast<int>(regionsY) > resolution.y()) {
		WARN("Number of regions requested in Y direction is greater than Y resolution.");
		return;
	}
	if (static_cast<int>(regionsZ) > resolution.z()) {
		WARN("Number of regions requested in Z direction is greater than Z resolution.");
		return;
	}
	if (resolution.x() % regionsX != 0) {
		WARN("X resolution is not divisible by number of regions requested in X direction.");
		return;
	}
	if (resolution.y() % regionsY != 0) {
		WARN("Y resolution is not divisible by number of regions requested in Y direction.");
		return;
	}
	if (resolution.z() % regionsZ != 0) {
		WARN("Z resolution is not divisible by number of regions requested in Z direction.");
		return;
	}

	const Geometry::Vec3i newResolution(resolution.x() / regionsX, resolution.y() / regionsY, resolution.z() / regionsZ);

	const auto childBoxes = Geometry::Helper::splitUpBox(getBB(), regionsX, regionsY, regionsZ);
	for(const auto & childBox : childBoxes) {
		ValuatedRegionNode * vrNode = createNewNode(childBox, newResolution);
		addChild(vrNode);
	}
}

Geometry::Vec3 ValuatedRegionNode::getPosition(float xCell, float yCell, float zCell) const {
	return Geometry::Vec3(
				region.getMinX() + (region.getExtentX() / resolution.x())*(xCell + 0.5),
				region.getMinY() + (region.getExtentY() / resolution.y())*(yCell + 0.5),
				region.getMinZ() + (region.getExtentZ() / resolution.z())*(zCell + 0.5)
		   );
}

GenericAttribute * ValuatedRegionNode::getValueAtPosition(const Geometry::Vec3 & absPos) {
	if (!getBB().contains(absPos)) {
		return nullptr;
	}
	if (!isLeaf()) {
		const auto childNodes = getChildNodes(this);
		for(const auto & childNode : childNodes) {
			ValuatedRegionNode * child = dynamic_cast<ValuatedRegionNode *>(childNode);
			if (child == nullptr) {
				continue;
			}
			GenericAttribute * result = child->getValueAtPosition(absPos);
			if (result != nullptr) {
				return result;
			}
		}
	}
	return getValue();
}

ValuatedRegionNode * ValuatedRegionNode::getNodeAtPosition(const Geometry::Vec3 & absPos) {
	if (!getBB().contains(absPos)) {
		return nullptr;
	}
	if (!isLeaf()) {
		const auto childNodes = getChildNodes(this);
		for(const auto & childNode : childNodes) {
			ValuatedRegionNode * child = dynamic_cast<ValuatedRegionNode *>(childNode);
			if (child == nullptr) {
				continue;
			}
			ValuatedRegionNode * result = child->getNodeAtPosition(absPos);
			if (result != nullptr) {
				return result;
			}
		}
	}
	return this;
}

//! ---|> GroupNode
void ValuatedRegionNode::doAddChild(Util::Reference<Node> child) {
	if (!dynamic_cast<ValuatedRegionNode*>(child.get())) {
		throw std::invalid_argument("ValuatedRegionNode can only contain other ValuatedRegionNodes");
	}
	setClosed(false);
	ListNode::doAddChild(child);
}

//! ---|> Node
void ValuatedRegionNode::doDisplay(FrameContext & context, const RenderParam & rp) {
	const bool iMode = context.getRenderingContext().getImmediateMode();

	context.getRenderingContext().setImmediateMode(true);

	if ( rp.getFlag(NO_BLENDING) ){
		uint8_t light = context.getRenderingContext().enableLight(Rendering::LightParameters());

		std::stack<ValuatedRegionNode *> todo;
		todo.push(this);
		while (!todo.empty()) {
			ValuatedRegionNode * vrNode = todo.top();
			todo.pop();
			if (vrNode == nullptr) {
				continue;
			} else if (vrNode->isLeaf()) {
				if(vrNode->additionalData != nullptr && !vrNode->additionalData->colors.empty() && vrNode->additionalData->colors.front().getA() != 0.0f){
					vrNode->drawColorBox(context);
				}
			} else {
				const auto childNodes = getChildNodes(vrNode);
				for( const auto & child : childNodes){
					ValuatedRegionNode * vrn = dynamic_cast<ValuatedRegionNode *>(child);
					if(vrn)
						todo.push( vrn );
				}
			}
		}
		context.getRenderingContext().disableLight(light);
	} // Blending (with sorting)
	else {
		context.getRenderingContext().pushAndSetBlending(Rendering::BlendingParameters(Rendering::BlendingParameters::SRC_ALPHA, Rendering::BlendingParameters::ONE_MINUS_SRC_ALPHA));
		context.getRenderingContext().pushAndSetCullFace( Rendering::CullFaceParameters::CULL_BACK );
		context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, false, Rendering::Comparison::LESS));
		context.getRenderingContext().pushAndSetLighting(Rendering::LightingParameters(false));
		context.getRenderingContext().applyChanges();

		std::vector< std::pair<float,ValuatedRegionNode*> > regions;
		const Geometry::Vec3 pos = context.getCamera()->getWorldPosition();

		std::stack<ValuatedRegionNode *> todo;
		todo.push(this);
		while (!todo.empty()) {
			ValuatedRegionNode * cn = todo.top();
			todo.pop();
			if (cn == nullptr) {
				continue;
			} else if (cn->isLeaf()) {
				if(cn->additionalData != nullptr && !cn->additionalData->colors.empty() && cn->additionalData->colors.front().getA() != 0.0f){
					regions.emplace_back( -cn->getWorldBB().getDistanceSquared(pos),cn); // * -1 to sort from back to front!
				}
			} else {
				const auto childNodes = getChildNodes(cn);
				for( const auto & child : childNodes){
					ValuatedRegionNode * vrn = dynamic_cast<ValuatedRegionNode *>(child);
					if(vrn)
						todo.push( vrn );
				}
			}
		}
		std::sort(regions.begin(),regions.end());

		for( auto & distanceRegionPair : regions){
			distanceRegionPair.second->drawColorBox(context);
		}
		context.getRenderingContext().popLighting();
		context.getRenderingContext().popDepthBuffer();
		context.getRenderingContext().popCullFace();
		context.getRenderingContext().popBlending();
	}

	if (rp.getFlag(BOUNDING_BOXES)) {
		context.getRenderingContext().pushAndSetColorBuffer(Rendering::ColorBufferParameters(false, false, false, false));
		context.getRenderingContext().pushAndSetPolygonOffset(Rendering::PolygonOffsetParameters(1.0, 1.0));

		std::stack<ValuatedRegionNode *> s;
		s.push(this);
		while (!s.empty()) {
			ValuatedRegionNode * cn = s.top();
			s.pop();
			if (cn == nullptr) {
				continue;
			} else if (cn->isLeaf()) {
				if (cn->additionalData != nullptr) {
					Geometry::Box regionBB = cn->getBB();
					if(cn->additionalData->heightScale != 1.0f) {
						regionBB.setMaxY(regionBB.getMinY()+regionBB.getExtentY()*cn->additionalData->heightScale);
					}
					if (!cn->additionalData->colors.empty() && cn->additionalData->colors.front().getA() != 0.0f) {
						Rendering::drawBox(context.getRenderingContext(), regionBB);
					}
				}
			} else {
				const size_t childCount = cn->countChildren();
				for (size_t i = 0; i < childCount; ++i) {
					s.push(dynamic_cast<ValuatedRegionNode *>(cn->getChild(i)));
				}
			}
		}
		context.getRenderingContext().popPolygonOffset();
		context.getRenderingContext().popColorBuffer();
		context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::LEQUAL));

		s.push(this);
		while (!s.empty()) {
			ValuatedRegionNode * cn = s.top();
			s.pop();
			if (cn == nullptr) {
				continue;
			} else if (cn->isLeaf()) {
				if (cn->additionalData != nullptr) {
					Geometry::Box regionBB = cn->getBB();
					if(cn->additionalData->heightScale != 1.0f) {
						regionBB.setMaxY(regionBB.getMinY()+regionBB.getExtentY()*cn->additionalData->heightScale);
					}
					if (!cn->additionalData->colors.empty() && cn->additionalData->colors.front().getA() != 0.0f) {
						Rendering::drawWireframeBox(context.getRenderingContext(), regionBB, Util::ColorLibrary::BLACK);
					}
				}
			} else {
				const size_t childCount = cn->countChildren();
				for (size_t i = 0; i < childCount; ++i) {
					s.push(dynamic_cast<ValuatedRegionNode *>(cn->getChild(i)));
				}
			}
		}
		context.getRenderingContext().popDepthBuffer();
	}
	context.getRenderingContext().setImmediateMode(iMode);
}


void ValuatedRegionNode::drawColorBox(FrameContext & context) {
	if(additionalData == nullptr) {
		return;
	}
	unsigned int numColors = additionalData->colors.size();
	if (numColors == 1) {
		Geometry::Box box(region);
		if (additionalData->heightScale!=1.0) {
			box.setMaxY(box.getMinY()+box.getExtentY()*additionalData->heightScale);
		}
		Rendering::drawBox(context.getRenderingContext(), box, additionalData->colors.front());
	} else if (numColors == 6) {
		region.resizeRel(0.9);
		const float dispX = region.getExtentX() * 0.05f;
		const float dispY = region.getExtentY() * 0.05f;
		const float dispZ = region.getExtentZ() * 0.05f;

		/*
		 *     6---------7           /---------|
		 *    /|        /|          /   Top:  /|
		 *   / |       / |         /   4     / |--Back:5
		 *  2---------3  |  Left: ----------|  |
		 *  |  |      |  |    0---|         | ----Right:3
		 *  |  4------|--5        |  Front: |  |
		 *  | /       | /         |    2    | /
		 *  |/        |/          |         |/--Bottom:1
		 *  0---------1           |----------
		 */

		const Geometry::Vec3f corners[8] = {
			Geometry::Vec3f(region.getMinX(), region.getMinY(), region.getMinZ()),     // (0)
			Geometry::Vec3f(region.getMaxX(), region.getMinY(), region.getMinZ()),     // (1)
			Geometry::Vec3f(region.getMinX(), region.getMaxY(), region.getMinZ()),     // (2)
			Geometry::Vec3f(region.getMaxX(), region.getMaxY(), region.getMinZ()),     // (3)
			Geometry::Vec3f(region.getMinX(), region.getMinY(), region.getMaxZ()),     // (4)
			Geometry::Vec3f(region.getMaxX(), region.getMinY(), region.getMaxZ()),     // (5)
			Geometry::Vec3f(region.getMinX(), region.getMaxY(), region.getMaxZ()),     // (6)
			Geometry::Vec3f(region.getMaxX(), region.getMaxY(), region.getMaxZ())      // (7)
		};
		static const unsigned char index[] = {
			0, 2, 6, 4,     // (left)
			1, 3, 2, 0,     // (front)
			5, 7, 3, 1,     // (right)
			4, 6, 7, 5,     // (back)
			4, 5, 1, 0,     // (bottom)
			7, 6, 2, 3      // (top)
		};
		const float displacement[6][3] = {
			{-dispX, 0.0f, 0.0f},   // (left)
			{0.0f, 0.0f, -dispZ},   // (front)
			{dispX, 0.0f, 0.0f},    // (right)
			{0.0f, 0.0f, dispZ},    // (back)
			{0.0f, -dispY, 0.0f},   // (bottom)
			{0.0f, dispY, 0.0f}     // (top)
		};

		unsigned char i = 0;
		for (std::list<Util::Color4f>::const_iterator it = additionalData->colors.begin(); it != additionalData->colors.end(); ++it) {
			if (i>3) break;
			Geometry::Matrix4x4 translation;
			translation.translate(displacement[i][0], displacement[i][1], displacement[i][2]);
			context.getRenderingContext().pushMatrix();
			context.getRenderingContext().multMatrix(translation);
			Rendering::drawQuad(
				context.getRenderingContext(),
				corners[index[i * 4]],
				corners[index[i * 4 + 1]],
				corners[index[i * 4 + 2]],
				corners[index[i * 4 + 3]],
				*it
			);
			context.getRenderingContext().popMatrix();
			++i;
		}
	} else if (numColors != 0) {
		WARN(std::string("Got ") + Util::StringUtils::toString(numColors)
				+ " colors. Drawing of classifications with one or six colors is supported only.");
	}
}

//! ---o
float ValuatedRegionNode::getValueAsNumber() const {
	if(value==nullptr)
		return 0.0f;
	Util::GenericAttributeList* l=dynamic_cast<Util::GenericAttributeList*>(value.get());
	if(l==nullptr){
		return value->toFloat();
	}
	float accum=0.0f;
	for(auto & elem : *l){
		accum+=(elem)->toFloat();
	}
	return l->size()>0 ? accum/l->size() : 0.0f;
}

//! ---o
void ValuatedRegionNode::getValueAsNumbers(std::list<float> & numbers) const {
	if(value==nullptr)
		return;
	Util::GenericAttributeList* l=dynamic_cast<Util::GenericAttributeList*>(value.get());
	if(l==nullptr){
		numbers.push_back(value->toFloat());
		return;
	}
	for(auto & elem : *l){
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
		const auto * vva = dynamic_cast<const VisibilityVectorAttribute *>(elem.get());
		if(vva != nullptr) {
			numbers.push_back(vva->ref().getTotalCosts());
			continue;
		}
#endif
		numbers.push_back((elem)->toFloat());
	}
}

}
