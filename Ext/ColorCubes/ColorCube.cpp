/*
	This file is part of the MinSG library extension ColorCubes.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2010-2011 Paul Justus <paul.justus@gmx.net>
	Copyright (C) 2010-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_COLORCUBES

#include "ColorCube.h"
#include "ColorCubeGenerator.h"
#include "../States/EnvironmentState.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/States/AlphaTestState.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/PolygonModeState.h"
#include "../../Core/States/State.h"
#include "../../Core/FrameContext.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Helper/Helper.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Geometry/BoxHelper.h>
#include <Geometry/Rect.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshBuilder.h>

#include <Util/Graphics/ColorLibrary.h>
#include <Util/Encoding.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <Util/ProgressIndicator.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

using Geometry::Box;
using Geometry::Rect;
using Util::Color4f;

namespace MinSG {

/* ################################ class responsible for processing color cubes ##################### */

/**
 *	CCProcessingListener is responsible for processing of color cubes (this always happens at the end of a frame). It is
 *	implemented as a subclass of FrameContext::FrameListener. If a color cube of any node is required but does not
 *	exits while processing a frame, an instance of CCProcessinListener will be created and registered at current rendering
 *	context. Its member function onEndFrame(...), that do the processing of color cubes, will be called by rendering context
 *	at the end of the frame. The missing color cubes will be available in the next frame.
 */
class CCProcessingListener {
	private:
		/** reference to the node whose color cube should be processed */
		Util::Reference<Node> node;

		/** */
		uint32_t nodeCount;

		/** */
		uint32_t triangleCount;

	public:

		/** [ctor] stores the node whose color cube should be processed
		 * @param node to process
		 * @param nodeCount
		 * @param triangleCount */
		CCProcessingListener(Node * _node, uint32_t nCount, uint32_t tCount) : node(_node), nodeCount(nCount), triangleCount(tCount) {
		}

		/**
		 *	if the specified node stored in current CCProcessingListener-instance has not got a color cube, the required
		 *	processing data will be created, and all missing color cubes in the subtree will be recursively processed.
		 *
		 *	@param context current rendering context
		 *	@return true because current instance of CCProcessingListener should be deleted after processing
		 */
		bool operator()(FrameContext & context) {

			if (!ColorCube::hasColorCube(node.get())){
				// store camera currently used by rendering context
				context.pushCamera();

				ColorCubeGenerator ccGen;
				ccGen.generateColorCubes(context, node.get(), nodeCount, triangleCount );

				// restore the default camera to avoid flickering of the GUI
				context.popCamera();
			}
			return true;
		}

};

/* ########################################  methods of ColorCube ######################################### */

/** determining the hash-value for the color cubes */ 
static const Util::StringIdentifier attrName_rtColorCube(			NodeAttributeModifier::create("ColorCube", NodeAttributeModifier::PRIVATE_ATTRIBUTE | NodeAttributeModifier::COPY_TO_CLONES) );
static const Util::StringIdentifier attrName_serializedColorCube( 	NodeAttributeModifier::create("ColorCube", NodeAttributeModifier::DEFAULT_ATTRIBUTE));

typedef Util::WrapperAttribute<ColorCube> ColorCubeAttribute;

const ColorCube & ColorCube::getColorCube(Node * node) {
	assert(node != nullptr);

	ColorCubeAttribute * cca = dynamic_cast<ColorCubeAttribute *>(node->getAttribute(attrName_rtColorCube));
	if(cca != nullptr) {
		return cca->ref();
	}

	Util::GenericAttribute * savedColorCube = node->getAttribute(attrName_serializedColorCube);
	if(savedColorCube != nullptr) {
		const std::vector<uint8_t> numbers = Util::decodeBase64(savedColorCube->toString());
		if(numbers.size() == 24) {
			ColorCube cc;
			for(uint8_t side = 0; side < 6; ++side) {
				cc.colors[side] = Util::Color4ub( numbers[side*4u+0u], numbers[side*4u+1u], numbers[side*4u+2u], numbers[side*4u+3u] );
			}
			ColorCube::attachColorCube(node, cc);
			return getColorCube(node);
		} else {
			WARN("processColorCube: invalid string");
		}
	}

	throw std::logic_error("Node has no attribute storing a ColorCube.");
}

bool ColorCube::hasColorCube(Node * node){
	assert(node != nullptr);
	return (node->isAttributeSet(attrName_serializedColorCube) || node->isAttributeSet(attrName_rtColorCube));
}

