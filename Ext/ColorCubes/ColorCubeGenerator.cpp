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
#include "ColorCubeGenerator.h"
#include "ColorCube.h"

#include <Geometry/Rect.h>
#include <Geometry/Tools.h>

#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/FBO.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Serialization/Serialization.h>

#include <Util/IO/FileName.h>
#include <Util/ProgressIndicator.h>

#include <Util/Graphics/ColorLibrary.h>
#include <Util/Encoding.h>
#include <Util/Macros.h>

#include "../../Helper/StdNodeVisitors.h"
#include "../../Helper/Helper.h"
#include "../../Core/Nodes/CameraNodeOrtho.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/States/State.h"
#include "../../Core/States/AlphaTestState.h"
#include "../../Core/States/PolygonModeState.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/FrameContext.h"
#include "../States/EnvironmentState.h"

#include <sstream>
#include <cassert>
#include <cmath>
#include <list>
#include <queue>

using namespace std;

using Geometry::Box;
using Geometry::Rect;
using Rendering::FBO;
using Rendering::Texture;
using Util::Color4f;
using Util::Color4ub;

namespace MinSG {

/* ################################ class responsible for processing color cubes ##################### */
/**
 * comparator used to sort nodes according to the distance of the specified side (side of the bounding box) from the camera
 */
struct NodeCompareBySide {
	/** the specified side of the bounding box */
	Geometry::side_t side;
	/** [ctor] creates a NodeCompareBySide-instance with specified side */
	NodeCompareBySide(Geometry::side_t _side) :
		side(_side) {
	}
	/**
	 * compare operator is used by STL sorting functions to compare two nodes.
	 * @param node1 the first node
	 * @param node2 the second node
	 * @return true if the specified side of node1 is nearer to the camera than that of node2, otherwise false
	 */
	bool operator()(Node * node1, Node * node2) const {
		switch (side) {
			case Geometry::side_t::X_POS:
				return node1->getWorldBB().getMaxX() > node2->getWorldBB().getMaxX();
			case Geometry::side_t::X_NEG:
				return node1->getWorldBB().getMinX() < node2->getWorldBB().getMinX();
			case Geometry::side_t::Y_POS:
				return node1->getWorldBB().getMaxY() > node2->getWorldBB().getMaxY();
			case Geometry::side_t::Y_NEG:
				return node1->getWorldBB().getMinY() < node2->getWorldBB().getMinY();
			case Geometry::side_t::Z_POS:
				return node1->getWorldBB().getMaxZ() > node2->getWorldBB().getMaxZ();
			case Geometry::side_t::Z_NEG:
				return node1->getWorldBB().getMinZ() < node2->getWorldBB().getMinZ();
			default:
				FAIL();
		}
	}
};

typedef std::priority_queue< Node*, std::vector<Node*>, NodeCompareBySide > distanceQueue_t;

/**
 * calculate the color for a side of the color cube
 * @param texture color texture
 * @param rect 	rectangle of the color texture to average
 */
static Util::Color4ub calculateColor(const Rendering::Texture * texture, const Geometry::Rect_i & rect) {
	// calculate the color from the color texture
	const uint32_t pixelCount = rect.getWidth() * rect.getHeight(); // calculate the number of pixels in the texture
	if(pixelCount==0)
		return Util::Color4ub(0,0,0,0);
	const uint32_t rowSize = texture->getFormat().getRowSize();

	const uint8_t * yPtr = texture->getLocalData() + rect.getY() * rowSize;
	uint32_t r_sum = 0, g_sum = 0, b_sum = 0, a_sum = 0;

	uint32_t isCount = 0;
	for (int32_t y = 0; y < rect.getHeight(); ++y ) {
		const uint8_t * xPtr = yPtr + rect.getX()*4;
		for (int32_t x = 0; x < rect.getWidth(); ++x ) {
// 			r_sum += *(xPtr++);
// 			g_sum += *(xPtr++);
// 			b_sum += *(xPtr++);
// 			a_sum += *(xPtr++);
			uint32_t r = *(xPtr++);
			uint32_t g = *(xPtr++);
			uint32_t b = *(xPtr++);
			uint32_t a = *(xPtr++);
			if(a>0){
				r_sum += r;
				g_sum += g;
				b_sum += b;
				a_sum += a;
				isCount++;
			}
		}
		yPtr += rowSize;
	}

	if(isCount==0)
		return Util::Color4ub(0,0,0,0);
	
	return Util::Color4ub(	r_sum / isCount,
							g_sum / isCount,
							b_sum / isCount,
							a_sum / pixelCount);
}

/**
 * checks whether current node/subtree contains at most nodeCount nodes and at most triangleCount triangles.
 * For nodes, that fulfill these conditions, the calculation of their color cubes is done directly by drawing
 * the Geometry. If an inner node fulfill the conditions, only its color cube will be processed, not those of
 * its children (in this case we do not really require the color cubes of the children to exist).
 *
 * @param node          : node that the conditions have to checked for
 * @param nodeCount     : the maximum number of nodes
 * @param triangleCount : the maximum number of triangles
 * @return @c true if the specified node fulfills the conditions, otherwise @c false.
 */
static bool fulfillsProcessingConditions(Node * _node, uint32_t _nodeCount, uint32_t _triangleCount) {
	struct Vis : public NodeVisitor {
		const uint32_t maxTriangleCount;
		const uint32_t maxNodeCount;
		uint32_t currentTriangleCount;
		uint32_t currentNodeCount; // counts only geometry nodes (we could also count all nodes, or only closed nodes)

		Vis(uint32_t maxNodes, uint32_t maxTriangles) :
			maxTriangleCount(maxTriangles), maxNodeCount(maxNodes), currentTriangleCount(0), currentNodeCount(0) {
		}
		virtual ~Vis() {}

		// ---|> NodeVisitor
		NodeVisitor::status enter(Node * node) override {
			GeometryNode * geometry = dynamic_cast<GeometryNode*>(node);
			if (geometry != nullptr) {
				currentTriangleCount += geometry->getTriangleCount();
				++currentNodeCount;
			}

			if (currentTriangleCount > maxTriangleCount || currentNodeCount > maxNodeCount) {
				return BREAK_TRAVERSAL;
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(_nodeCount, _triangleCount);
	_node->traverse(visitor);
	return (visitor.currentNodeCount <= _nodeCount && visitor.currentTriangleCount <= _triangleCount);
}

/* ###################################  methods  in ColorCubeGenerator  ################################# */

//! [ctor]
ColorCubeGenerator::ColorCubeGenerator() 			{	}

//! [dtor]
ColorCubeGenerator::~ColorCubeGenerator(){}

void ColorCubeGenerator::generateColorCubes(FrameContext& context, Node * node, uint32_t nodeCount, uint32_t triangleCount){
	if(fbo.isNull()){
		// init the data needed for processing the colors and process the color cubes
		depthTexture = Rendering::TextureUtils::createDepthTexture(maxTexSize * 6, maxTexSize);
		colorTexture = Rendering::TextureUtils::createStdTexture(maxTexSize * 6, maxTexSize, true);
		fbo = new Rendering::FBO();
		context.getRenderingContext().pushAndSetFBO(fbo.get());
		fbo->attachColorTexture(context.getRenderingContext(),colorTexture.get());
		fbo->attachDepthTexture(context.getRenderingContext(),depthTexture.get());
		context.getRenderingContext().popFBO();
		camera = new CameraNodeOrtho();
	}

	processColorCubes(context, node, nodeCount, triangleCount);
}


//! (internal)
void ColorCubeGenerator::processColorCubes(FrameContext& context, Node * _node, uint32_t nodeCount, uint32_t triangleCount) {
	//cerr << "begin processing color cubes !" << endl;
	//  0: create the ambient light
	static LightingState * preprolight = nullptr;
	if (!preprolight) {
		auto ln = new LightNode(Rendering::LightParameters::DIRECTIONAL);
		ln->setAmbientLightColor( Util::Color4f(1, 1, 1, 1));
		ln->setDiffuseLightColor( Util::Color4f(0, 0, 0, 0));
		ln->setSpecularLightColor(Util::Color4f(0, 0, 0, 0));
		preprolight = new LightingState(ln);
	}

	preprolight->enableState(context, _node, 0);

	struct Vis : public NodeVisitor {
		ColorCubeGenerator & ccpro;
		bool processed;
		FrameContext& rc;

		uint32_t maxNodeCount;  	// maximum number of geometry nodes in a subtree
		uint32_t maxTriangleCount;	// maximum number of triangles in a subtree

		Vis(ColorCubeGenerator & _ccpro, FrameContext& _context, uint32_t _nodeCount, uint32_t _triangleCount) :
			ccpro(_ccpro), processed(false), rc(_context), maxNodeCount(_nodeCount), maxTriangleCount(_triangleCount) {
		}
		virtual ~Vis() {}


		// ---|> NodeVisitor
		NodeVisitor::status enter(Node * node) override {
			if (node->getWorldBB().getExtentMax() <= 0.0f) { // there are boxes with zero-extension (e.g. looseOctree/octree)
				if (!ColorCube::hasColorCube(node))
					ColorCube::attachColorCube(node, ColorCube());
				return BREAK_TRAVERSAL;
			}
			if (ColorCube::hasColorCube(node)) {
				processed = true;
				return BREAK_TRAVERSAL;
			}

			if (node->isClosed())
				return BREAK_TRAVERSAL;
			return CONTINUE_TRAVERSAL;
		}


		// ---|> NodeVisitor
		NodeVisitor::status leave(Node * node) override {
			if (node->getWorldBB().getExtentMax() <= 0.0f) // there are boxes with zero-extension (e.g. looseOctree/octree)
				return CONTINUE_TRAVERSAL;
			if (processed) {
				processed = false;
				return CONTINUE_TRAVERSAL;
			}

			deque<Node*> children;
			bool drawgeometry = fulfillsProcessingConditions(node, maxNodeCount, maxTriangleCount );
			if (!node->isClosed() && !drawgeometry) {
				const auto newChildren = getChildNodes(node);
				children.insert(children.end(), newChildren.begin(), newChildren.end());
			}
			ccpro.processColorCube(rc, node, children);
			return CONTINUE_TRAVERSAL;
		}

	} visitor(*this, context, nodeCount, triangleCount);
	_node->traverse(visitor);

	preprolight->disableState(context, _node, 0);
	//cerr << "end of processing" << endl;
}

/*! @note very important: color cubes of the children must already exist, otherwise there will be a segmentation fault  */
void ColorCubeGenerator::processColorCube(FrameContext& context, Node * node, deque<Node*>& children) {

	const Geometry::Box box = node->getWorldBB();
	ColorCube cc;

	// 1. case: current node's color cube should be processed by drawing Geometry < see processColorCubes(...) >
	if (children.empty()) {
		context.getRenderingContext().pushAndSetLighting(Rendering::LightingParameters(true));
		context.getRenderingContext().applyChanges();

		// render the six sides
		context.getRenderingContext().pushAndSetFBO(fbo.get());
		for (uint8_t _side = 0; _side < 6; ++_side) { // determine the color for each of the 6 faces
			const Geometry::side_t side = static_cast<Geometry::side_t> (_side);
			if (!prepareCamera(context, box, side))
				continue;

			context.getRenderingContext().clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
			context.displayNode(node, USE_WORLD_MATRIX);
		}
		context.getRenderingContext().popFBO();
		colorTexture->downloadGLTexture(context.getRenderingContext());

		//////////// debug
// 		static uint32_t counter=0;
// 		stringstream ss;
// 		ss << "screens/colorcube_" << counter++ << ".png";
// 		Util::FileName filename(ss.str());
// 		Rendering::Serialization::saveTexture(context.getRenderingContext(), colorTexture.get(), filename);
		//////////// end debug



		// determine the color for each of the 6 faces
		for (uint8_t _side = 0; _side < 6; ++_side) {
			const Geometry::side_t side = static_cast<Geometry::side_t> (_side);
			if (prepareCamera(context, box, side)){
				const Color4ub c = calculateColor(colorTexture.get(), camera->getViewport()); //calculate color from texture
				cc.colors[static_cast<uint8_t> (side)] = c;
			} else {
				cc.colors[static_cast<uint8_t> (side)] = Color4ub(0, 0, 0, 0);
			}
		}

		context.getRenderingContext().popLighting();
	}
	else{
		context.getRenderingContext().pushAndSetFBO(fbo.get()); // enable the fbo

		// 2. case:  the node is not a closed GroupNode  (inner node)
		for (uint8_t _side = 0; _side < 6; ++_side) { // for each of the six faces
			Geometry::side_t side = static_cast<Geometry::side_t> (_side);
			if (!prepareCamera(context, box, side))
				continue;

			context.getRenderingContext().clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
			context.getRenderingContext().pushAndSetMatrix_modelToCamera( context.getRenderingContext().getMatrix_worldToCamera() );


			// draw faces using blending
			context.getRenderingContext().pushAndSetBlending(Rendering::BlendingParameters(Rendering::BlendingParameters::ONE, Rendering::BlendingParameters::ONE_MINUS_SRC_ALPHA));
			context.getRenderingContext().pushAndSetCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_BACK));
			context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, false, Rendering::Comparison::LESS));
			context.getRenderingContext().pushAndSetLighting(Rendering::LightingParameters(false));
			context.getRenderingContext().applyChanges();

			distanceQueue_t nodes(side); // distance queue used for sorting the children with color cubes

			// sort (back-to-front order) the children according to distance of current side to the camera
			for(const auto & child : children) {
				nodes.push(child);
			}

			// draw the faces in back-to-front order
			while (!nodes.empty()) {
				Node* child = nodes.top();
				nodes.pop();
				const ColorCube & childsColorCube = ColorCube::getColorCube(child);
				//cerr << "before drawing colored box " << endl;
				childsColorCube.drawColoredBox(context, child->getWorldBB());
				//cerr << "after drawing colored box" << endl;
			}

			context.getRenderingContext().popLighting();
			context.getRenderingContext().popDepthBuffer();
			context.getRenderingContext().popCullFace();
			context.getRenderingContext().popBlending();
			context.getRenderingContext().popMatrix_modelToCamera();
		}
		context.getRenderingContext().popFBO();
		colorTexture->downloadGLTexture(context.getRenderingContext());
		// determine the color for each of the 6 faces
		for (uint8_t _side = 0; _side < 6; ++_side) {
			Geometry::side_t side = static_cast<Geometry::side_t> (_side);
			if (!prepareCamera(context, box, side))
				continue;
			//calculate color from texture
			cc.colors[static_cast<uint8_t> (side)] = calculateColor(colorTexture.get(), camera->getViewport());
		}
	}

