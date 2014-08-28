/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "HOMRenderer.h"
#include "OcclusionCullingStatistics.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include "../../Core/NodeVisitor.h"
#include "../../Core/RenderParam.h"
#include "../../Core/Statistics.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Tools.h>
#include <Geometry/Vec3.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/FBO.h>
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <map>
#include <stdexcept>
#include <utility>

#ifdef MINSG_PROFILING
#include <Util/Timer.h>
#endif

namespace MinSG {

HOMRenderer::HOMRenderer(unsigned int pyramidSize) :
	State(), rootNode(nullptr), sideLength(pyramidSize), numLevels(0),
			homPyramid(), maxOccluderDepth(30.0f),
			showOnlyOccluders(false), showHOMPyramid(false),
			showCulledGeometry(false), minOccluderSize(0.5f),
			maxOccluderComplexity(1000), triangleLimit(10000l), fbo(),
			occluderShader(),
			pyramidTests(0), pyramidTestsVisible(0), pyramidTestsInvisible(0), culledGeometry(0) {
}

HOMRenderer::HOMRenderer(const HOMRenderer & source) :
	State(), rootNode(source.rootNode), sideLength(source.sideLength),
			numLevels(0), homPyramid(), maxOccluderDepth(
					source.maxOccluderDepth), showOnlyOccluders(
					source.showOnlyOccluders), showHOMPyramid(
					source.showHOMPyramid), showCulledGeometry(
					source.showCulledGeometry), minOccluderSize(
					source.minOccluderSize), maxOccluderComplexity(
					source.maxOccluderComplexity), triangleLimit(
					source.triangleLimit), fbo(), occluderDatabase(
					source.occluderDatabase), occluderShader(),
					pyramidTests(0), pyramidTestsVisible(0), pyramidTestsInvisible(0), culledGeometry(0) {
}

HOMRenderer::~HOMRenderer() = default;

void HOMRenderer::setupShader() {
	occluderShader = Rendering::Shader::createShader(
			"void main(void){gl_Position=ftransform();}",
			"void main(void){gl_FragColor=vec4(1.0,1.0,1.0,1.0);}");
}

bool HOMRenderer::setSideLength(unsigned int pyramidSize) {
	if (pyramidSize < 4 || (pyramidSize & (pyramidSize - 1)) != 0) {
		return false;
	}
	if (sideLength != pyramidSize) {
		sideLength = pyramidSize;
		cleanupHOMPyramid();
		return true;
	}
	return false;
}

void HOMRenderer::setupHOMPyramid(Rendering::RenderingContext & context) {
	if (!homPyramid.empty()) {
		throw std::logic_error("There is already a pyramid.");
	}

	// Value of sideLength has to be a power of two.
	if (sideLength < 4 || (sideLength & (sideLength - 1)) != 0) {
		throw std::invalid_argument("Side length for highest resolution HOM has to be a power of two and has to be greater or equal four.");
	}
	// Calculate the binary logarithm of the side length.
	unsigned int steps = 0;
	unsigned int value = 1;
	while (value != sideLength) {
		value <<= 1;
		++steps;
	}
	// Lowest level should have size 4 x 4, so the index for it is (steps - 2) (4 = 2^2).
	numLevels = steps - 1;

	homPyramid.reserve(numLevels);

	unsigned int length = sideLength;
	for (unsigned int level = 0; level < numLevels; ++level) {
		homPyramid.push_back(Rendering::TextureUtils::createRedTexture(length, length, true));
		length /= 2;
	}

	fbo = new Rendering::FBO();

	context.pushAndSetFBO(fbo.get());
	fbo->attachColorTexture(context, homPyramid[0].get());

	if(!fbo->isComplete(context)){
		WARN(fbo->getStatusMessage(context));
		context.popFBO();
		return;
	}
	context.popFBO();
}

void HOMRenderer::cleanupHOMPyramid() {
	fbo = nullptr;
	homPyramid.clear();
}

struct HOMRenderer::SelectedOccluder {
	SelectedOccluder(GeometryNode * selectedOccluder, float minimumDepth, float maximumDepth) :
			occluder(selectedOccluder), minDepth(minimumDepth), maxDepth(maximumDepth) {
	}
	SelectedOccluder(const SelectedOccluder & other) :
			occluder(other.occluder), minDepth(other.minDepth), maxDepth(other.maxDepth) {
	}
	SelectedOccluder & operator=(const SelectedOccluder & other) {
		// Handle self-assignment gracefully.
		occluder = other.occluder;
		minDepth = other.minDepth;
		maxDepth = other.maxDepth;
		return *this;
	}
	GeometryNode * occluder;
	float minDepth;
	float maxDepth;
};

struct HOMRenderer::DepthSorter {
	bool operator()(const HOMRenderer::SelectedOccluder & a, const HOMRenderer::SelectedOccluder & b) const {
		return a.minDepth < b.minDepth || (!(a.minDepth > b.minDepth) &&		// compare minimum depth first
				(a.maxDepth < b.maxDepth || (!(a.maxDepth > b.maxDepth) &&		// compare maximum depth second
				&a < &b)));														// compare pointers third
	}
};

void HOMRenderer::selectOccluders(std::deque<SelectedOccluder> & occluders, AbstractCameraNode * camera) const {
	const Geometry::Vec3f cameraDir = (camera->getWorldTransformationMatrix() * Geometry::Vec4f(0.0f, 0.0f, -1.0f, 0.0f)).xyz().normalize();
	const Geometry::Vec3f cameraPos = camera->getWorldOrigin();

	for (const auto & occluder : occluderDatabase) {
		// Check if occluder is in the viewing frustum.
		// Do not add occluder if the camera is inside it.
		const Geometry::Box & bb = occluder->getWorldBB();
		if (camera->testBoxFrustumIntersection(bb) != Geometry::Frustum::OUTSIDE && !bb.contains(camera->getWorldOrigin())) {
			float minDistance = std::numeric_limits<float>::max();
			float maxDistance = 0.0f;
			for (uint_fast8_t i = 0; i < 8; ++i) {
				const Geometry::Vec3 corner = bb.getCorner(static_cast<Geometry::corner_t>(i));
				// Project the vector from the camera position to the corner onto the camera direction.
				const float distance = (corner - cameraPos).dot(cameraDir);
				if(distance < minDistance) {
					minDistance = distance;
				}
				if(distance > maxDistance) {
					maxDistance = distance;
				}
			}

			occluders.emplace_back(occluder, minDistance, maxDistance);
		}
	}
}

double HOMRenderer::drawOccluders(const std::deque<SelectedOccluder> & occluders, FrameContext & context) const {
	double zPlane = 0.0;

	unsigned long triangles = 0l;
	// List is sorted by distance of objects to camera.
	for (auto & occluder : occluders) {
		float zValue = occluder.maxDepth;

		// Stop if maximum allowd z value is reached.
		if (zValue > maxOccluderDepth) {
			break;
		}

		if (zValue > zPlane) {
			zPlane = zValue;
		}

		context.displayNode(occluder.occluder, 0);

		triangles += occluder.occluder->getTriangleCount();
		if (triangles >= triangleLimit) {
			break;
		}
	}

	return zPlane;
}

void HOMRenderer::initOccluderDatabase() {
	clearOccluderDatabase();
	if (rootNode == nullptr) {
		return;
	}
	// minOccluderSize is the minimum radius of the bounding sphere.
	// Because of performance reasons, compare it to the squared diameter.
	const float minOccluderDiameterSquared = 4.0f * minOccluderSize * minOccluderSize;
	// Traverse the whole scene.
	const auto nodes = collectNodes<GeometryNode>(rootNode);
	for(const auto & geoNode : nodes) {
		// Check if node should be added to the occluder database.
		const Geometry::Box & geoBB = geoNode->getWorldBB();
		// Check if occluder is large enough.
		if (geoBB.getDiameterSquared() < minOccluderDiameterSquared) {
			continue;
		}
		// Check if the complexity of the occluder is not too high.
		if (geoNode->getTriangleCount() > maxOccluderComplexity) {
			continue;
		}
		Node::addReference(geoNode);
		occluderDatabase.push_back(geoNode);
	}
}

void HOMRenderer::clearOccluderDatabase() {
	while (!occluderDatabase.empty()) {
		GeometryNode * geo = occluderDatabase.front();
		occluderDatabase.pop_front();
		Node::removeReference(geo);
	}
}


State::stateResult_t HOMRenderer::doEnableState(FrameContext & context,
												Node * node, const RenderParam & rp) {
	if (rp.getFlag(SKIP_RENDERER)) {
		return State::STATE_SKIPPED;
	}

	GroupNode * group = dynamic_cast<GroupNode *> (node);
	if (group == nullptr) {
		return State::STATE_SKIPPED;
	}

	if (group != rootNode) {
		rootNode = group;
		initOccluderDatabase();
	}

	Rendering::RenderingContext & renderingContext = context.getRenderingContext();

	if (homPyramid.empty()) {
		setupHOMPyramid(renderingContext);
	}

	if (showOnlyOccluders) {
		for (const auto & occluder : occluderDatabase) {
			context.displayNode(occluder, rp);
		}
		return State::STATE_SKIP_RENDERING;
	}

	pyramidTests = 0;
	pyramidTestsVisible = 0;
	pyramidTestsInvisible = 0;
	culledGeometry = 0;
	if (occluderShader.isNull()) {
		setupShader();
	}

#ifdef MINSG_PROFILING
	static unsigned long frame = 0;
	++frame;
	Util::Timer timer;
	timer.reset();
#endif

	Util::Reference<CameraNode> oldCamera = dynamic_cast<CameraNode *>(context.getCamera());
	if(oldCamera.isNull()) {
		return State::STATE_SKIPPED;
	}

	// 1. Construction of the Occlusion Map Hierarchy
	// - View-frustum culling
	int t = oldCamera->testBoxFrustumIntersection(rootNode->getWorldBB());
	if (t == Geometry::Frustum::OUTSIDE) {
		return State::STATE_SKIP_RENDERING;
	}

#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << frame << ":\tFrustum culling:\t" << timer.getMilliseconds() << " ms" << std::endl;

	timer.reset();
#endif


	// - Occluder selection
	std::deque<SelectedOccluder> occluders;
	selectOccluders(occluders, oldCamera.get());
#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << frame << ":\tOccluder selection:\t" << timer.getMilliseconds() << " ms" << std::endl;

	timer.reset();
#endif

	std::sort(occluders.begin(), occluders.end(), DepthSorter());
#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << frame << ":\tOccluder sorting:\t" << timer.getMilliseconds() << " ms" << std::endl;

	timer.reset();
#endif
	// In case that this renderer is not used at the scene root, but at a node that is transformed, rendering matrices have to be pushed and popped here.

	// - Occluder rendering and depth estimation
	// Use FBO for occluder rendering.
	renderingContext.pushAndSetFBO(fbo.get());
	// Set perspective and viewport.
	const Geometry::Vec3f cameraDir = (oldCamera->getWorldTransformationMatrix() * Geometry::Vec4f(0.0f, 0.0f, -1.0f, 0.0f)).xyz().normalize();
	const Geometry::Vec3f cameraPos = oldCamera->getWorldOrigin();

	Util::Reference<CameraNode> camera = new CameraNode();
	camera->setRelTransformation(oldCamera->getWorldTransformationMatrix());
	camera->setViewport(Geometry::Rect_i(0, 0, sideLength, sideLength));
	float fovLeft;
	float fovRight;
	float fovBottom;
	float fovTop;
	oldCamera->getAngles(fovLeft, fovRight, fovBottom, fovTop);
	camera->setAngles(fovLeft, fovRight, fovBottom, fovTop);
	if(!occluders.empty()) {
		camera->setNearFar(std::max(1e-2f, occluders.begin()->minDepth), std::min(1e+4f, occluders.rbegin()->maxDepth));
	} else {
		camera->setNearFar(0.01f, maxOccluderDepth);
	}
	context.pushAndSetCamera(camera.get());

	const Geometry::Matrix4x4f cameraMatrix = renderingContext.getMatrix_worldToCamera();
	const Geometry::Matrix4x4f projectionMatrix = renderingContext.getMatrix_cameraToClip();

	renderingContext.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LESS));
	renderingContext.applyChanges();

	renderingContext.clearColor(Util::Color4f(0.0f, 0.0f, 0.0f, 1.0f));
	renderingContext.pushAndSetShader(occluderShader.get());

	const float zPlane = drawOccluders(occluders, context);

	renderingContext.popShader();

	renderingContext.popDepthBuffer();

	// - Building the Hierarchical Occlusion Maps
	// Read the highest resolution HOM from the frame buffer.
	renderingContext.finish();
