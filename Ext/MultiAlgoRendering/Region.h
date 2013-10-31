/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef REGION_H
#define REGION_H

#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"
#include "Utils.h"

#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Rendering/Draw.h>
#include <Util/AttributeProvider.h>
#include <Util/GenericAttribute.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Macros.h>
#include <queue>


namespace MinSG {
namespace MAR {

class Region : public Util::ReferenceCounter<Region>, public Util::AttributeProvider {

	private:
		class SortB2F {
				Geometry::Vec3 pos;
			public:
				SortB2F(const Geometry::Vec3f & _pos) {
					pos = _pos;
				}
				bool operator()(const Region * a, const Region * b) const {
					return a->bounds.getDistanceSquared(pos) < b->bounds.getDistanceSquared(pos);
				}
		};

	public:

		Region(const Geometry::Box & _bounds, Region * _parent) :
			ReferenceCounter_t(), Util::AttributeProvider(),
			color(Util::ColorLibrary::WHITE), bounds(_bounds), parent(_parent) {
			children.shrink_to_fit();
		}

		size_t treeSize() const {
			size_t x = 1;
			for(const auto & child : children)
				x += child->treeSize();
			return x;
		}

		Region * getRoot() {
			if(parent.isNull())
				return this;
			return parent->getRoot();
		}

		const Geometry::Box & getBounds() const {
			return bounds;
		}

		const Util::Color4ub & getColor() const {
			return color;
		}

		static const Geometry::Box & getBounds2(const Region* region) {
			return region->getBounds();
		}

		static const Util::Color4f getColor2(const Region* region) {
			return region->getColor();
		}

		const std::vector<ref_t> & getChildren() const {
			return children;
		}
private:
		void getLeaveRegions(std::deque<const Region*> & regions) const{
			if(hasChildren())
				for(const auto & child : children)
					child->getLeaveRegions(regions);
			else
				regions.push_back(this);
		}
public:
		/**
			* displays the boundingboxes of all leaves in back to front order
			*/
		void display(FrameContext & frameContext, float alpha) const{
			std::deque<const Region *> regions;
			getLeaveRegions(regions);
			std::sort(begin(regions), end(regions), Region::SortB2F(frameContext.getCamera()->getWorldPosition()));
			debugDisplay<std::deque<const Region*>>(regions, frameContext, alpha, &getBounds2, &getColor2);
		}

		void split(const uint32_t x = 2, const uint32_t y = 2, const uint32_t z = 2) {
			if(hasChildren()) {
				WARN("region has already children");
				return;
			}
			const auto tmp = Geometry::Helper::splitUpBox(bounds, x, y, z);
			for(const auto & box : tmp) {
				children.emplace_back(new Region(box, this));
			}
			children.shrink_to_fit();
		}

		void splitCubeLike() {
			if(hasChildren()) {
				WARN("region has already children");
				return;
			}
			const auto tmp = Geometry::Helper::splitBoxCubeLike(bounds);
			for(const auto & box : tmp) {
				children.emplace_back(new Region(box, this));
			}
			children.shrink_to_fit();
		}

		void split(uint32_t axis, float ratio) {
			if(hasChildren()) {
				WARN("region has already children");
				return;
			}
			Geometry::Vec3f mini(bounds.getMinX(), bounds.getMinY(), bounds.getMinZ());
			Geometry::Vec3f maxi(bounds.getMaxX(), bounds.getMaxY(), bounds.getMaxZ());
			Geometry::Vec3f mini2(maxi);
			Geometry::Vec3f maxi2(mini);
			float f;
			switch(axis) {
				case 0:
					f = mini.getX()+bounds.getExtentX() * ratio;
					mini2.setX(f);
					maxi2.setX(f);
					break;
				case 1:
					f = mini.getY()+bounds.getExtentY() * ratio;
					mini2.setY(f);
					maxi2.setY(f);
					break;
				case 2:
					f = mini.getZ()+bounds.getExtentZ() * ratio;
					mini2.setZ(f);
					maxi2.setZ(f);
					break;
				default:
					FAIL();
			}
			children.emplace_back(new Region(Geometry::Box(mini,mini2), this));
			children.emplace_back(new Region(Geometry::Box(maxi,maxi2), this));
			children.shrink_to_fit();
		}

		std::string toString() const {
			std::stringstream ss;
			ss << "Region:[ Depth:" << getDepth() << " , " << bounds << " , " << color.toString() << " ]";
			return ss.str();
		}

		void setColor(const Util::Color4ub & _color) {
			this->color = _color;
		}

		Region * getParent() const {
			return parent.get();
		}

		bool hasChildren() const { return !children.empty(); }

		uint32_t getDepth() const {
			if(parent.isNull())
				return 0;
			return parent->getDepth() + 1;
		}

	protected:

	private:
		Util::Color4ub color;
		Geometry::Box bounds;
		std::vector<ref_t> children;
		ref_t parent;
};
}
}

#endif // REGION_H
#endif // MINSG_EXT_MULTIALGORENDERING