	ColorCube::attachColorCube(node, cc);
}

//! (internal)
bool ColorCubeGenerator::prepareCamera(FrameContext& context, const Box& _box, Geometry::side_t side) {

	Geometry::Box box(_box);
	camera->resetRelTransformation();
	camera->setRelOrigin(box.getCenter());
	switch(side){
		case Geometry::side_t::X_POS:
			camera->rotateRel(Geometry::Angle::deg(90.0f), Geometry::Vec3(0, 1, 0));
			if(box.getExtentY() == 0 || box.getExtentZ() == 0) return false; // width or height equal zero
			if(box.getExtentX() == 0) box.resizeAbs(0.1,0,0); // depth equal zero
			box.resizeRel(1.01,1,1);
			break;
		case Geometry::side_t::X_NEG:
			camera->rotateRel(Geometry::Angle::deg(-90.0f), Geometry::Vec3(0, 1, 0));
			if(box.getExtentY() == 0 || box.getExtentZ() == 0) return false; // width or height equal zero
			if(box.getExtentX() == 0) box.resizeAbs(0.1,0,0); // depth equal zero
			box.resizeRel(1.01,1,1);
			break;
		case Geometry::side_t::Y_POS:
			camera->rotateRel(Geometry::Angle::deg(-90.0f), Geometry::Vec3(1, 0, 0));
			if(box.getExtentX() == 0 || box.getExtentZ() == 0) return false; // width or height equal zero
			if(box.getExtentY() == 0) box.resizeAbs(0,0.1,0); // depth equal zero
			box.resizeRel(1,1.01,1);
			break;
		case Geometry::side_t::Y_NEG:
			camera->rotateRel(Geometry::Angle::deg(90.0f), Geometry::Vec3(1, 0, 0));
			if(box.getExtentX() == 0 || box.getExtentZ() == 0) return false; // width or height equal zero
			if(box.getExtentY() == 0) box.resizeAbs(0,0.1,0); // depth equal zero
			box.resizeRel(1,1.01,1);
			break;
		case Geometry::side_t::Z_POS:
			if(box.getExtentX() == 0 || box.getExtentY() == 0) return false; // width or height equal zero
			if(box.getExtentZ() == 0) box.resizeAbs(0,0,0.1); // depth equal zero
			box.resizeRel(1,1,1.01);
			break;
		case Geometry::side_t::Z_NEG:
			camera->rotateRel(Geometry::Angle::deg(180.0f), Geometry::Vec3(0, 1, 0));
			if(box.getExtentX() == 0 || box.getExtentY() == 0) return false; // width or height equal zero
			if(box.getExtentZ() == 0) box.resizeAbs(0,0,0.1); // depth equal zero
			box.resizeRel(1,1,1.01);
			break;
		default:
			throw std::logic_error("the roof is on fire");
	}
	
	//box.resizeRel(1.01);
	Geometry::Frustum f = Geometry::calcEnclosingOrthoFrustum(box, camera->getWorldTransformationMatrix().inverse());
	camera->setClippingPlanes(f.getLeft(), f.getRight(), f.getBottom(), f.getTop());
	camera->setNearFar(f.getNear() , f.getFar());
	camera->setViewport(Geometry::Rect_i(maxTexSize*static_cast<uint8_t>(side), 0, maxTexSize, maxTexSize), true);
	context.setCamera(camera.get());

	return true;
}

}

#endif // MINSG_EXT_COLORCUBES