#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << frame << ":\tOccluder rendering:\t" << timer.getMilliseconds() << " ms" << std::endl;

	timer.reset();
#endif
	homPyramid[0]->downloadGLTexture(renderingContext);

	// Build the upper levels by calculating the average over 2 x 2 pixels.
	// Possible speed up by texture mapping.
#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << frame << ":\tPyramid reading:\t" << timer.getMilliseconds() << " ms" << std::endl;

	timer.reset();
#endif
	unsigned int srcLength = sideLength;
	unsigned int dstLength = srcLength / 2;
	for (unsigned int level = 1; level < numLevels; ++level) {
		const uint8_t * src = homPyramid[level - 1]->openLocalData(renderingContext);
		uint8_t * dst = homPyramid[level]->openLocalData(renderingContext);
		for (unsigned int y = 0; y < dstLength; ++y) {
			for (unsigned int x = 0; x < dstLength; ++x) {
				unsigned int srcRow1 = (y * 2) * srcLength + (x * 2);
				unsigned int srcRow2 = srcRow1 + srcLength;
				*dst = (static_cast<unsigned short> (src[srcRow1])
						+ static_cast<unsigned short> (src[srcRow1 + 1])
						+ static_cast<unsigned short> (src[srcRow2])
						+ static_cast<unsigned short> (src[srcRow2 + 1])) / 4;
				++dst;
			}
		}
		homPyramid[level]->dataChanged();
		srcLength /= 2;
		dstLength /= 2;
	}
