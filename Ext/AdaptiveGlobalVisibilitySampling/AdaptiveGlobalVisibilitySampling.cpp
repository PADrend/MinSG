/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#include "AdaptiveGlobalVisibilitySampling.h"
#include "SampleDistributions.h"
#include "Sample.h"
#include "ViewCells.h"
#include "../RayCasting/RayCaster.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include <Geometry/Box.h>
#include <Geometry/Line.h>
#include <Geometry/RayBoxIntersection.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <Util/Utils.h>
#include <cstdint>
#include <deque>
#include <limits>

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
#include "../Profiling/Logger.h"
#include "../Profiling/Profiler.h"
#include <fstream>
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

namespace MinSG {
namespace AGVS {

template<typename value_t>
struct AdaptiveGlobalVisibilitySampling::Implementation {
	typedef Geometry::_Ray<Geometry::_Vec3<value_t>> ray_t;

	Util::Reference<GroupNode> scene;
	std::vector<Sample<value_t>> newSamples;
	std::vector<ray_t> rays;
	std::vector<Sample<value_t>> meshSamples;
	SampleDistributions sampleDistributions;
	//! Storage for the view space subdivison
	Util::Reference<ValuatedRegionNode> rootViewCell;

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
	std::unique_ptr<Profiling::LoggerTSV> tsvLogger;
	std::ofstream tsvLoggerStream;
	Profiling::Profiler profiler;
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

	Implementation(GroupNode * p_scene, 
				   ValuatedRegionNode * viewSpaceSubdivision) :
		scene(p_scene), newSamples(), rays(), meshSamples(),
		sampleDistributions(viewSpaceSubdivision->getWorldBB(), scene.get()),
		rootViewCell(viewSpaceSubdivision) {

		splitViewCell(rootViewCell.get());

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
		tsvLoggerStream.open(Util::Utils::createTimeStamp() +
													"_AGVS_Preprocessing.tsv");
		tsvLogger.reset(new Profiling::LoggerTSV(tsvLoggerStream));
		profiler.registerLogger(tsvLogger.get());
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */
	}

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
	~Implementation() {
		profiler.unregisterLogger(tsvLogger.get());
		tsvLogger.reset();
	}
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

	bool performSampling(uint32_t numSamples) {
		rays.reserve(2 * numSamples);
		newSamples.reserve(numSamples);

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
		auto sampleAction = profiler.beginTimeMemoryAction("Sample generation");
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

		for(uint_fast32_t sample = 0; sample < numSamples; ++sample) {
			newSamples.emplace_back(sampleDistributions.generateSample());
			Sample<value_t> & newSample = newSamples.back();

			const bool castForwardRay = !newSample.hasForwardResult();
			const bool castBackwardRay = !newSample.hasBackwardResult();
			if(castForwardRay) {
				rays.emplace_back(newSample.getForwardRay());
			}
			if(castBackwardRay) {
				rays.emplace_back(newSample.getBackwardRay());
			}
		}
		if(rays.empty()) {
			newSamples.clear();
			return false;
		}

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
		profiler.endTimeMemoryAction(sampleAction);
		auto rayAction = profiler.beginTimeMemoryAction("Ray casting");
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

		const auto results = RayCasting::RayCaster<float>::castRays(scene.get(), rays);

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
		profiler.endTimeMemoryAction(rayAction);
		auto resultAction = profiler.beginTimeMemoryAction("Result processing");
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

		rays.clear();
		auto result = results.cbegin();
		for(auto & newSample : newSamples) {
			const bool castForwardRay = !newSample.hasForwardResult();
			const bool castBackwardRay = !newSample.hasBackwardResult();

			if(castForwardRay) {
				if(result->first != nullptr) {
					newSample.setForwardResult(result->first, result->second);
				}
				++result;
			}
			if(castBackwardRay) {
				if(result->first != nullptr) {
					newSample.setBackwardResult(result->first, result->second);
				}
				++result;
			}

			if(newSample.getNumHits() == 0) {
				continue;
			}

			const auto originCell = rootViewCell->getNodeAtPosition(newSample.getOrigin());
			const auto contribution = updateWithSample(rootViewCell.get(), newSample, originCell);
			sampleDistributions.updateWithSample(newSample, contribution, originCell);

			meshSamples.emplace_back(newSample);
		}

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
		profiler.endTimeMemoryAction(resultAction);
		auto terminateAction = profiler.beginTimeMemoryAction("Termination check");
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

		const bool terminate = sampleDistributions.terminate();

#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING
		profiler.endTimeMemoryAction(terminateAction);
		auto updateAction = profiler.beginTimeMemoryAction("Distribution update");
#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING */

		sampleDistributions.calculateDistributionProbabilities();

		newSamples.clear();
		return terminate;
	}

	Rendering::Mesh * createMeshFromSamples() {
		Rendering::VertexDescription vertexDesc;
		vertexDesc.appendPosition3D();
		vertexDesc.appendColorRGBAByte();
		Rendering::MeshUtils::MeshBuilder meshBuilder(vertexDesc);
		for(const auto & sample : meshSamples) {
			Util::Color4ub endPointColor;
			switch(sample.getNumHits()) {
				case 1:
					endPointColor = Util::Color4ub(0, 0, 255, 255);
					break;
				case 2:
					endPointColor = Util::Color4ub(0, 255, 255, 255);
					break;
				default:
					// Skip invalid samples
					continue;
			}
			const Util::Color4ub originPointColor(0, 0, 0, 255);

			if(sample.hasForwardResult()) {
				meshBuilder.position(sample.getOrigin());
				meshBuilder.color(originPointColor);
				meshBuilder.addVertex();

				meshBuilder.position(sample.getForwardTerminationPoint());
				meshBuilder.color(endPointColor);
				meshBuilder.addVertex();
			}

			if(sample.hasBackwardResult()) {
				meshBuilder.position(sample.getOrigin());
				meshBuilder.color(originPointColor);
				meshBuilder.addVertex();

				meshBuilder.position(sample.getBackwardTerminationPoint());
				meshBuilder.color(endPointColor);
				meshBuilder.addVertex();
			}
		}

		Rendering::Mesh * mesh = meshBuilder.buildMesh();
		if(mesh != nullptr) {
			mesh->setUseIndexData(false);
			mesh->setDrawMode(Rendering::Mesh::DRAW_LINES);
		}
		meshSamples.clear();
		return mesh;
	}
};

AdaptiveGlobalVisibilitySampling::AdaptiveGlobalVisibilitySampling(GroupNode * scene,
																   ValuatedRegionNode * viewSpaceSubdivision) :
	impl(new Implementation<float>(scene, viewSpaceSubdivision)) {
}

AdaptiveGlobalVisibilitySampling::~AdaptiveGlobalVisibilitySampling() = default;

bool AdaptiveGlobalVisibilitySampling::performSampling(uint32_t numSamples) {
	return impl->performSampling(numSamples);
}

Rendering::Mesh * AdaptiveGlobalVisibilitySampling::createMeshFromSamples() const {
	return impl->createMeshFromSamples();
}

ValuatedRegionNode * AdaptiveGlobalVisibilitySampling::getViewCellHierarchy() const {
	return impl->rootViewCell.get();
}

}
}

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
