/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef VALUATEDREGIONNODE_H
#define VALUATEDREGIONNODE_H

#include "../../Core/Nodes/ListNode.h"
#include <Util/Graphics/Color.h>
#include <memory>
#include <list>

namespace MinSG {

/**
 *  ValuatedRegionNode ---|> ListNode ---|> GroupNode ---|> Node
 * @ingroup nodes
 */
class ValuatedRegionNode : public ListNode {
		PROVIDES_TYPE_NAME(ValuatedRegionNode)
	public:
		// additional Renderingflag
		static const renderFlag_t NO_BLENDING = 1 << 16;

	// ----------

	//! @name Main
	//	@{
		ValuatedRegionNode(Geometry::Box _region, Geometry::Vec3i resolution);
		ValuatedRegionNode(const ValuatedRegionNode & cn);
		virtual ~ValuatedRegionNode();

		/**
		 * Create new ValuatedRegionNodes inside the current region.
		 * These new nodes are automatically added as children to the current node.
		 * The parameters are checked to have reasonable values.
		 * Only leaf nodes can be split.
		 *
		 * @param regionsX Number of resulting nodes in X direction. At most the resolution in X direction.
		 * @param regionsY Number of resulting nodes in Y direction. At most the resolution in Y direction.
		 * @param regionsZ Number of resulting nodes in Z direction. At most the resolution in Z direction.
		 */
		void splitUp(unsigned int regionsX, unsigned int regionsY, unsigned int regionsZ);
		bool isLeaf()const 			{	return countChildren() == 0;	}

		//! ---o
		ValuatedRegionNode * createNewNode(const Geometry::Box & _region, const Geometry::Vec3i & resolution) const;

		//! ---|> GroupNode
		void doAddChild(Util::Reference<Node> child) override;
	private:
		//! ---|> Node
		ValuatedRegionNode * doClone()const override	{	return new ValuatedRegionNode(*this);	}
		const Geometry::Box& doGetBB() const override	{	return region;	}

		class ClassificationExporter;
	// @}

	// ----------

	//! @name Display
	//	@{
	public:
		//! ---|> Node
		void doDisplay(FrameContext & context, const RenderParam & rp) override;

	protected:
		Geometry::Box region;
		void drawColorBox(FrameContext & context);

	// @}

	// ----------

	//! @name Value
	//	@{
	private:
		std::unique_ptr<Util::GenericAttribute> value;
	public:
		void setValue(Util::GenericAttribute * value);

		//! Set the value to nullptr without deleting the old value first.
		void clearValue() {
			value.release();
		}

		Util::GenericAttribute * getValue()const 	{	return value.get();	}
		Util::GenericAttribute * getValueAtPosition(const Geometry::Vec3 & absPos);
		ValuatedRegionNode * getNodeAtPosition(const Geometry::Vec3 & absPos);

		/*! ---o
			Convert the value of the region into a single number (e.g. for color computation).
			By default, the value is expected to be one single number or a list of numbers
			and the average value is returned.	*/
		float getValueAsNumber() const;

		/*! ---o
			Convert the value of the region into a list of numbers (e.g. for color computation
			for different directions).
			By default, the value is expected to be one single number or a list of numbers
			which are added to numbers without any computations.	*/
		void getValueAsNumbers(std::list<float> & numbers) const;
	// @}

	// ----------

	//! @name Grid
	//	@{
	private:
		Geometry::Vec3i resolution;
	public:
		const Geometry::Vec3i & getResolution()const	{	return resolution;	}
		int getXResolution()const 						{	return resolution.x();	}
		int getYResolution()const						{	return resolution.y();	}
		int getZResolution()const 						{	return resolution.z();	}
		int getSize()const 								{	return getXResolution()*getYResolution()*getZResolution();	}
		Geometry::Vec3 getPosition(float xCell, float yCell, float zCell) const;

	// @}

	// ----------

	//! @name Colors
	//	@{
	private:
		struct additional_data_t {
			std::list<Util::Color4f> colors;
			float heightScale;
		};
		//! Extra struct to save memory in case it is not needed.
		std::unique_ptr<additional_data_t> additionalData;
	public:
		const additional_data_t * getAdditionalData() const {	return additionalData.get();	}

	public:
		void addColor(float r, float g, float b, float a);
		void clearColors();
		void setHeightScale(float s);
		float getHeightScale() const;
	// @}


};
}
#endif // VALUATEDREGIONNODE_H
