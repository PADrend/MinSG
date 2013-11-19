/*
	This file is part of the MinSG library extension SphericalSampling.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SPHERICALSAMPLING

#include "SamplingSphere.h"
#include "Helper.h"
#include "../Evaluator/Evaluator.h"
#include "../Triangulation/Delaunay3d.h"
#include "../Triangulation/Helper.h"
#include "../Triangulation/TetrahedronWrapper.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/CameraNodeOrtho.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Point.h>
#include <Geometry/Rect.h>
#include <Geometry/Triangle.h>
#include <Rendering/MeshUtils/PlatonicSolids.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/GenericAttribute.h>
#include <array>
#include <deque>
#include <functional>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <vector>

using namespace MinSG::VisibilitySubdivision;

namespace MinSG {
namespace SphericalSampling {

struct SampleEntry : public Geometry::Point<Geometry::Vec3f> {
	static const size_t INVALID_INDEX;
	const size_t sampleIndex;

	SampleEntry() :
			Geometry::Point<Geometry::Vec3f>(Geometry::Vec3f()), sampleIndex(INVALID_INDEX) {
	}
	explicit SampleEntry(const Geometry::Vec3f & samplePosition, size_t index) :
			Geometry::Point<Geometry::Vec3f>(samplePosition), sampleIndex(index) {
	}
};
const size_t SampleEntry::INVALID_INDEX = std::numeric_limits<size_t>::max();

static Triangulation::Delaunay3d<SampleEntry> * triangulateSamplePoints(std::vector<SamplePoint> & samples) {
	auto * triangulation = new Triangulation::Delaunay3d<SampleEntry>;
	// Insert center of sphere without data.
	triangulation->addPoint(SampleEntry());

	const size_t sampleCount = samples.size();
	for(size_t i = 0; i < sampleCount; ++i) {
		triangulation->addPoint(SampleEntry(samples[i].getPosition(), i));
	}
	triangulation->validate();

	try {
		// Check the triangulation for invalid tetrahedrons
		triangulation->generate([](const Triangulation::TetrahedronWrapper<SampleEntry> & tetrahedron) {
			size_t validSamples = 0;
			if(tetrahedron.getA()->sampleIndex != SampleEntry::INVALID_INDEX) {
				++validSamples;
			}
			if(tetrahedron.getB()->sampleIndex != SampleEntry::INVALID_INDEX) {
				++validSamples;
			}
			if(tetrahedron.getC()->sampleIndex != SampleEntry::INVALID_INDEX) {
				++validSamples;
			}
			if(tetrahedron.getD()->sampleIndex != SampleEntry::INVALID_INDEX) {
				++validSamples;
			}
			if(validSamples != 3) {
				std::ostringstream message;
				message.precision(std::numeric_limits<long double>::digits10);
				message << "Tetrahedron does not contain exactly one invalid vertex.\n"
						<< "A: index=" << tetrahedron.getA()->sampleIndex << '\n'
						<< "   position=" << tetrahedron.getA()->getPosition() << '\n'
						<< "B: index=" << tetrahedron.getB()->sampleIndex << '\n'
						<< "   position=" << tetrahedron.getB()->getPosition() << '\n'
						<< "C: index=" << tetrahedron.getC()->sampleIndex << '\n'
						<< "   position=" << tetrahedron.getC()->getPosition() << '\n'
						<< "D: index=" << tetrahedron.getD()->sampleIndex << '\n'
						<< "   position=" << tetrahedron.getD()->getPosition()  << '\n';
				throw std::runtime_error(message.str());
			}
		});
	} catch(const std::runtime_error & runtimeError) {
		if(samples.size() != 20) {
			throw;
		}
		WARN("Sample points invalid. Trying to generate 20 new sample positions using the vertices of a dodecahedron.");
		using namespace Rendering::MeshUtils::PlatonicSolids;
		Util::Reference<Rendering::Mesh> mesh = createDodecahedron();
		auto accessor = Rendering::PositionAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
		for(std::size_t i = 0; accessor->checkRange(i); ++i) {
			const auto newPosition = accessor->getPosition(i);
			if(samples[i].getPosition().distance(newPosition) > 1.0e-6f) {
				throw;
			}
			SamplePoint newSamplePoint(newPosition);
			newSamplePoint.setValue(samples[i].getValue());
			samples[i] = newSamplePoint;
		}
		WARN("Update of invalid sample points successful.");
		return triangulateSamplePoints(samples);
	}
	return triangulation;
}

SamplingSphere::SamplingSphere(Geometry::Sphere_f  _sphere, const std::vector<SamplePoint> & _samples) :
	sphere(std::move(_sphere)), samples(_samples), triangulation(), minimumScaleFactor(1.0f) {
	if(_samples.empty()) {
		throw std::invalid_argument("Array of sample points is empty.");
	}

	triangulation.reset(triangulateSamplePoints(samples));
}

SamplingSphere::SamplingSphere(Geometry::Sphere_f && _sphere, std::vector<SamplePoint> && _samples) :
		sphere(std::forward<Geometry::Sphere_f>(_sphere)),
		samples(std::forward<std::vector<SamplePoint>>(_samples)),
		triangulation(),
		minimumScaleFactor(1.0f) {
	if(samples.empty()) {
		throw std::invalid_argument("Array of sample points is empty.");
	}

	triangulation.reset(triangulateSamplePoints(samples));
}

SamplingSphere::SamplingSphere(Geometry::Sphere_f  newSphere,
							   const std::deque<const SamplingSphere *> & samplingSpheres) :
	sphere(std::move(newSphere)), samples(), triangulation(), minimumScaleFactor(1.0f) {
	if(samplingSpheres.empty()) {
		throw std::invalid_argument("Array of sample points is empty.");
	}

	// Check consistency
	const std::size_t numSpheres = samplingSpheres.size();
	const SamplingSphere * firstSphere = samplingSpheres.front();
	const std::size_t firstNumSamples = firstSphere->samples.size();
	for(std::size_t s = 0; s < firstNumSamples; ++s) {
		const Geometry::Vec3f & firstPos = firstSphere->samples[s].getPosition();
		for(std::size_t i = 1; i < numSpheres; ++i) {
			const Geometry::Vec3f & currentPos = samplingSpheres[i]->samples[s].getPosition();
			if(firstPos != currentPos) {
				throw std::invalid_argument("Sample positions are not consistent.");
			}
		}
	}

	samples = firstSphere->samples;
	triangulation = firstSphere->triangulation;
}

bool SamplingSphere::operator==(const SamplingSphere & other) const {
	return sphere == other.sphere && samples == other.samples;
}

ListNode * SamplingSphere::getTriangulationMinSGNodes() const {
	return Triangulation::createMinSGNodes(*triangulation.get(), false);
}

static void evaluateSample(SamplePoint & sample,
						   const Geometry::Sphere_f & sphere,
						   CameraNodeOrtho * camera,
						   FrameContext & frameContext,
						   Evaluators::Evaluator & evaluator,
						   Node * node) {
	transformCamera(camera, sphere, node->getWorldMatrix(), sample.getPosition());
	frameContext.setCamera(camera);

	evaluator.beginMeasure();
	evaluator.measure(frameContext, *node, Geometry::Rect(camera->getViewport()));
	evaluator.endMeasure(frameContext);

	const Util::GenericAttribute * result = evaluator.getResults()->front();
	const auto * vva = dynamic_cast<const VisibilityVectorAttribute *>(result);
	if(vva == nullptr) {
		throw std::invalid_argument("Invalid evaluator.");
	}
	sample.setValue(vva->ref());
}

void SamplingSphere::evaluateAllSamples(FrameContext & frameContext,
										Evaluators::Evaluator & evaluator,
										CameraNodeOrtho * camera,
										Node * node) {
	for(auto & sample : samples) {
		evaluateSample(sample, sphere, camera, frameContext, evaluator, node);
	}
}

void SamplingSphere::evaluateAllSamples(FrameContext & frameContext,
										Evaluators::Evaluator & evaluator,
										CameraNodeOrtho * camera,
										Node * node,
										const std::deque<const SamplingSphere *> & samplingSpheres,
										const std::deque<GeometryNode *> & explicitNodes) {
	auto allNodes = collectNodes<GeometryNode>(node);
	std::sort(allNodes.begin(), allNodes.end());

	const std::size_t numSamples = samples.size();
	for(std::size_t s = 0; s < numSamples; ++s) {
		// Combine results from all spheres to get all visible nodes
		VisibilityVector maxVV;
		for(const auto & samplingSphere : samplingSpheres) {
			const auto & currentValue = samplingSphere->samples[s].getValue();
			if(maxVV.getVisibleNodeCount() == 0) {
				maxVV = currentValue;
			} else {
				maxVV = VisibilityVector::makeMax(maxVV, currentValue);
			}
		}
		std::deque<GeometryNode *> visibleNodes;
		for(uint32_t index = 0; index < maxVV.getIndexCount(); ++index) {
			visibleNodes.push_back(maxVV.getNode(index));
		}
		std::sort(visibleNodes.begin(), visibleNodes.end());

		// Nodes that will stay activated
		std::deque<GeometryNode *> activeNodes;
		// activeNodes = visibleNodes + explicitNodes
		std::set_union(visibleNodes.cbegin(), visibleNodes.cend(),
					   explicitNodes.cbegin(), explicitNodes.cend(),
					   std::back_inserter(activeNodes));

		// Nodes that will be deactivated
		std::deque<GeometryNode *> inactiveNodes;
		// inactiveNodes = allNodes - activeNodes
		std::set_difference(allNodes.cbegin(), allNodes.cend(),
							activeNodes.cbegin(), activeNodes.cend(),
							std::back_inserter(inactiveNodes));

		if(activeNodes.size() + inactiveNodes.size() != allNodes.size()) {
			throw std::runtime_error("Set operations failed.");
		}

		std::for_each(inactiveNodes.begin(), inactiveNodes.end(), std::mem_fn(&Node::deactivate));
		evaluateSample(samples[s], sphere, camera, frameContext, evaluator, node);
		std::for_each(inactiveNodes.begin(), inactiveNodes.end(), std::mem_fn(&Node::activate));
	}
}

size_t SamplingSphere::getMemoryUsage() const {
	size_t size = sizeof(SamplingSphere);
	for(const auto & sample : samples) {
		size += sample.getMemoryUsage();
	}
	// This might be inaccurate, becaues a triangulation can be shared by spheres.
	size += triangulation->getMemoryUsage() /* / triangulation.use_count()*/;
	return size;
}