#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << frame << ":\tPyramid downscale:\t" << timer.getMilliseconds() << " ms" << std::endl;

	timer.reset();
#endif


	// Deactivate FBO and restore perspective and viewport.
	context.popCamera();
	renderingContext.popFBO();


	// Draw the scene with occlusion culling.
	std::deque<Node *> nodes;
	nodes.push_back(rootNode);
	while (!nodes.empty()) {
		Node * current = nodes.front();
		nodes.pop_front();
		if (process(current, cameraPos, cameraDir, zPlane, context, rp, cameraMatrix, projectionMatrix) != NodeVisitor::BREAK_TRAVERSAL) {
			if (!current->isClosed()) {
				// Add children to list.
				const auto children = getChildNodes(node);
				nodes.insert(nodes.end(), children.begin(), children.end());
			}
		}
	}
#ifdef MINSG_PROFILING
	renderingContext.finish();
	timer.stop();
	std::cout << frame << ":\tScene rendering:\t" << timer.getMilliseconds() << " ms" << std::endl;
#endif

	if(showHOMPyramid) {
		// Debugging code for rendering the HOM pyramid.
		unsigned int xPos = 0;
		unsigned int length = sideLength;
		for(unsigned int level = 0; level < numLevels; ++level) {
			Rendering::TextureUtils::drawTextureToScreen(
				renderingContext,
				Geometry::Rect_i(xPos, 0, length, length),
				*homPyramid[level].get(),
				Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f)
			);
			xPos += length;
			length /= 2;
		}
	}

	Statistics & statistics = context.getStatistics();
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestCounter(), pyramidTests);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestVisibleCounter(), pyramidTestsVisible);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getOccTestInvisibleCounter(), pyramidTestsInvisible);
	statistics.addValue(OcclusionCullingStatistics::instance(statistics).getCulledGeometryNodeCounter(), culledGeometry);

	return State::STATE_SKIP_RENDERING;
}

