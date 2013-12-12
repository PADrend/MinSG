/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#include "PreprocessingContext.h"
#include "Helper.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../SceneManagement/SceneManager.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/FBO.h>
#include <Util/Utils.h>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#ifdef MINSG_EXT_SVS_PROFILING
#include "../Profiling/Logger.h"
#include "../Profiling/Profiler.h"
#include <fstream>
#endif /* MINSG_EXT_SVS_PROFILING */

namespace MinSG {
namespace SVS {

#ifdef MINSG_EXT_SVS_PROFILING
const Util::StringIdentifier PreprocessingContext::ATTR_numDescendantsGeometryNode("numDescendantsGeometryNode");
const Util::StringIdentifier PreprocessingContext::ATTR_numDescendantsGroupNode("numDescendantsGroupNode");
const Util::StringIdentifier PreprocessingContext::ATTR_numDescendantsTriangles("numDescendantsTriangles");
const Util::StringIdentifier PreprocessingContext::ATTR_numGeometryNodesVisible("numGeometryNodesVisible");
const Util::StringIdentifier PreprocessingContext::ATTR_numTrianglesVisible("numTrianglesVisible");
const Util::StringIdentifier PreprocessingContext::ATTR_numPixelsVisible("numPixelsVisible");
const Util::StringIdentifier PreprocessingContext::ATTR_numChildrenGeometryNode("numChildrenGeometryNode");
const Util::StringIdentifier PreprocessingContext::ATTR_numChildrenGroupNode("numChildrenGroupNode");
const Util::StringIdentifier PreprocessingContext::ATTR_numVertices("numVertices");
const Util::StringIdentifier PreprocessingContext::ATTR_sphereRadius("sphereRadius");
#endif /* MINSG_EXT_SVS_PROFILING */

struct PreprocessingContext::Implementation {
	/**
	 * Container holding the nodes that have to be preprocessed. Implicitly,
	 * the container reflects the state of the preprocessing. If it is empty,
	 * the preprocessing is finished.
	 */
	std::deque<GroupNode *> unfinishedNodes;

	SceneManagement::SceneManager & sceneManager;
	FrameContext & frameContext;
	const std::vector<Geometry::Vec3f> positions;
	const uint32_t resolution;
	const bool useExistingVisibilityResults;
	const bool computeTightInnerBoundingSpheres;

	Util::Reference<Rendering::Texture> colorTexture;
	Util::Reference<Rendering::Texture> depthTexture;
	Util::Reference<Rendering::FBO> fbo;

#ifdef MINSG_EXT_SVS_PROFILING
	std::unique_ptr<Profiling::LoggerTSV> tsvLogger;
	std::ofstream tsvLoggerStream;
	Profiling::Profiler profiler;
#endif /* MINSG_EXT_SVS_PROFILING */

