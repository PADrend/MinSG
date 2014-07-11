/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#include "Helper.h"
#include "PreprocessingContext.h"
#include "SamplePoint.h"
#include "VisibilitySphere.h"
#include "../Evaluator/Evaluator.h"
#include "../VisibilitySubdivision/CostEvaluator.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/CameraNodeOrtho.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../SceneManagement/SceneManager.h"
#include <Geometry/Box.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Sphere.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>
#include <Util/References.h>
#include <Util/StringIdentifier.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef MINSG_EXT_SVS_PROFILING
#include "../Profiling/Profiler.h"
#endif /* MINSG_EXT_SVS_PROFILING */

typedef Util::WrapperAttribute<MinSG::SVS::VisibilitySphere> VisibilitySphereAttribute;

namespace MinSG {
namespace SceneManagement {
class SceneManager;
}
}

namespace Rendering {
class Texture;
}
namespace MinSG {
namespace SVS {

static const Util::StringIdentifier CONTEXT_DATA_SCENEMANAGER("SceneManager");
typedef Util::WrapperAttribute<SceneManagement::SceneManager &> scene_manager_attribute_t;

std::string GATypeNameVisibilitySphere = "VisibilitySphere";
std::string GATypeNameVisibilitySphereDeprecated = "SamplingSphere";
static std::pair<std::string, std::string> serializeGAVisibilitySphere(const std::pair<const Util::GenericAttribute *, const Util::GenericAttributeMap *> & attributeAndContext) {
	auto context = attributeAndContext.second;
	auto sceneManagerAttribute = dynamic_cast<const scene_manager_attribute_t *>(context->getValue(CONTEXT_DATA_SCENEMANAGER));
	if(sceneManagerAttribute == nullptr) {
		throw std::logic_error("Serialization context does not contain a reference to the SceneManager.");
	}
	const SceneManagement::SceneManager & sceneManager = sceneManagerAttribute->ref();

	auto visibilitySphereAttribute = dynamic_cast<const VisibilitySphereAttribute *>(attributeAndContext.first);
	const auto & visibilitySphere = visibilitySphereAttribute->ref();
	std::ostringstream stream;
	stream.precision(std::numeric_limits<long double>::digits10);
	stream << visibilitySphere.getSphere() << ' ' << visibilitySphere.getSamples().size();
	for(const auto & sample : visibilitySphere.getSamples()) {
		stream << ' ' << sample.getPosition() << ' ';
		sample.getValue().serialize(stream, sceneManager);
	}
	return std::make_pair(GATypeNameVisibilitySphere, stream.str());
}
static VisibilitySphereAttribute * unserializeGAVisibilitySphere(const std::pair<std::string, const Util::GenericAttributeMap *> & contentAndContext) {
	auto context = contentAndContext.second;
	auto sceneManagerAttribute = dynamic_cast<const scene_manager_attribute_t *>(context->getValue(CONTEXT_DATA_SCENEMANAGER));
	if(sceneManagerAttribute == nullptr) {
		throw std::logic_error("Unserialization context does not contain a reference to the SceneManager.");
	}
	const SceneManagement::SceneManager & sceneManager = sceneManagerAttribute->ref();

	std::istringstream stream(contentAndContext.first);
	Geometry::Sphere_f sphere;
	stream >> sphere;
	std::size_t sampleCount;
	stream >> sampleCount;
	std::vector<SamplePoint> samples;
	samples.reserve(sampleCount);
	for(std::size_t s = 0; s < sampleCount; ++s) {
		Geometry::Vec3f pos;
		stream >> pos;
		SamplePoint sample(pos);
		sample.setValue(VisibilitySubdivision::VisibilityVector::unserialize(stream, sceneManager));
		samples.emplace_back(std::move(sample));
	}
	return new VisibilitySphereAttribute(std::move(sphere), std::move(samples));
}

static bool serializerRegisteredDeprecated = Util::GenericAttributeSerialization::registerSerializer<VisibilitySphereAttribute>(GATypeNameVisibilitySphereDeprecated, serializeGAVisibilitySphere, unserializeGAVisibilitySphere);
static bool serializerRegistered = Util::GenericAttributeSerialization::registerSerializer<VisibilitySphereAttribute>(GATypeNameVisibilitySphere, serializeGAVisibilitySphere, unserializeGAVisibilitySphere);

CameraNodeOrtho * createSamplingCamera(const Geometry::Sphere_f & sphere, const Geometry::Matrix4x4f & worldMatrix, int resolution) {
	const auto worldSphere = transformSphere(sphere, worldMatrix);
	const float radius = worldSphere.getRadius();

	auto camera = new CameraNodeOrtho();
	camera->setViewport(Geometry::Rect_i(0, 0, resolution, resolution));
	camera->setClippingPlanes(-radius, radius, -radius, radius);
	// Camera is standing radius away from the sphere surface.
	camera->setNearFar(radius, 3 * radius);

	return camera;
}

void transformCamera(AbstractCameraNode * camera, const Geometry::Sphere_f & sphere, const Geometry::Matrix4x4f & worldMatrix, const Geometry::Vec3f & position) {
	const auto worldSphere = transformSphere(sphere, worldMatrix);
	// Camera is standing radius away from the sphere surface.
	camera->setWorldPosition(worldSphere.getCenter() + position * 2 * worldSphere.getRadius());
	camera->rotateToWorldDir(position);
}

Rendering::Texture * createColorTexture(uint32_t width, uint32_t height, const VisibilitySphere & visibilitySphere, interpolation_type_t interpolation) {
	std::vector<std::size_t> values;
	values.reserve(width * height);
	std::size_t minValue = std::numeric_limits<std::size_t>::max();
	std::size_t maxValue = 0;

	const float inclinationFactor = 1.0f / static_cast<float>(height) * M_PI;
	const float azimuthFactor = 1.0f / static_cast<float>(width) * 2.0f * M_PI;

	for(uint_fast32_t y = 0; y < height; ++y) {
		for(uint_fast32_t x = 0; x < width; ++x) {
			const float inclination = static_cast<float>(y) * inclinationFactor;
			const float azimuth = static_cast<float>(x) * azimuthFactor;

			const Geometry::Vec3f position = Geometry::Sphere_f::calcCartesianCoordinateUnitSphere(inclination, azimuth);

			const auto vv = visibilitySphere.queryValue(position, interpolation);
			const std::size_t value = vv.getVisibleNodeCount();

			values[y * width + x] = value;
			if(value < minValue) {
				minValue = value;
			}
			if(value > maxValue) {
				maxValue = value;
			}
		}
	}

	Util::Reference<Util::Bitmap> bitmap = new Util::Bitmap(width, height, Util::PixelFormat::RGB);
	const uint32_t range = maxValue - minValue;
	Util::Reference<Util::PixelAccessor> accessor = Util::PixelAccessor::create(bitmap.get());
	if(range == 0) {
		WARN("Zero range.");
		accessor->fill(0, 0, width, height, Util::Color4ub(0, 0, 0, 255));
	} else {
		const double rangeFactor = 255.0 / static_cast<double>(range);
		for(uint_fast32_t y = 0; y < height; ++y) {
			for(uint_fast32_t x = 0; x < width; ++x) {
				const uint8_t colorValue = static_cast<double>(values[y * width + x] - minValue) * rangeFactor;
				Util::Color4ub color(0, colorValue, 0, 255);

				// Flip the image horizontally to make sure that it is correctly displayed on the sphere.
				accessor->writeColor(width - 1 - x, y, color);
			}
		}
	}
	return Rendering::TextureUtils::createTextureFromBitmap(*bitmap.get(), Rendering::TextureType::TEXTURE_2D,1, true).detachAndDecrease();
}

static Geometry::Sphere_f computeLocalBoundingSphere(PreprocessingContext & preprocessingContext __attribute__((unused)),
													 GeometryNode * geoNode) {
#ifdef MINSG_EXT_SVS_PROFILING
	auto & profiling = preprocessingContext.getProfiler();
	auto computeBoundingSphereAction = profiling.beginTimeMemoryAction("Compute leaf bounding sphere");
#endif /* MINSG_EXT_SVS_PROFILING */

	const auto geoSphere = Rendering::MeshUtils::calculateBoundingSphere(geoNode->getMesh());

#ifdef MINSG_EXT_SVS_PROFILING
	computeBoundingSphereAction.setValue(PreprocessingContext::ATTR_numVertices, Util::GenericAttribute::create(geoNode->getVertexCount()));
	computeBoundingSphereAction.setValue(PreprocessingContext::ATTR_sphereRadius, Util::GenericAttribute::create(geoSphere.getRadius()));
	profiling.endTimeMemoryAction(computeBoundingSphereAction);
#endif /* MINSG_EXT_SVS_PROFILING */

	return geoSphere;
}

static Geometry::Sphere_f computeLocalBoundingSphere(PreprocessingContext & preprocessingContext,
													 GroupNode * groupNode) {
	if(preprocessingContext.getComputeTightInnerBoundingSpheres()) {
		// Retrieve all meshes from the subtree
		const auto geoNodes = collectNodes<GeometryNode>(groupNode);
		std::vector<std::pair<Rendering::Mesh *, Geometry::Matrix4x4f>> meshesAndTransformations;
		meshesAndTransformations.reserve(geoNodes.size());
		for(const auto & geoNode : geoNodes) {
			// For simplicity, compute sphere in world coordinates
			meshesAndTransformations.emplace_back(geoNode->getMesh(), geoNode->getWorldMatrix());
		}

		const auto sphere = Rendering::MeshUtils::calculateBoundingSphere(meshesAndTransformations);
		// Transform sphere from world to local coordinates
		return transformSphere(sphere, groupNode->getWorldMatrix().inverse());
	} else {
		// Start with an invalid sphere
		Geometry::Sphere_f sphere(Geometry::Vec3f(0, 0, 0), -1);

		const auto nodeBB = groupNode->getBB();
		const auto nodeBBRadius = nodeBB.getBoundingSphereRadius();

		// Retrieve direct child nodes
		const auto children = getChildNodes(groupNode);
		for(const auto & childNode : children) {
			GroupNode * groupChild = dynamic_cast<GroupNode *>(childNode);
			if(groupChild != nullptr) {
				const VisibilitySphere & childVisibilitySphere = retrieveVisibilitySphere(groupChild);
				const auto & childSphere = childVisibilitySphere.getSphere();

				// Transform sphere from child local to parent local coordinates
				sphere.include(transformSphere(childSphere, groupChild->getMatrix()));
				continue;
			}
			GeometryNode * geoChild = dynamic_cast<GeometryNode *>(childNode);
			if(geoChild != nullptr) {
				const auto geoSphere = computeLocalBoundingSphere(preprocessingContext,
																  geoChild);

				// Transform sphere from child local to parent local coordinates
				sphere.include(transformSphere(geoSphere, geoChild->getMatrix()));
			}

			// Stop early if the radius of the combined sphere is too large
			if(nodeBBRadius < sphere.getRadius()) {
				return Geometry::Sphere_f(nodeBB.getCenter(), nodeBBRadius);
			}
		}

		return sphere;
	}
}

void preprocessNode(PreprocessingContext & preprocessingContext, GroupNode * node) {
	if(hasVisibilitySphere(node)) {
		const VisibilitySphere & visibilitySphere = retrieveVisibilitySphere(node);
		if(isVisibilitySphereValid(node, visibilitySphere)) {
			return;
		} else {
			removeVisibilitySphereUpwards(node);
		}
	}
	createVisibilitySphere(preprocessingContext, node);
}

#ifdef MINSG_EXT_SVS_PROFILING
static void enrichActionWithTreeInformation(Profiling::Action & action,
											const Node * node) {
	const auto geoNodes = collectNodes<GeometryNode>(node);
	action.setValue(PreprocessingContext::ATTR_numDescendantsGeometryNode, Util::GenericAttribute::create(geoNodes.size()));
	action.setValue(PreprocessingContext::ATTR_numDescendantsGroupNode, Util::GenericAttribute::create(collectNodes<GroupNode>(node).size()));
	uint32_t numTriangles = 0;
	for(const auto & geoNode : geoNodes) {
		numTriangles += geoNode->getTriangleCount();
	}
	action.setValue(PreprocessingContext::ATTR_numDescendantsTriangles, Util::GenericAttribute::create(numTriangles));
}

static void enrichActionWithVVInformation(Profiling::Action & action,
										  const VisibilitySubdivision::VisibilityVector & vv) {
	action.setValue(PreprocessingContext::ATTR_numGeometryNodesVisible, Util::GenericAttribute::create(vv.getVisibleNodeCount()));
	action.setValue(PreprocessingContext::ATTR_numTrianglesVisible, Util::GenericAttribute::create(vv.getTotalCosts()));
	action.setValue(PreprocessingContext::ATTR_numPixelsVisible, Util::GenericAttribute::create(vv.getTotalBenefits()));
}
#endif /* MINSG_EXT_SVS_PROFILING */

void createVisibilitySphere(PreprocessingContext & preprocessingContext, GroupNode * node) {
#ifdef MINSG_EXT_SVS_PROFILING
	auto & profiling = preprocessingContext.getProfiler();
	auto collectChildrenAction = profiling.beginTimeMemoryAction("Collect child nodes");
#endif /* MINSG_EXT_SVS_PROFILING */

	// Retrieve direct child nodes
	const auto children = getChildNodes(node);
	// Sort child nodes by type
	std::deque<GeometryNode *> childrenGeometryNode;
	std::deque<GroupNode *> childrenGroupNode;
	for(const auto & childNode : children) {
		GroupNode * groupNode = dynamic_cast<GroupNode *>(childNode);
		if(groupNode != nullptr) {
			childrenGroupNode.push_back(groupNode);
			continue;
		}
		GeometryNode * geoNode = dynamic_cast<GeometryNode *>(childNode);
		if(geoNode != nullptr) {
			childrenGeometryNode.push_back(geoNode);
		}
	}

#ifdef MINSG_EXT_SVS_PROFILING
	collectChildrenAction.setValue(PreprocessingContext::ATTR_numChildrenGeometryNode, Util::GenericAttribute::create(childrenGeometryNode.size()));
	collectChildrenAction.setValue(PreprocessingContext::ATTR_numChildrenGroupNode, Util::GenericAttribute::create(childrenGroupNode.size()));
	enrichActionWithTreeInformation(collectChildrenAction, node);
	profiling.endTimeMemoryAction(collectChildrenAction);

	auto collectSpheresAction = profiling.beginTimeMemoryAction("Collect visibility spheres");
#endif /* MINSG_EXT_SVS_PROFILING */

	// Collect visibility data from child nodes
	std::deque<const VisibilitySphere *> visibilitySpheres;
	for(const auto & groupNode : childrenGroupNode) {
		visibilitySpheres.push_back(&retrieveVisibilitySphere(groupNode));
	}

#ifdef MINSG_EXT_SVS_PROFILING
	profiling.endTimeMemoryAction(collectSpheresAction);
	auto computeBoundingSphereAction = profiling.beginTimeMemoryAction("Compute inner bounding sphere");
#endif /* MINSG_EXT_SVS_PROFILING */

	const auto sphere = computeLocalBoundingSphere(preprocessingContext, node);

#ifdef MINSG_EXT_SVS_PROFILING
	computeBoundingSphereAction.setValue(PreprocessingContext::ATTR_sphereRadius, Util::GenericAttribute::create(sphere.getRadius()));
	profiling.endTimeMemoryAction(computeBoundingSphereAction);
#endif /* MINSG_EXT_SVS_PROFILING */

	VisibilitySubdivision::CostEvaluator evaluator(Evaluators::Evaluator::SINGLE_VALUE);

	preprocessingContext.getFrameContext().pushCamera();
	Util::Reference<CameraNodeOrtho> camera = createSamplingCamera(sphere, node->getWorldMatrix(), static_cast<int>(preprocessingContext.getResolution()));

	// Check if there are only leaves below this node
	if(!preprocessingContext.getUseExistingVisibilityResults() ||
			childrenGroupNode.empty()) {
#ifdef MINSG_EXT_SVS_PROFILING
		auto triangulationAction = profiling.beginTimeMemoryAction("Sample point triangulation");
#endif /* MINSG_EXT_SVS_PROFILING */

		std::vector<SamplePoint> samplePoints(preprocessingContext.getPositions().begin(), preprocessingContext.getPositions().end());
		VisibilitySphere visibilitySphere(sphere, samplePoints);

#ifdef MINSG_EXT_SVS_PROFILING
		profiling.endTimeMemoryAction(triangulationAction);

		auto testVisibilityAction = profiling.beginTimeMemoryAction("Test visibility");
#endif /* MINSG_EXT_SVS_PROFILING */

		visibilitySphere.evaluateAllSamples(preprocessingContext.getFrameContext(), evaluator, camera.get(), node);

#ifdef MINSG_EXT_SVS_PROFILING
		const auto vv = visibilitySphere.queryValue(Geometry::Vec3f(), INTERPOLATION_MAXALL);
		enrichActionWithVVInformation(testVisibilityAction, vv);
		enrichActionWithTreeInformation(testVisibilityAction, node);
		profiling.endTimeMemoryAction(testVisibilityAction);
#endif /* MINSG_EXT_SVS_PROFILING */

		storeVisibilitySphere(node, std::move(visibilitySphere));
	} else {
#ifdef MINSG_EXT_SVS_PROFILING
		auto mergeSpheresAction = profiling.beginTimeMemoryAction("Merge spheres");
#endif /* MINSG_EXT_SVS_PROFILING */

		VisibilitySphere visibilitySphere(sphere, visibilitySpheres);
		std::sort(childrenGeometryNode.begin(), childrenGeometryNode.end());

#ifdef MINSG_EXT_SVS_PROFILING
		profiling.endTimeMemoryAction(mergeSpheresAction);

		auto testVisibilityAction = profiling.beginTimeMemoryAction("Test visibility");
#endif /* MINSG_EXT_SVS_PROFILING */

		visibilitySphere.evaluateAllSamples(preprocessingContext.getFrameContext(), evaluator, camera.get(), node, visibilitySpheres, childrenGeometryNode);

#ifdef MINSG_EXT_SVS_PROFILING
		const auto vv = visibilitySphere.queryValue(Geometry::Vec3f(), INTERPOLATION_MAXALL);
		enrichActionWithVVInformation(testVisibilityAction, vv);
		enrichActionWithTreeInformation(testVisibilityAction, node);
		profiling.endTimeMemoryAction(testVisibilityAction);
#endif /* MINSG_EXT_SVS_PROFILING */

		storeVisibilitySphere(node, std::move(visibilitySphere));
	}

	preprocessingContext.getFrameContext().popCamera();
}

static const Util::StringIdentifier attributeId("VisibilitySphere");
static const Util::StringIdentifier attributeIdDeprecated("SamplingSphere");

bool isVisibilitySphereValid(GroupNode * node, const VisibilitySphere & visibilitySphere) {
	const auto & samples = visibilitySphere.getSamples();
	if(samples.empty()) {
		// A visibility sphere without samples should not be treated as valid.
		return false;
	}
	// Check if the node has been cloned.
	const auto & vv = samples.front().getValue();
	if(vv.getIndexCount() == 0) {
		// An empty sample should be treated as invalid.
		return false;
	}
	// Simply fetch the first node.
	const Node * someNode = vv.getNode(0);
	while(someNode->hasParent()) {
		GroupNode * parent = someNode->getParent();
		if(parent == node) {
			// The node from the sample is from this subtree.
			return true;
		}
		someNode = parent;
	}
	return false;
}

bool hasVisibilitySphere(GroupNode * node) {
	return node->isAttributeSet(attributeId) || node->isAttributeSet(attributeIdDeprecated);
}

static VisibilitySphere & accessVisibilitySphere(GroupNode * node) {
	Util::GenericAttribute * attribute = node->getAttribute(attributeId);
	if(attribute == nullptr) {
		// Check if visibility sphere is stored under the depreacted name.
		Util::GenericAttribute * deprecatedAttribute = node->getAttribute(attributeIdDeprecated);
		if(deprecatedAttribute == nullptr) {
			throw std::logic_error("Attribute not found");
		}
		// Rename the attribute to the new name.
		node->setAttribute(attributeId, deprecatedAttribute->clone());
		node->unsetAttribute(attributeIdDeprecated);
		attribute = node->getAttribute(attributeId);
	}
	VisibilitySphereAttribute * visibilitySphereAttribute = dynamic_cast<VisibilitySphereAttribute *>(attribute);
	if(visibilitySphereAttribute == nullptr) {
		throw std::logic_error("Attribute has wrong type");
	}
	return visibilitySphereAttribute->ref();
}

const VisibilitySphere & retrieveVisibilitySphere(GroupNode * node) {
	return accessVisibilitySphere(node);
}

void storeVisibilitySphere(GroupNode * node, VisibilitySphere && visibilitySphere) {
	Util::GenericAttribute * attribute = node->getAttribute(attributeId);
	if(attribute != nullptr) {
		throw std::logic_error("Attribute already exists");
	}
	auto visibilitySphereAttribute = new VisibilitySphereAttribute(std::move(visibilitySphere));
	node->setAttribute(attributeId, visibilitySphereAttribute);
}

void removeVisibilitySphereUpwards(GroupNode * node) {
	for(Node * someNode = node; someNode != nullptr; someNode = someNode->getParent()) {
		node->unsetAttribute(attributeId);
		node->unsetAttribute(attributeIdDeprecated);
	}
}

static void transformSphereWorlToLocal(GroupNode * node) {
	if(!hasVisibilitySphere(node)) {
		return;
	}
	VisibilitySphere & visibilitySphere = accessVisibilitySphere(node);
	const auto & oldSphere = visibilitySphere.getSphere();
	const auto inverseWorldMatrix = node->getWorldMatrix().inverse();
	visibilitySphere.setSphere(transformSphere(oldSphere, inverseWorldMatrix));
}

void transformSpheresFromWorldToLocal(GroupNode * rootNode) {
	forEachNodeBottomUp<GroupNode>(rootNode, &transformSphereWorlToLocal);
}

interpolation_type_t interpolationFromUInt(uint32_t number) {
	switch(number) {
		case INTERPOLATION_NEAREST:
			return INTERPOLATION_NEAREST;
		case INTERPOLATION_MAX3:
			return INTERPOLATION_MAX3;
		case INTERPOLATION_MAXALL:
			return INTERPOLATION_MAXALL;
		case INTERPOLATION_WEIGHTED3:
			return INTERPOLATION_WEIGHTED3;
		default:
			break;
	}
	throw std::invalid_argument("Invalid interpolation method.");
}

std::string interpolationToString(interpolation_type_t interpolation) {
	switch(interpolation) {
		case INTERPOLATION_NEAREST:
			return "NEAREST";
		case INTERPOLATION_MAX3:
			return "MAX3";
		case INTERPOLATION_MAXALL:
			return "MAXALL";
		case INTERPOLATION_WEIGHTED3:
			return "WEIGHTED3";
		default:
			break;
	}
	throw std::invalid_argument("Invalid interpolation method.");
}

interpolation_type_t interpolationFromString(const std::string & str) {
	static const Util::StringIdentifier idNEAREST("NEAREST");
	static const Util::StringIdentifier idMAX3("MAX3");
	static const Util::StringIdentifier idMAXALL("MAXALL");
	static const Util::StringIdentifier idWEIGHTED3("WEIGHTED3");

	const Util::StringIdentifier idStr(str);
	if(idStr == idNEAREST) {
		return INTERPOLATION_NEAREST;
	} else if(idStr == idMAX3) {
		return INTERPOLATION_MAX3;
	} else if(idStr == idMAXALL) {
		return INTERPOLATION_MAXALL;
	} else if(idStr == idWEIGHTED3) {
		return INTERPOLATION_WEIGHTED3;
	}
	throw std::invalid_argument("Invalid interpolation method.");
}

}
}

#endif /* MINSG_EXT_SVS */