int HOMRenderer::process(Node * node,
							const Geometry::Vec3f & cameraPos,
							const Geometry::Vec3f & cameraDir,
							float zPlane,
							FrameContext & context,
							const RenderParam & rp,
							const Geometry::Matrix4x4f & cameraMatrix,
							const Geometry::Matrix4x4f & projectionMatrix) {
	const AbstractCameraNode * camera = context.getCamera();
	const Geometry::Box & worldBB = node->getWorldBB();
	// 2. Visibility Culling with Hierarchical Occlusion Maps

	// - View-frustum Culling
	if (camera->testBoxFrustumIntersection(worldBB) == Geometry::Frustum::OUTSIDE) {
		return NodeVisitor::BREAK_TRAVERSAL;
	}

	// - Depth Comparison
	float minDistance = std::numeric_limits<float>::max();
	for (uint_fast8_t i = 0; i < 8; ++i) {
		const Geometry::Vec3 corner = worldBB.getCorner(static_cast<Geometry::corner_t>(i));
		// Project the vector from the camera position to the corner onto the camera direction.
		const float distance = (corner - cameraPos).dot(cameraDir);
		if(distance < minDistance) {
			minDistance = distance;
		}
	}
	if (minDistance <= zPlane) {
		if (node->isClosed()) {
			context.displayNode(node, rp);
		}
		return NodeVisitor::CONTINUE_TRAVERSAL;
	}

	++pyramidTests;


	// - Overlap test with Occlusion Maps
	const Geometry::Rect pyramidRect(0, 0, sideLength, sideLength);
	const Geometry::Rect_f projectedRect = Geometry::projectBox(worldBB, cameraMatrix, projectionMatrix, pyramidRect);

	// Check if rect is outside of viewport.
	if(!projectedRect.intersects(pyramidRect)) {
		return NodeVisitor::BREAK_TRAVERSAL;
	}

	const Geometry::Rect_i rect(projectedRect);

	// Clip rect to screen space.
	int minX = rect.getMinX();
	if (minX < 0) {
		minX = 0;
	}
	int maxX = rect.getMaxX();
	if (maxX >= static_cast<int> (sideLength)) {
		maxX = sideLength - 1;
	}

	int minY = rect.getMinY();
	if (minY < 0) {
		minY = 0;
	}
	int maxY = rect.getMaxY();
	if (maxY >= static_cast<int> (sideLength)) {
		maxY = sideLength - 1;
	}

	unsigned int sizeX = maxX - minX;
	unsigned int sizeY = maxY - minY;
	unsigned int size = sizeX > sizeY ? sizeX : sizeY;

	if (sizeX > 0 && sizeY > 0) {
		// Calculate the start level in the HOM pyramid from the size of the screen rectangle.
		unsigned int level = static_cast<unsigned int>(logf(size) / 0.301029995663981198); // = log(2)
		if (level >= numLevels) {
			level = numLevels - 1;
		}

		unsigned int levelMinX = minX >> level;
		unsigned int levelMaxX = maxX >> level;
		unsigned int levelMinY = minY >> level;
		unsigned int levelMaxY = maxY >> level;

		bool visible = isAreaVisible(level, levelMinX, levelMaxX, levelMinY,
				levelMaxY, minX, maxX, minY, maxY);
		if (visible) {
			++pyramidTestsVisible;
			if (node->isClosed()) {
				context.displayNode(node, rp);
			}
			return NodeVisitor::CONTINUE_TRAVERSAL;
		}

		++pyramidTestsInvisible;

		if (HOMRenderer::showCulledGeometry) {
			if (node->isClosed()) {
				context.displayNode(node, rp - NO_GEOMETRY);
			}
			return NodeVisitor::CONTINUE_TRAVERSAL;
		}
	}
	// (Only objects that fail one of the latter two tests (depth or overlap) are rendered.)
	++culledGeometry;
	return NodeVisitor::BREAK_TRAVERSAL;
}