VisibilityVector SamplingSphere::queryValue(const Geometry::Vec3f & query, interpolation_type_t interpolationMethod) const {
	if(interpolationMethod == INTERPOLATION_NEAREST) {
		std::size_t closestSampleIndex = 0;
		float minSquaredDistance = samples[0].getPosition().distanceSquared(query);
		for(std::size_t s = 1; s < samples.size(); ++s) {
			const float currentSquaredDistance = samples[s].getPosition().distanceSquared(query);
			if(currentSquaredDistance < minSquaredDistance) {
				closestSampleIndex = s;
				minSquaredDistance = currentSquaredDistance;
			}
		}
		return samples[closestSampleIndex].getValue();
	} else if(interpolationMethod == INTERPOLATION_MAXALL) {
		VisibilityVector result;
		for(const auto & sample : samples) {
			const auto & currentValue = sample.getValue();
			if(result.getVisibleNodeCount() == 0) {
				result = currentValue;
			} else {
				result = VisibilityVector::makeMax(result, currentValue);
			}
		}
		return result;
	}

	const Triangulation::TetrahedronWrapper<SampleEntry> * tetrahedron = triangulation->findTetrahedron(query * minimumScaleFactor, 1.0e-6f);
	while(tetrahedron == nullptr) {
		minimumScaleFactor *= 0.9;
		if(minimumScaleFactor < 0.1) {
			throw std::underflow_error("Minimum scale factor too small. Error in triangulation search.");
		}
		tetrahedron = triangulation->findTetrahedron(query * minimumScaleFactor, 1.0e-6f);
	}
	if(tetrahedron == nullptr) {
		throw std::runtime_error("No tetrahedron found.");
	}
	std::array<const SamplePoint *, 3> nearestSamples;
	auto arrayIt = nearestSamples.begin();
	if(tetrahedron->getA()->sampleIndex != SampleEntry::INVALID_INDEX) {
		*arrayIt++ = &samples[tetrahedron->getA()->sampleIndex];
	}
	if(tetrahedron->getB()->sampleIndex != SampleEntry::INVALID_INDEX) {
		*arrayIt++ = &samples[tetrahedron->getB()->sampleIndex];
	}
	if(tetrahedron->getC()->sampleIndex != SampleEntry::INVALID_INDEX) {
		*arrayIt++ = &samples[tetrahedron->getC()->sampleIndex];
	}
	if(tetrahedron->getD()->sampleIndex != SampleEntry::INVALID_INDEX) {
		*arrayIt++ = &samples[tetrahedron->getD()->sampleIndex];
	}
	if(std::distance(nearestSamples.begin(), arrayIt) != 3) {
		throw std::runtime_error("Tetrahedron does not contain exactly one invalid vertex.");
	}
	if(interpolationMethod == INTERPOLATION_WEIGHTED3) {
			Geometry::Triangle<Geometry::Vec3f> triangle(nearestSamples[0]->getPosition(),
														 nearestSamples[1]->getPosition(),
														 nearestSamples[2]->getPosition());
			Geometry::Vec3f bc;
			if(triangle.calcNormal().dot(query) < 0) {
				triangle = Geometry::Triangle<Geometry::Vec3f>(nearestSamples[2]->getPosition(),
															   nearestSamples[1]->getPosition(),
															   nearestSamples[0]->getPosition());
				triangle.closestPoint(query, bc);
				bc = Geometry::Vec3f(bc.getZ(), bc.getY(), bc.getX());
			} else {
				triangle.closestPoint(query, bc);
			}
			return VisibilityVector::makeWeightedThree(bc.getX(), nearestSamples[0]->getValue(),
													   bc.getY(), nearestSamples[1]->getValue(),
													   bc.getZ(), nearestSamples[2]->getValue());
	}
	// interpolationMethod == INTERPOLATION_MAX3
	return VisibilityVector::makeMax(VisibilityVector::makeMax(nearestSamples[0]->getValue(),
															   nearestSamples[1]->getValue()),
									 nearestSamples[2]->getValue());
}

}
}

#endif /* MINSG_EXT_SPHERICALSAMPLING */