	Implementation(SceneManagement::SceneManager & p_sceneManager,
				  FrameContext & p_frameContext,
				  std::vector<Geometry::Vec3f> p_positions,
				  uint32_t p_resolution,
				  bool p_useExistingVisibilityResults,
				  bool p_computeTightInnerBoundingSpheres) :
		unfinishedNodes(),
		sceneManager(p_sceneManager),
		frameContext(p_frameContext),
		positions(std::move(p_positions)),
		resolution(p_resolution),
		useExistingVisibilityResults(p_useExistingVisibilityResults),
		computeTightInnerBoundingSpheres(p_computeTightInnerBoundingSpheres),
		colorTexture(Rendering::TextureUtils::createStdTexture(resolution, resolution, true)),
		depthTexture(Rendering::TextureUtils::createDepthTexture(resolution, resolution)),
		fbo(new Rendering::FBO) {
	}
};

PreprocessingContext::PreprocessingContext(SceneManagement::SceneManager & sceneManager,
										   FrameContext & frameContext,
										   GroupNode * rootNode,
										   const std::vector<Geometry::Vec3f> & positions,
										   uint32_t resolution,
										   bool useExistingVisibilityResults,
										   bool computeTightInnerBoundingSpheres) :
	impl(new Implementation(sceneManager, 
							frameContext, 
							positions, 
							resolution, 
							useExistingVisibilityResults, 
							computeTightInnerBoundingSpheres)) {

	impl->frameContext.getRenderingContext().pushAndSetFBO(impl->fbo.get());
	impl->fbo->attachColorTexture(impl->frameContext.getRenderingContext(), impl->colorTexture.get());
	impl->fbo->attachDepthTexture(impl->frameContext.getRenderingContext(), impl->depthTexture.get());
	impl->frameContext.getRenderingContext().popFBO();

#ifdef MINSG_EXT_SVS_PROFILING
	const auto fileNamePrefix = Util::Utils::createTimeStamp() + "_SVS_Preprocessing";

	impl->tsvLoggerStream.open(fileNamePrefix + ".tsv");
	impl->tsvLogger.reset(new Profiling::LoggerTSV(impl->tsvLoggerStream));
	impl->tsvLogger->addColumn(ATTR_numDescendantsGeometryNode);
	impl->tsvLogger->addColumn(ATTR_numDescendantsGroupNode);
	impl->tsvLogger->addColumn(ATTR_numDescendantsTriangles);
	impl->tsvLogger->addColumn(ATTR_numGeometryNodesVisible);
	impl->tsvLogger->addColumn(ATTR_numTrianglesVisible);
	impl->tsvLogger->addColumn(ATTR_numPixelsVisible);
	impl->tsvLogger->addColumn(ATTR_numChildrenGeometryNode);
	impl->tsvLogger->addColumn(ATTR_numChildrenGroupNode);
	impl->tsvLogger->addColumn(ATTR_numVertices);
	impl->tsvLogger->addColumn(ATTR_sphereRadius);

	impl->profiler.registerLogger(impl->tsvLogger.get());

	auto action = impl->profiler.beginTimeMemoryAction("Initial tree traversal");
#endif /* MINSG_EXT_SVS_PROFILING */

	// Do a bottom-up tree traversal to collect all internal nodes
	forEachNodeBottomUp<GroupNode>(rootNode,
								   [this](GroupNode * groupNode) { impl->unfinishedNodes.push_back(groupNode); });

#ifdef MINSG_EXT_SVS_PROFILING
	impl->profiler.endTimeMemoryAction(action);
#endif /* MINSG_EXT_SVS_PROFILING */
}

PreprocessingContext::~PreprocessingContext() {
#ifdef MINSG_EXT_SVS_PROFILING
	impl->profiler.unregisterLogger(impl->tsvLogger.get());
	impl->tsvLogger.reset();
#endif /* MINSG_EXT_SVS_PROFILING */
}

void PreprocessingContext::preprocessSingleNode() {
#ifdef MINSG_EXT_SVS_PROFILING
	auto action = impl->profiler.beginTimeMemoryAction("Node preprocessing");
#endif /* MINSG_EXT_SVS_PROFILING */

	GroupNode * currentNode = impl->unfinishedNodes.front();
	impl->unfinishedNodes.pop_front();

	impl->frameContext.getRenderingContext().pushAndSetFBO(impl->fbo.get());
	preprocessNode(*this, currentNode);
	impl->frameContext.getRenderingContext().popFBO();

#ifdef MINSG_EXT_SVS_PROFILING
	impl->profiler.endTimeMemoryAction(action);
#endif /* MINSG_EXT_SVS_PROFILING */
}

bool PreprocessingContext::isFinished() const {
	return impl->unfinishedNodes.empty();
}

std::size_t PreprocessingContext::getNumRemainingNodes() const {
	return impl->unfinishedNodes.size();
}

SceneManagement::SceneManager & PreprocessingContext::getSceneManager() {
	return impl->sceneManager;
}

FrameContext & PreprocessingContext::getFrameContext() {
	return impl->frameContext;
}

const std::vector<Geometry::Vec3f> & PreprocessingContext::getPositions() const {
	return impl->positions;
}

uint32_t PreprocessingContext::getResolution() const {
	return impl->resolution;
}

bool PreprocessingContext::getUseExistingVisibilityResults() const {
	return impl->useExistingVisibilityResults;
}

bool PreprocessingContext::getComputeTightInnerBoundingSpheres() const {
	return impl->computeTightInnerBoundingSpheres;
}

#ifdef MINSG_EXT_SVS_PROFILING
Profiling::Profiler & PreprocessingContext::getProfiler() {
	return impl->profiler;
}
#endif /* MINSG_EXT_SVS_PROFILING */

}
}

#endif /* MINSG_EXT_SVS */