//! (static)
void ColorCube::attachColorCube(Node * node, const ColorCube & cc){
	assert(node!=nullptr);
	node->setAttribute(attrName_rtColorCube, new ColorCubeAttribute(cc));

	// 1a : calculate and store the colors
	std::vector<uint8_t> numbers(24);
	size_t i=0;

	for (uint8_t side = 0; side < 6; ++side) {
		const Util::Color4ub & c = cc.colors[side];
		numbers[i++] = c.getR();
		numbers[i++] = c.getG();
		numbers[i++] = c.getB();
		numbers[i++] = c.getA();
	}
	const std::string base64 = Util::encodeBase64(numbers);
	node->setAttribute(attrName_serializedColorCube, Util::GenericAttribute::createString(base64));
}


void ColorCube::removeColorCube(Node * node, bool recursive) {
	assert(node!=nullptr);
	// remove color cubes from nodes on the path between current node and the root
	Node * n = node;
	while (n->hasParent()) {
		n = n->getParent();
		n->unsetAttribute(attrName_rtColorCube); // unsetAttribute(...) also deletes the ColorCubeAttribute
	}

	// remove nodes in current subtree
	if (recursive)
		removeColorCubesRecursive(node);
	else{
		node->unsetAttribute(attrName_rtColorCube);
		node->unsetAttribute(attrName_serializedColorCube);
	}
}

//! private: called from ColorCube::removeColorCube(...)
void ColorCube::removeColorCubesRecursive(Node * node) {
	assert(node!=nullptr);
	node->unsetAttribute(attrName_rtColorCube);
	node->unsetAttribute(attrName_serializedColorCube);
	const auto children = getChildNodes(node);
	std::for_each(children.begin(), children.end(), removeColorCubesRecursive);
}

void ColorCube::buildColorCubes(FrameContext & context, Node * node, uint32_t nodeCount, uint32_t triangleCount) {
	assert(node != nullptr);
	if (!hasColorCube(node)) {
		context.addBeginFrameListener(CCProcessingListener(node, nodeCount, triangleCount));
	}
}

// rendering
void ColorCube::drawColoredBox(FrameContext & context, const Box& box) const {
	Rendering::Mesh * mesh=getCubeMesh();
	Rendering::MeshVertexData & vd = mesh->openVertexData();
	const Rendering::VertexDescription & desc=vd.getVertexDescription();

	uint8_t * dataCursor = vd.data();
	const uint64_t vertexOffset = desc.getAttribute( Rendering::VertexAttributeIds::POSITION ).getOffset();
	const uint64_t colorOffset = desc.getAttribute( Rendering::VertexAttributeIds::COLOR ).getOffset();
	for (uint_fast8_t s = 0; s < 6; ++s) {
		const Geometry::side_t side = static_cast<Geometry::side_t> (s);
		const Geometry::corner_t * corners = Geometry::Helper::getCornerIndices(side);
		for (uint_fast8_t v = 0; v < 4; ++v) {
			// Position
			const Geometry::Vec3 & corner = box.getCorner(corners[v]);
			float * vertex=reinterpret_cast<float*>(dataCursor+vertexOffset);
			*vertex++ = corner.getX();
			*vertex++ = corner.getY();
			*vertex = corner.getZ();

			// Color
			*reinterpret_cast<uint32_t*>(dataCursor+colorOffset) = *reinterpret_cast<const uint32_t*>(colors[s].data());

			dataCursor+=desc.getVertexSize();
		}
	}
	vd.updateBoundingBox();
	vd.markAsChanged();

	context.displayMesh(mesh);
}

//! (static)
Rendering::Mesh * ColorCube::getCubeMesh(){
	static Util::Reference<Rendering::Mesh> mesh;
	using namespace Rendering;

	if (mesh.isNull()) {
		Rendering::VertexDescription vertexDescription;
		vertexDescription.appendPosition3D();
		vertexDescription.appendNormalByte();
		vertexDescription.appendColorRGBAByte();
		Rendering::MeshUtils::MeshBuilder meshBuilder(vertexDescription);

		// add vertices
		meshBuilder.color(Util::ColorLibrary::RED);
		const Geometry::Box unitBox(Geometry::Vec3(-0.5f, -0.5f, -0.5f), Geometry::Vec3(0.5f, 0.5f, 0.5f));
		for (uint_fast8_t s = 0; s < 6; ++s) {
			const Geometry::side_t side = static_cast<Geometry::side_t> (s);
			const Geometry::corner_t * corners = Geometry::Helper::getCornerIndices(side);

			meshBuilder.normal( Geometry::Helper::getNormal(side) );
			for (uint_fast8_t v = 0; v < 4; ++v) {

				meshBuilder.position(unitBox.getCorner(corners[v]));
				meshBuilder.addVertex();
			}
		}
		// add indices
		for (uint32_t i = 0; i < 6; ++i) {
			meshBuilder.addQuad( i*4, i*4+1, i*4+2, i*4+3 );
		}

		mesh=meshBuilder.buildMesh();
		mesh->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy() );
	}
	return mesh.get();
}

}

#endif // MINSG_EXT_COLORCUBES