bool HOMRenderer::isAreaVisible(unsigned int level, unsigned int minX,
								unsigned int maxX, unsigned int minY,
								unsigned int maxY, unsigned int bMinX,
								unsigned int bMaxX, unsigned int bMinY,
								unsigned int bMaxY) const {
	const unsigned char treshold = 255;


	// Intersect the rect of the area to test with the rect of the projected bounding box.
	unsigned int levelMinX = bMinX >> level;
	unsigned int levelMaxX = bMaxX >> level;
	unsigned int levelMinY = bMinY >> level;
	unsigned int levelMaxY = bMaxY >> level;

	unsigned int length = sideLength >> level;
	if (minX < levelMinX) {
		minX = levelMinX;
	} else if (minX > levelMaxX) {
		return false;
	}
	if (maxX > levelMaxX) {
		maxX = levelMaxX;
	} else if (maxX < levelMinX) {
		return false;
	}
	if (minY < levelMinY) {
		minY = levelMinY;
	} else if (minY > levelMaxY) {
		return false;
	}
	if (maxY > levelMaxY) {
		maxY = levelMaxY;
	} else if (maxY < levelMinY) {
		return false;
	}

	// Test the area.
	const uint8_t * data = homPyramid[level]->getLocalData();
	for (unsigned int x = minX; x <= maxX; ++x) {
		for (unsigned int y = minY; y <= maxY; ++y) {
			if (data[y * length + x] < treshold) {
				if (level > 0) {
					// Check sub pixels.
					unsigned int subX = x << 1;
					unsigned int subY = y << 1;
					if (isAreaVisible(level - 1, subX, subX + 1, subY,
							subY + 1, bMinX, bMaxX, bMinY, bMaxY)) {
						return true;
					}
					// If the object is hidden behind the sub pixels, continue with the other pixels on this level.
				} else {
					// Most detailed level reached. Object is visible.
					return true;
				}
			}
		}
	}
	// No visible pixel was found.
	return false;
}

HOMRenderer * HOMRenderer::clone() const {
	return new HOMRenderer(*this);
}

}
