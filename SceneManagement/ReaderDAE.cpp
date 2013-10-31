/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ReaderDAE.h"
#include "SceneDescription.h"
#include <Geometry/Matrix4x4.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/Serialization/Serialization.h>
#include <Util/Macros.h>
#include <Util/MicroXML.h>
#include <Util/StringUtils.h>
#include <Util/Utils.h>
#include <cassert>
#include <deque>
#include <iostream>
#include <iterator>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <stack>

namespace MinSG {
namespace SceneManagement {
namespace ReaderDAE {

static const Util::StringIdentifier DAE_DATA("_DAEData_");
static const Util::StringIdentifier DAE_TAG_TYPE("_DAEType_");
static const Util::StringIdentifier DAE_SUBTREE_ID("_DAESubtreeId_");
static const Util::StringIdentifier DAE_CHILDREN("_DAEChildren_");

static const Util::StringIdentifier DAE_TAG_TYPE_ACCESSOR("accessor");
static const Util::StringIdentifier DAE_TAG_TYPE_CHANNEL("channel");
static const Util::StringIdentifier DAE_TAG_TYPE_GEOMETRY("geometry");
static const Util::StringIdentifier DAE_TAG_TYPE_INSTANCE_CAMERA("instance_camera");
static const Util::StringIdentifier DAE_TAG_TYPE_INSTANCE_LIGHT("instance_light");
static const Util::StringIdentifier DAE_TAG_TYPE_INSTANCE_GEOMETRY("instance_geometry");
static const Util::StringIdentifier DAE_TAG_TYPE_INSTANCE_CONTROLLER("instance_controller");
static const Util::StringIdentifier DAE_TAG_TYPE_INSTANCE_VISUAL_SCENE("instance_visual_scene");
static const Util::StringIdentifier DAE_TAG_TYPE_MATRIX("matrix");
static const Util::StringIdentifier DAE_TAG_TYPE_NODE("node");
static const Util::StringIdentifier DAE_TAG_TYPE_ROTATE("rotate");
static const Util::StringIdentifier DAE_TAG_TYPE_SCALE("scale");
static const Util::StringIdentifier DAE_TAG_TYPE_TRANSLATE("translate");
static const Util::StringIdentifier DAE_TAG_TYPE_UP_AXIS("up_axis");

static const Util::StringIdentifier DAE_ATTR_BIND_MATRIX("bind_matrix");
static const Util::StringIdentifier DAE_ATTR_COUNT("count");
static const Util::StringIdentifier DAE_ATTR_FLOAT_ARRAY("float_array");
static const Util::StringIdentifier DAE_ATTR_ID("id");
static const Util::StringIdentifier DAE_ATTR_INPUT_SET("input_set");
static const Util::StringIdentifier DAE_ATTR_OFFSET("offset");
static const Util::StringIdentifier DAE_ATTR_MATERIAL("material");
static const Util::StringIdentifier DAE_ATTR_NAME_ARRAY("name_array");
static const Util::StringIdentifier DAE_ATTR_SEMANTIC("semantic");
static const Util::StringIdentifier DAE_ATTR_SID("sid");
static const Util::StringIdentifier DAE_ATTR_SOURCE("source");
static const Util::StringIdentifier DAE_ATTR_STRIDE("stride");
static const Util::StringIdentifier DAE_ATTR_SYMBOL("symbol");
static const Util::StringIdentifier DAE_ATTR_TARGET("target");
static const Util::StringIdentifier DAE_ATTR_TEXCOORD("texcoord");
static const Util::StringIdentifier DAE_ATTR_TEXTURE("texture");
static const Util::StringIdentifier DAE_ATTR_URL("url");

typedef Util::WrapperAttribute<std::deque<float>> float_data_t;

//! Add the given child description to the Consts::CHILDREN-List of the container.
static void addToMinSGChildren(NodeDescription * container, NodeDescription * child) {
	if(child != nullptr) {
		auto children = dynamic_cast<NodeDescriptionList *>(container->getValue(Consts::CHILDREN));
		if (children == nullptr) {
			children = new NodeDescriptionList;
			container->setValue(Consts::CHILDREN, children);
		}
		children->push_back(child);
	}
}

//! Add the given child description to the DAE_CHILDREN-List of the container.
static void addToDAEChildren(NodeDescription * container, NodeDescription * child) {
	if(child != nullptr) {
		auto children = dynamic_cast<Util::GenericAttributeList *>(container->getValue(DAE_CHILDREN));
		if (children == nullptr) {
			children = new Util::GenericAttributeList;
			container->setValue(DAE_CHILDREN, children);
		}
		children->push_back(child);
	}
}

/**
 * Search the container and all children for a element of the given
 * type. If there are more than one node with the given type
 * then one of them will be returned.
 *
 * @param container Container to search.
 * @param type Type which should be searched.
 * @return A node description with the given type or @c nullptr if
 * no such node could be found.
 */
static const NodeDescription * findElementByType(const NodeDescription * container, const char * type) {
	if (container == nullptr) {
		return nullptr;
	}
	if (container->getString(DAE_TAG_TYPE) == type) {
		return container;
	}
	auto children = dynamic_cast<const NodeDescriptionList *>(container->getValue(DAE_CHILDREN));
	if (children == nullptr) {
		return nullptr;
	}
	for (auto & elem : *children) {
		const NodeDescription * result = findElementByType(dynamic_cast<const NodeDescription *>(elem.get()), type);
		if (result != nullptr) {
			return result;
		}
	}
	return nullptr;
}

/**
 * Search the container and all children for element of the given type.
 * Add all elements that match the given type to the container @a elements.
 *
 * @param container Container to search.
 * @param type Type which should be searched.
 * @param elements Container that is used to store the result.
 */
static void findElementsByType(const NodeDescription * container, const char * type, std::deque<const NodeDescription *> & elements) {
	if (container == nullptr) {
		return;
	}
	if (container->getString(DAE_TAG_TYPE) == type) {
		elements.push_back(container);
		return;
	}
	auto children = dynamic_cast<const NodeDescriptionList *>(container->getValue(DAE_CHILDREN));
	if (children == nullptr) {
		return;
	}
	for (auto & elem : *children) {
		findElementsByType(dynamic_cast<const NodeDescription *>(elem.get()), type, elements);
	}
}

static Geometry::Matrix4x4f getMatrixFromDescription(NodeDescription & description) {
	std::istringstream matrixStream(description.getString(Consts::ATTR_MATRIX));
	Geometry::Matrix4x4f matrix;
	matrixStream >> matrix;
	return matrix;
}

static void setMatrixToDescription(NodeDescription & description, const Geometry::Matrix4x4f & matrix) {
	std::ostringstream matrixStream;
	matrixStream << matrix;
	description.setValue(Consts::ATTR_MATRIX, Util::GenericAttribute::createString(matrixStream.str()));
}

struct VertexPart {
	enum type_t {
		POSITION, NORMAL, COLOR, TEXCOORD
	} type;
	uint16_t indexOffset;
	uint16_t stride;
	std::vector<float> data;

	//! Fill the internal fields from the given @c <source> description.
	bool fill(const std::string & semantic, const uint16_t offset, const NodeDescription * desc) {
		const NodeDescription * floatArray = findElementByType(desc, "float_array");
		if (floatArray == nullptr) {
			WARN("No float array found.");
			return false;
		}

		stride = Util::StringUtils::toNumber<uint16_t>(floatArray->getString(DAE_ATTR_STRIDE));

		if (semantic == "POSITION") {
			if (stride != 3) {
				WARN("Only stride equal to 3 is supported for positions.");
				return false;
			}
			type = POSITION;
		} else if (semantic == "NORMAL") {
			if (stride != 3) {
				WARN("Only stride equal to 3 is supported for normals.");
				return false;
			}
			type = NORMAL;
		} else if (semantic == "COLOR") {
			if (stride != 3 && stride != 4) {
				WARN("Only stride equal to 3 or 4 is supported for colors.");
				return false;
			}
			type = COLOR;
		} else if (semantic == "TEXCOORD") {
			if (stride != 2) {
				WARN("Only stride equal to two is supported for texture coordinates.");
				return false;
			}
			type = TEXCOORD;
		} else {
			WARN("Unsupported semantic.");
			return false;
		}
		indexOffset = offset;

		float_data_t * floatData = dynamic_cast<float_data_t *>(floatArray->getValue(DAE_DATA));
		if (floatData == nullptr) {
			WARN("Wrong data format.");
			return false;
		}
		std::copy(floatData->ref().begin(), floatData->ref().end(), std::back_inserter(data));

		if (data.size() != Util::StringUtils::toNumber<size_t>(floatArray->getString(DAE_ATTR_COUNT))) {
			WARN("Vertex count does not match.");
			return false;
		}

		return true;
	}
};

struct VisitorContext {
		VisitorContext(bool invertTransparency) :
			scene(nullptr), 
#ifdef MINSG_EXT_SKELETAL_ANIMATION
			jointIdCount(0),
			meshDescription(),
			jointMap(),
			channelDesc(nullptr),
#endif
			flag_invertTransparency(invertTransparency),
			stats_tagCounter(0), 
			stats_meshCounter(0),
			stats_zeroPolylistCounter(0),
			ignorePhysics(false),
			coordinateSystem(Y_UP) {
		}

		bool enter(const std::string & tagName, const Util::MicroXML::attributes_t & attributes);
		bool leave(const std::string & tagName);
		bool data(const std::string & tagName, const std::string & data);

	//!	@name Data members
	//	@{
		NodeDescription * scene;

#ifdef MINSG_EXT_SKELETAL_ANIMATION
		int jointIdCount;
		std::map<std::string, NodeDescription *> meshDescription;
		std::map<std::string, uint32_t> jointMap;
		NodeDescription * channelDesc;
#endif

		bool flag_invertTransparency;

		// stats
		int stats_tagCounter;
		int stats_meshCounter;
		int stats_zeroPolylistCounter;

		// registries
		//! Mapping from id to a node in the tree.
		std::map<std::string, NodeDescription *> allElementsRegistry;

		typedef std::deque<std::pair<Util::Reference<Rendering::Mesh>, std::string> > mesh_list_t;
		//! Registry for meshes. Maps from one id to a list which contains (mesh, materialId).
		std::map<std::string, mesh_list_t> meshRegistry;

		struct Material {
			Material() : stateDescription(nullptr) {}
			NodeDescription * stateDescription;
		};

		//! Registry for materials. Maps from id to material description.
		std::map<std::string, Material> materialRegistry;

#ifdef MINSG_EXT_SKELETAL_ANIMATION
		// skeletal animation structure for skinning
		struct SkinningPairs {
			float vcount;
			std::vector<float> jointId;
			std::vector<float> weight;
		};
		//std::vector<uint32_t> vertexId;
#endif

		// misc
		std::unique_ptr<NodeDescription> rootElement;
		std::stack<NodeDescription *> elementStack;
		std::stack<std::string> idStack;

		//! Set to @c true when entering a physics section.
		bool ignorePhysics;

		enum coordinate_system_t {
			X_UP,
			Y_UP,
			Z_UP
		} coordinateSystem;

	//	@}

		/**
		 * Return the node description reference by the @a attribute in
		 * @a node.
		 *
		 * @param sourceElement Element which contains the referring attribute.
		 * @param refAttrName Name of the attribute containing the id of the searched element.
		 * @return Referenced node or @c nullptr if the node was not found.
		 */
		NodeDescription * findElementByRef(const NodeDescription * sourceElement, const Util::StringIdentifier & refAttrName) const;
		NodeDescription * findElementById(const std::string & id, bool warn = true) const;

		//! Return new meshes created from the information in @a geometryDesc.
		bool createMeshes(const NodeDescription * geometryDesc, mesh_list_t & meshList) ;

		//! Build a single mesh from the given vertex and index data.
#ifdef MINSG_EXT_SKELETAL_ANIMATION
		static Rendering::Mesh * createSingleMesh(const std::deque<VertexPart> & vertexParts, const std::vector<uint32_t> & indices, const uint32_t triangleCount,
										const uint16_t maxIndexOffset, std::map<uint32_t, SkinningPairs> weights);
#else
		static Rendering::Mesh * createSingleMesh(const std::deque<VertexPart> & vertexParts, const std::vector<uint32_t> & indices, const uint32_t triangleCount,
										const uint16_t maxIndexOffset);
#endif

		//! Return a new description for a texture created with the information in @a textureDesc.
		NodeDescription * createTexture(const NodeDescription * textureDesc) const;


		//! Checks if material with given Id already exists and calls corresponding method for adding material to currentElement
		void addMaterial(NodeDescription * currentNode, const std::pair<std::string, std::map<std::string, int> > & materialMapDesc);

		//! Create new material and return object with material information for storing in map
		bool createNewMaterial(Material & material, const NodeDescription * materialElement, const std::map<std::string, int> & textureUnitBindings);

		//! Add already existing material from map to currentElement
		void addExistingMaterial(NodeDescription * currentNode, const Material & material);
};


using namespace Util;
using namespace Rendering;
using namespace Geometry;
using namespace std;

bool VisitorContext::enter(const std::string & tagName, const Util::MicroXML::attributes_t & attributes) {
	if ((++stats_tagCounter % 10000) == 0){
		Util::info << "Element: #" << stats_tagCounter << "  \tMeshes:" << stats_meshCounter << "\t "<<Util::Utils::getVirtualMemorySize()/(1024*1024)<<"MB\n";
	}

	if (ignorePhysics) {
		return true;
	}

	auto currentElement = new NodeDescription;
	currentElement->setString(DAE_TAG_TYPE, tagName);

	NodeDescription * parent = elementStack.empty() ? nullptr : elementStack.top();
	if(!parent){
		// store the root element (should be the COLLADA-element)
		rootElement.reset(currentElement);
	}

	elementStack.push(currentElement);

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	if (tagName == "library_animations")
	{
		channelDesc = new NodeDescription();

		channelDesc->setString(Consts::TYPE, Consts::TYPE_BEHAVIOUR);
		channelDesc->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA);
	}
#endif

	if (tagName == "library_physics_materials" || tagName == "library_physics_models" || tagName == "library_physics_scenes") {
		ignorePhysics = true;
	}
	if(elementStack.size()==2)
		Util::info << tagName<<"...\n";

	// transfer all attributes from the tag to the description and search for the id (and sid) of the node.
	std::string subtreeId=idStack.empty() ? "ROOT" : idStack.top();
	for(const auto & attr : attributes) {
		// Skip the name attribute as it is never used.
		if(attr.first == "name")
			continue;
		currentElement->setString(attr.first, attr.second);
		// Add nodes with id.
		if (attr.first == "id") {
			subtreeId = attr.second;
			bool inserted = allElementsRegistry.insert(std::make_pair(subtreeId, currentElement)).second;
			if (!inserted) {
	//			WARN("Two elements with the same \"id\" were found.");
			}
		}else if(attr.first=="sid"){
			std::string sid = "sid:"+subtreeId+':'+attr.second;
			bool inserted = allElementsRegistry.insert(std::make_pair(sid , currentElement)).second;
			if (!inserted) {
	//			WARN("Two elements with the same \"sid\" were found.");
			}
		}
	}
	idStack.push(subtreeId);

	// store last node id in description (used to find the surrounding node when searching for a local sid)
	currentElement->setString(DAE_SUBTREE_ID,subtreeId);

	const Util::StringIdentifier tagId(tagName);

	bool isMinSGDescription(false); // and not to DAE_CHILDREN

	if (tagId == DAE_TAG_TYPE_ACCESSOR) {
		assert(parent != nullptr && parent->getString(DAE_TAG_TYPE) == "technique_common");
		NodeDescription * floatArray = findElementByRef(currentElement, DAE_ATTR_SOURCE);
		floatArray->setString(DAE_ATTR_STRIDE, currentElement->getString(DAE_ATTR_STRIDE));
	} else if (tagId == DAE_TAG_TYPE_NODE) {
		isMinSGDescription=true;
		currentElement->setString(Consts::TYPE, Consts::TYPE_NODE);
		#ifdef MINSG_EXT_SKELETAL_ANIMATION
		if(currentElement->getString(Consts::ATTR_NODE_TYPE) == Consts::NODE_TYPE_SKEL_JOINT)
		{
			if(parent->getString(Consts::ATTR_NODE_ID) == Consts::NODE_TYPE_SKEL_ARMATURE)
				currentElement->setString(Consts::ATTR_SKEL_ID_TYPE_ARMATURE_MATRIX, parent->getString(Consts::ATTR_MATRIX));
			currentElement->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_JOINT);
			currentElement->setString(Consts::ATTR_SKEL_JOINTID, Util::StringUtils::toString(jointIdCount));

			jointMap[currentElement->getString(Consts::ATTR_NODE_ID)] = jointIdCount;
			jointIdCount++;
		}
		else
			currentElement->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);
		#else
			currentElement->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);
		#endif
	} else if (tagId == DAE_TAG_TYPE_INSTANCE_GEOMETRY) {
		//! \note (cl2010-10-07) Instead of this being a separate node, shouldn't this (at least when there is only one mesh)
		//! only change the parent node into an GeometryNode?
		isMinSGDescription=true;
		// ... the type of the node is set on leaving the node
	} else if (tagId == DAE_TAG_TYPE_INSTANCE_LIGHT) {
		isMinSGDescription=true;
		assert(parent != nullptr && parent->getString(Consts::TYPE) == "node");
		currentElement->setString(Consts::TYPE, Consts::TYPE_NODE);
		currentElement->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIGHT);
		const NodeDescription * light = findElementByRef(currentElement, DAE_ATTR_URL);
		{
			const NodeDescription * color = findElementByType(light, "color");
			if (color != nullptr) {

				string colorString = color->getString(DAE_DATA);
				if(Util::StringUtils::toFloats(colorString).size() == 3)
					colorString += " 1";

				currentElement->setString(Consts::ATTR_LIGHT_AMBIENT, colorString);
				currentElement->setString(Consts::ATTR_LIGHT_DIFFUSE, colorString);
				currentElement->setString(Consts::ATTR_LIGHT_SPECULAR, colorString);
			}
		}
		const NodeDescription * nd;
		if ((nd = findElementByType(light, "constant_attenuation"))) {
			currentElement->setString(Consts::ATTR_LIGHT_CONSTANT_ATTENUATION, nd->getString(DAE_DATA));
		}
		if ((nd = findElementByType(light, "linear_attenuation"))) {
			currentElement->setString(Consts::ATTR_LIGHT_LINEAR_ATTENUATION, nd->getString(DAE_DATA));
		}
		if ((nd = findElementByType(light, "quadratic_attenuation"))) {
			currentElement->setString(Consts::ATTR_LIGHT_QUADRATIC_ATTENUATION, nd->getString(DAE_DATA));
		}
		if ((nd = findElementByType(light, "falloff_angle"))) {
			currentElement->setString(Consts::ATTR_LIGHT_SPOT_CUTOFF, nd->getString(DAE_DATA));
		}
		if ((nd = findElementByType(light, "falloff_exponent"))) {
			currentElement->setString(Consts::ATTR_LIGHT_SPOT_EXPONENT, nd->getString(DAE_DATA));
		}
		if (findElementByType(light, "spot")) {
			currentElement->setString(Consts::ATTR_LIGHT_TYPE, Consts::LIGHT_TYPE_SPOT);
		} else if (findElementByType(light, "point")) {
			currentElement->setString(Consts::ATTR_LIGHT_TYPE, Consts::LIGHT_TYPE_POINT);
		} else {
			currentElement->setString(Consts::ATTR_LIGHT_TYPE, Consts::LIGHT_TYPE_DIRECTIONAL);
		}

	} else if (tagId == DAE_TAG_TYPE_INSTANCE_CAMERA) {
		isMinSGDescription=true;
		assert(parent != nullptr && parent->getString(Consts::TYPE) == "node");
		currentElement->setString(Consts::TYPE, Consts::TYPE_NODE);
		currentElement->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_CAMERA);
		const NodeDescription * camera = findElementByRef(currentElement, DAE_ATTR_URL);

		const NodeDescription * nd;
		if ( (nd = findElementByType(camera, "yfov")) ) {
			currentElement->setString(Consts::ATTR_CAM_ANGLE, nd->getString(DAE_DATA));
		}
		if ( (nd = findElementByType(camera, "znear")) ) {
			currentElement->setString(Consts::ATTR_CAM_NEAR, nd->getString(DAE_DATA));
		}
		if ( (nd = findElementByType(camera, "zfar")) ) {
			currentElement->setString(Consts::ATTR_CAM_FAR, nd->getString(DAE_DATA));
		}
	} else if (tagId == DAE_TAG_TYPE_INSTANCE_VISUAL_SCENE) {
		assert(parent != nullptr && parent->getString(DAE_TAG_TYPE) == "scene");
		assert(scene == nullptr);
		NodeDescription * sceneDesc = findElementByRef(currentElement, DAE_ATTR_URL);
		sceneDesc->setString(Consts::TYPE, Consts::TYPE_NODE);
		sceneDesc->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);

		// Add an intermediate description for the correct scene rotation because attributes of the top-level scene description are ignored.
		//! \bug (cl2010-10-07) This is buggy as the intermediateDesc is not inserted properly into the element tree and though never deleted
		//! 	Could this be solved if the top-level scene could have a rotation and the intermediateDesc is removed??
		auto intermediateDesc = new NodeDescription();
		intermediateDesc->setString(Consts::TYPE, "scene");
		auto children = new GenericAttributeList;
		children->push_back(sceneDesc);
		intermediateDesc->setValue(Consts::CHILDREN, children);

		// The COLLADA coordinate system has to be rotated into the PADrend coordinate system here.
		// Nothing has to be done for Y_UP, because in this case both coordinate systems are the same.
		if(coordinateSystem == X_UP) {
			// COLLADA coordinate system: Up axis = positive x, In axis = positive z
			// Rotate 90° around positive z axis.
			sceneDesc->setString(Consts::ATTR_SRT_UP, "-1.0 0.0 0.0");
			sceneDesc->setString(Consts::ATTR_SRT_DIR, "0.0 0.0 1.0");
		} else if(coordinateSystem == Z_UP) {
			// COLLADA coordinate system: Up axis = positive z, In axis = negative y
			// Rotate 90° around negative x axis.
			sceneDesc->setString(Consts::ATTR_SRT_UP, "0.0 0.0 -1.0");
			sceneDesc->setString(Consts::ATTR_SRT_DIR, "0.0 1.0 0.0");
		}

		scene = intermediateDesc;

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	} else if (tagId == DAE_TAG_TYPE_INSTANCE_CONTROLLER)
	{
		parent->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_SKELETALOBJECT);
		if(channelDesc != nullptr)
		{
			addToMinSGChildren(parent, channelDesc);
			NodeDescriptionList *poseDescription = dynamic_cast<NodeDescriptionList *>(channelDesc->getValue(Consts::ATTR_SKEL_SKELETALANIMATIONPOSEDESCRIPTION));
			if(poseDescription == nullptr)
				WARN("Corrupt sample data.");

			channelDesc->setString(Consts::ATTR_SKEL_SKELETALANIMATIONNAME, "std");
            channelDesc->setString(Consts::ATTR_SKEL_SKELETALSTARTANIMATION, "true");
            channelDesc->setString(Consts::ATTR_SKEL_SKELETALFROMANIMATIONS, "");
            channelDesc->setString(Consts::ATTR_SKEL_SKELETALTOANIMATIONS, "");
			channelDesc->setString(Consts::DATA_BLOCK, poseDescription->toJSON());
		}

		isMinSGDescription = true;
#endif
	}

	if (!parent) {
		return true;
	}

	if(	isMinSGDescription ){
		addToMinSGChildren(parent,currentElement);
	}else {
		addToDAEChildren(parent,currentElement);
	}
	return true;
}

bool VisitorContext::leave(const std::string & tagName) {
	assert(!elementStack.empty());
	NodeDescription * currentElement = elementStack.top();
	assert(currentElement != nullptr);
	if (ignorePhysics) {
		// Check if the current tag is the last seen physics tag.
		if (currentElement->getString(DAE_TAG_TYPE) == tagName) {
			ignorePhysics = false;
		} else {
			return true;
		}
	}
	elementStack.pop();
	if (elementStack.empty()) {
		return true;
	}
	if(!idStack.empty())
		idStack.pop();

	NodeDescription * parent = elementStack.top();
	assert(parent != nullptr);

	const Util::StringIdentifier tagId(tagName);
	if (tagId == DAE_TAG_TYPE_ROTATE) {
		assert(parent->getString(Consts::TYPE) == "node");

		// Get current matrix from parent node.
		Matrix4x4 matrix = getMatrixFromDescription(*parent);

		// Get new rotation and apply it to the current one.
		const std::vector<float> values = Util::StringUtils::toFloats(currentElement->getString(DAE_DATA));
		assert(values.size() == 4);
		matrix.rotate_deg(values[3], values[0], values[1], values[2]);

		setMatrixToDescription(*parent,matrix);

	} else if (tagId == DAE_TAG_TYPE_SCALE) {
		assert(parent->getString(Consts::TYPE) == "node");

		// Get current matrix from parent node.
		Matrix4x4 matrix = getMatrixFromDescription(*parent);
		const std::vector<float> values = Util::StringUtils::toFloats(currentElement->getString(DAE_DATA));
		assert(values.size() == 3);
		matrix.scale(values[0], values[1], values[2]);
		setMatrixToDescription(*parent,matrix);

	} else if (tagId == DAE_TAG_TYPE_TRANSLATE) {
		assert(parent->getString(Consts::TYPE) == "node");

		// Get current matrix from parent node.
		Matrix4x4 matrix = getMatrixFromDescription(*parent);
		const std::vector<float> values = Util::StringUtils::toFloats(currentElement->getString(DAE_DATA));
		assert(values.size() == 3);
		matrix.translate(values[0], values[1], values[2]);
		setMatrixToDescription(*parent,matrix);

	} else if (tagId == DAE_TAG_TYPE_MATRIX) {
		assert(parent->getString(Consts::TYPE) == "node");
//		currentElement->setString(Consts::TYPE, Consts::TYPE_ATTRIBUTE);
		parent->setString(Consts::ATTR_MATRIX, currentElement->getString(DAE_DATA));
	} else if (tagId == DAE_TAG_TYPE_UP_AXIS) {
		assert(parent->getString(DAE_TAG_TYPE) == "asset");
		std::string upString = currentElement->getString(DAE_DATA);
		if (upString == "X_UP") {
			coordinateSystem = X_UP;
		} else if (upString == "Y_UP") {
			coordinateSystem = Y_UP;
		} else if (upString == "Z_UP") {
			coordinateSystem = Z_UP;
		} else {
			WARN("Invalid up axis.");
		}
	}
#ifdef MINSG_EXT_SKELETAL_ANIMATION
	else if (tagId == DAE_TAG_TYPE_CHANNEL)
	{
		NodeDescription *source = findElementByRef(currentElement, DAE_ATTR_SOURCE);
		if(source == nullptr)
			WARN("Source in channel description missing.");

		auto sampler = new NodeDescription();
		sampler->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_ANIMATIONSAMPLE);
		NodeDescriptionList *children = dynamic_cast<NodeDescriptionList *> (source->getValue(DAE_CHILDREN));

		for(auto & elem : *children)
		{
			NodeDescription *child = dynamic_cast<NodeDescription *>(elem.get());


			if(child == nullptr)
				WARN("channel description corrupt.");

			NodeDescription *dataDesc = dynamic_cast<NodeDescription *>((dynamic_cast<NodeDescriptionList *>(findElementByRef(child, DAE_ATTR_SOURCE)->getValue(DAE_CHILDREN)))->front());
			if(dataDesc == nullptr)
				WARN("Animation data missing.");

			float_data_t * animationData = dynamic_cast<float_data_t *> (dataDesc->getValue(DAE_DATA));
			stringstream ss;
			if(animationData != nullptr)
			{
				for(uint32_t i=0; i< animationData->ref().size(); ++i)
				{
					ss << animationData->ref().at(i);
					if(i!= animationData->ref().size()-1)
						ss << " ";
				}
			}
			const std::string childSemantic = child->getString(DAE_ATTR_SEMANTIC);
			if(childSemantic == "INPUT")
			{
				sampler->setString(Consts::ATTR_SKEL_TIMELINE, ss.str());
			}
			else if(childSemantic == "OUTPUT")
			{
				sampler->setString(Consts::ATTR_SKEL_SKELETALSAMPLERDATA, ss.str());
			}
			else if(childSemantic == "INTERPOLATION")
			{
				sampler->setString(Consts::ATTR_SKEL_SKELETALINTERPOLATIONTYPE, dataDesc->getString(DAE_DATA));
			}
		}
		const std::string currentTarget = currentElement->getString(DAE_ATTR_TARGET);
		sampler->setString(Consts::ATTR_SKEL_SKELETALANIMATIONTARGET, currentTarget.substr(0, currentTarget.find("/")));
		sampler->setString(Consts::ATTR_SKEL_SKELETALANIMATIONSTARTTIME, "0");

		GenericAttributeList * childrenPose = dynamic_cast<GenericAttributeList*> (channelDesc->getValue(Consts::ATTR_SKEL_SKELETALANIMATIONPOSEDESCRIPTION));
		if (childrenPose == nullptr) {
			childrenPose = new GenericAttributeList;
			channelDesc->setValue(Consts::ATTR_SKEL_SKELETALANIMATIONPOSEDESCRIPTION, childrenPose);
		}
		childrenPose->push_back(sampler);
	}

#endif
	else if (tagId == DAE_TAG_TYPE_INSTANCE_GEOMETRY

			#ifdef MINSG_EXT_SKELETAL_ANIMATION
			|| tagId == DAE_TAG_TYPE_INSTANCE_CONTROLLER
			#endif
			) {
		assert(parent->getString(Consts::TYPE) == "node");

		// Set up material bindings mapping from symbols to materials if a "bind_material" element is
		// available.

		typedef std::map<std::string, std::pair<std::string, std::map<std::string, int> > > material_mapping_type;

		material_mapping_type materialMapping;

		const NodeDescription * bindMaterial=findElementByType(currentElement,"bind_material");
		if (bindMaterial != nullptr) {
			const NodeDescription * techniqueCommon = findElementByType(bindMaterial, "technique_common");
			if (techniqueCommon != nullptr) {
				const NodeDescriptionList * techChildren = dynamic_cast<const NodeDescriptionList *> (techniqueCommon->getValue(DAE_CHILDREN));
				for (auto & elem : *techChildren) {
					const NodeDescription * child = dynamic_cast<const NodeDescription *>(elem.get());
					if (child->getString(DAE_TAG_TYPE) == "instance_material") {
						std::deque<const NodeDescription *> bindings;
						findElementsByType(child, "bind_vertex_input", bindings);
						std::map<std::string, int> textureUnitBinding;
						for (auto & binding : bindings) {
							textureUnitBinding.insert(std::make_pair(binding->getString(DAE_ATTR_SEMANTIC), StringUtils::toNumber<int>(binding->getString(DAE_ATTR_INPUT_SET))));
						}
						//make sure indices are correct (without leaps, >= 0 and <= 7) [==> TEX0 ... TEX7]
						std::list<int> sortedTexUnitIndices;
						for(auto & textureUnitBinding_tuIt : textureUnitBinding){
							sortedTexUnitIndices.push_back((textureUnitBinding_tuIt).second);
						}
						sortedTexUnitIndices.sort();
						for(auto & textureUnitBinding_tuIt : textureUnitBinding){
							int indexCounter = 0;
							//determine index of current texUnitIndex in sorted list
							for(auto & sortedTexUnitIndice : sortedTexUnitIndices){
								if((textureUnitBinding_tuIt).second == (sortedTexUnitIndice))
									break;
								indexCounter++;
							}
							(textureUnitBinding_tuIt).second = indexCounter;
						}
						materialMapping.insert(std::make_pair(child->getString(DAE_ATTR_SYMBOL), std::make_pair(child->getString(DAE_ATTR_TARGET).substr(1), textureUnitBinding)));
					}
				}
			}
		}

#ifdef MINSG_EXT_SKELETAL_ANIMATION
		// Get the meshes description referenced by "url" with their material symbols.

		// first check if mesh already created
		bool found = false;
		bool isAnimation = false;
		std::string meshId = currentElement->getString(DAE_ATTR_URL).substr(1);
		mesh_list_t meshList;
		auto meshIt = meshRegistry.find(
																				currentElement->getString(DAE_ATTR_URL).substr(1)); // strip the '#' from the id

		NodeDescription *nd = findElementByRef(currentElement, DAE_ATTR_URL);
		const NodeDescription *skinDesc = findElementByType(nd, "skin");
		if(meshIt == meshRegistry.end() && skinDesc != nullptr)
		{
			meshIt = meshRegistry.find(skinDesc->getString(DAE_ATTR_SOURCE).substr(1));
			isAnimation = true;
		}

		if (meshIt != meshRegistry.end())
		{
			found = true;
			meshList = meshIt->second;
		}

		if(!found)
		{
			auto meshItDesc = meshDescription.find(meshId);

			if(meshItDesc == meshDescription.end())
			{
				nd = findElementByRef(currentElement, DAE_ATTR_URL);
				meshItDesc = meshDescription.find(findElementByType(nd, "skin")->getString(DAE_ATTR_SOURCE).substr(1));
				isAnimation = true;
			}

			if (meshItDesc == meshDescription.end()) {
				WARN(std::string("Could not find mesh for id \"") + currentElement->getString(DAE_ATTR_URL) + "\".");
				return true;
			}

			// if meshed used then create.
			NodeDescription *meshDesc = meshItDesc->second;
			if(isAnimation)
				addToDAEChildren(meshDesc, nd);

			// append joints and weights to mesh
			if (!createMeshes(meshDesc, meshList))
				WARN("Could not create meshes:"+meshItDesc->first);
			else
			{
				meshRegistry[meshItDesc->first] = meshList;

				meshDescription.erase(meshItDesc->first);
			}
		}
#else
		std::map<std::string, mesh_list_t>::iterator meshIt = meshRegistry.find(
					currentElement->getString(DAE_ATTR_URL).substr(1)); // strip the '#' from the id

		if (meshIt == meshRegistry.end()) {
			WARN(std::string("Could not find mesh for id \"") + currentElement->getString(DAE_ATTR_URL) + "\".");
			return true;
		}

		mesh_list_t & meshList = meshIt->second;
#endif
		if (meshList.size() == 1) {
			currentElement->setString(Consts::TYPE, Consts::TYPE_NODE);
			currentElement->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_GEOMETRY);

			auto dataDesc=new NodeDescription;
			dataDesc->setString(Consts::TYPE, Consts::TYPE_DATA);
			dataDesc->setString(Consts::ATTR_DATA_TYPE, "mesh");

			dataDesc->setValue(Consts::ATTR_MESH_DATA, new Serialization::MeshWrapper_t(meshList.front().first.get()));
			addToMinSGChildren(currentElement,dataDesc);

			// Translate symbol to material instance.
			const std::string & materialSymbol = meshList.front().second;
			material_mapping_type::const_iterator matMapIt = materialMapping.find(materialSymbol);
			if (matMapIt != materialMapping.end()) {
				addMaterial(currentElement, matMapIt->second);
			} else {
				// it may be, that the material directly references a material if no "bind_material"-element is available
				std::map<std::string, int> emptyBindingMap;
				addMaterial(currentElement, std::make_pair(materialSymbol, emptyBindingMap));
			}


		} else {
			// Create a ListNode which contains multiple GeometryNodes.
			currentElement->setString(Consts::TYPE, Consts::TYPE_NODE);
			currentElement->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIST);

			for (auto & elem : meshList) {
				auto geoDesc = new NodeDescription;
				geoDesc->setString(Consts::TYPE, Consts::TYPE_NODE);
				geoDesc->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_GEOMETRY);

				auto dataDesc=new NodeDescription;
				dataDesc->setString(Consts::ATTR_DATA_TYPE, "mesh");
				dataDesc->setString(Consts::TYPE, Consts::TYPE_DATA);
				dataDesc->setValue(Consts::ATTR_MESH_DATA, new Serialization::MeshWrapper_t(elem.first.get()));
				addToMinSGChildren(geoDesc,dataDesc);

				// Translate symbol to material instance.
				const std::string & materialSymbol = elem.second;
				material_mapping_type::const_iterator matMapIt = materialMapping.find(materialSymbol);
				if (matMapIt != materialMapping.end()) {
					addMaterial(geoDesc, matMapIt->second);
				} else {
					// it may be, that the material directly references a material if no "bind_material"-element is available
					std::map<std::string, int> emptyBindingMap;
					addMaterial(currentElement, std::make_pair(materialSymbol, emptyBindingMap));
				}
				addToMinSGChildren(currentElement,geoDesc);
			}
		}

#ifdef MINSG_EXT_SKELETAL_ANIMATION
		NodeDescriptionList * children = dynamic_cast<NodeDescriptionList *> (currentElement->getValue(DAE_CHILDREN));
		if(children != nullptr)
		{
			bool isAnimation2 = false;
			for (NodeDescriptionList::const_iterator it = children->begin(); it != children->end(); ++it)
			{
				NodeDescription * child = dynamic_cast<NodeDescription *>(it->get());
				if(child->getString(DAE_TAG_TYPE) == "skeleton")
				{
					NodeDescription *skeleton = findElementByRef(child, DAE_DATA);

					// inv bind matrix for joints
					const NodeDescription *skinningDesc = findElementByRef(currentElement, DAE_ATTR_URL);
					const NodeDescription *skinningSemantic = dynamic_cast<const NodeDescription *> (findElementByType(skinningDesc, "joints"));

					NodeDescriptionList *jointBindSemantics = dynamic_cast<NodeDescriptionList *> (skinningSemantic->getValue(DAE_CHILDREN));
					NodeDescription *invJointNameArray = nullptr;
					auto invBindMatrix = new NodeDescription();
					for(auto & jointBindSemantic : *jointBindSemantics)
					{
						NodeDescription *semanticChild = dynamic_cast<NodeDescription *>(jointBindSemantic.get());
						if(semanticChild->getString(DAE_ATTR_SEMANTIC) == "JOINT")
						{
							invJointNameArray = findElementByRef(semanticChild, DAE_ATTR_SOURCE);
							if(invJointNameArray != nullptr)
								invJointNameArray = dynamic_cast<NodeDescription * > ((dynamic_cast<NodeDescriptionList *> (invJointNameArray->getValue(DAE_CHILDREN)))->front());
							isAnimation2 = true;
						}
						else if(semanticChild->getString(DAE_ATTR_SEMANTIC) == "INV_BIND_MATRIX")
						{
							NodeDescription *invBindMatrixTmp = findElementByRef(semanticChild, DAE_ATTR_SOURCE);
							if(invBindMatrixTmp != nullptr)
								invBindMatrix->setValue(DAE_ATTR_FLOAT_ARRAY, (dynamic_cast<NodeDescription * > ((dynamic_cast<NodeDescriptionList *> (invBindMatrixTmp->getValue(DAE_CHILDREN)))->front()))->getValue(DAE_DATA));
							isAnimation2 = true;
							invBindMatrix->setString(Consts::TYPE, "INV_BIND_MATRIX");
							invBindMatrix->setValue(DAE_ATTR_NAME_ARRAY, invJointNameArray->getValue(DAE_DATA));
							addToMinSGChildren(parent, dynamic_cast<NodeDescription *>(invBindMatrix->clone()));
						}
					}

					if(isAnimation2)
					{
						NodeDescription *bindMatrix = dynamic_cast<NodeDescription *> (findElementByType(findElementByRef(currentElement, DAE_ATTR_URL), "bind_shape_matrix")->clone());
						bindMatrix->setValue(DAE_ATTR_BIND_MATRIX, bindMatrix->getValue(DAE_DATA));
						bindMatrix->setString(Consts::TYPE, Consts::TYPE_STATE);
						bindMatrix->setString(Consts::ATTR_DATA_TYPE, Consts::STATE_TYPE_SKEL_SKELETALHARDWARERENDERERSTATE);

						//export inv_bind_matrix data
						istringstream iss(invBindMatrix->getString(DAE_ATTR_NAME_ARRAY));
						   vector<string> tokens;
						copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(tokens));
						float_data_t* floatData = dynamic_cast<float_data_t *>(invBindMatrix->getValue(DAE_ATTR_FLOAT_ARRAY));

						map<string, string> invMatrixData;
						uint32_t index = 0;
						for(uint32_t i=0; i<floatData->ref().size()/16; ++i)
						{
							ostringstream tmp;
							for(uint32_t j=0; j<16; ++j)
								tmp << floatData->ref()[index+j] << " ";
							index += 16;
							invMatrixData[tokens[i]] = tmp.str();
						}


						deque<NodeDescription *> nodeList;
						nodeList.push_back(skeleton);
						while(!nodeList.empty())
						{
							if(nodeList.front()->contains(Consts::CHILDREN))
							{
								NodeDescriptionList *list = dynamic_cast<NodeDescriptionList *>(nodeList.front()->getValue(Consts::CHILDREN));
								for(auto & elem : *list)
								{
									NodeDescription *item = dynamic_cast<NodeDescription *>(elem.get());
									nodeList.push_back(item);
								}
							}
							nodeList.front()->setString(Consts::ATTR_SKEL_INVERSEBINDMATRIX, invMatrixData[nodeList.front()->getString(DAE_ATTR_SID)]);

							//if(nodeList.front()->contains(Consts::ATTR_SKEL_ID_TYPE_ARMATURE_MATRIX))
							//    parent->setString(Consts::ATTR_MATRIX, nodeList.front()->getString(Consts::ATTR_SKEL_ID_TYPE_ARMATURE_MATRIX));

							nodeList.pop_front();
						}

						auto armature = new NodeDescription();
						armature->setString(Consts::TYPE, Consts::TYPE_NODE);
						armature->setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_SKEL_ARMATURE);
						addToMinSGChildren(armature, dynamic_cast<NodeDescription *> (skeleton->clone()));
						skeleton->clear();

						addToMinSGChildren(parent, dynamic_cast<NodeDescription *>(bindMatrix->clone()));
						
						// Armature nodes have to be at the beginnig of the childrenlist, so the animation data can
						// create links to them in the second processing step.
						
						//addToMinSGChildren(parent, armature);
						{
							NodeDescriptionList * pChildren = dynamic_cast<NodeDescriptionList *> (parent->getValue(Consts::CHILDREN));
							if (pChildren == nullptr) {
								children = new NodeDescriptionList;
								parent->setValue(Consts::CHILDREN, pChildren);
							}
							pChildren->push_front(armature);
						}
					}
				}
			}
		}
#endif

		// remove collada elements to save memory (they should no longer be needed)
		//! \note this is may be dangerous as the elements are not removed from the allElementsRegistry
		currentElement->unsetValue(DAE_CHILDREN);
		currentElement->unsetValue(DAE_TAG_TYPE);
		currentElement->unsetValue(DAE_SUBTREE_ID);

//		std::cout<<parent->toString()<<"\n";
//		std::cout<<currentElement->toString()<<"\n";
//		std::cout << "------\n\n";
	} else if(tagId == DAE_TAG_TYPE_GEOMETRY){
		const std::string meshId = currentElement->getString(DAE_ATTR_ID);

#ifdef MINSG_EXT_SKELETAL_ANIMATION
		meshDescription[meshId] = dynamic_cast<NodeDescription *>(currentElement->clone());
#else
		mesh_list_t  meshList;

		if (createMeshes(currentElement, meshList)) {
			meshRegistry[meshId] = meshList;
		}else{
			WARN("Could not create meshes:"+meshId);
		}

		// remove collada elements to save memory (they should no longer be needed)
		//! \note this is may be dangerous as the elements are not removed from the allElementsRegistry
		currentElement->unsetValue(DAE_CHILDREN);
		currentElement->unsetValue(DAE_TAG_TYPE);
#endif
	}

	return true;
}

bool VisitorContext::data(const std::string & tagName, const std::string & _data) {
	if (elementStack.empty())
		return true;

	if (tagName == "float_array") {
		// convert string to list of numbers
		auto floatData = new float_data_t;
		Util::StringUtils::extractFloats(_data, floatData->ref());
		elementStack.top()->setValue(DAE_DATA, floatData);
	} else {
		elementStack.top()->setValue(DAE_DATA, Util::GenericAttribute::createString(_data));
	}
	return true;
}

void VisitorContext::addMaterial(NodeDescription * currentNode, const std::pair<std::string, std::map<std::string, int> > & materialMapDesc) {
	const std::map<std::string, Material>::iterator iter = materialRegistry.find(materialMapDesc.first);
	if (iter != materialRegistry.end()) { // add existing material
		const Material & material = iter->second;
		if (material.stateDescription != nullptr) {
			auto d = new NodeDescription;
			d->setString(Consts::TYPE, Consts::TYPE_STATE);
			d->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_REFERENCE);
			d->setString(Consts::ATTR_REFERENCED_STATE_ID, material.stateDescription->getString(Consts::ATTR_STATE_ID) );
			addToMinSGChildren(currentNode, d);
		}
	}
	else { // create and add new material
		const NodeDescription * materialElement = findElementById(materialMapDesc.first);
		if (materialElement != nullptr) {
			Material newMaterial;
			bool result = createNewMaterial(newMaterial, materialElement, materialMapDesc.second);
			if (result) {
				materialRegistry.insert(make_pair(materialMapDesc.first, newMaterial));
				addToMinSGChildren(currentNode, newMaterial.stateDescription);
			}
		}else{
			WARN("material not found:"+materialMapDesc.first);
		}
	}
}

bool VisitorContext::createNewMaterial(Material & material, const NodeDescription * materialElement, const std::map<std::string, int> & textureUnitBindings) {
	const NodeDescription * effectInstance = findElementByType(materialElement, "instance_effect");
	if (effectInstance == nullptr) {
		WARN("Effect instance not found.");
		return false;
	}

	const NodeDescription * effect = findElementByRef(effectInstance, DAE_ATTR_URL);
	if (effect == nullptr) {
		WARN("Effect not found.");
		return false;
	}


	std::vector<NodeDescription *> stateDescriptions;

	// MATERIAL
	auto materialDesc = new NodeDescription;
	stateDescriptions.push_back(materialDesc);

	materialDesc->setString(Consts::TYPE, Consts::TYPE_STATE);
	materialDesc->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_MATERIAL);

	const NodeDescription * nd;
	if ( (nd = findElementByType(effect, "ambient")) ) {
		const NodeDescription * colorInstance = findElementByType(nd, "color");
		if (colorInstance != nullptr) {
			materialDesc->setString(Consts::ATTR_MATERIAL_AMBIENT, colorInstance->getString(DAE_DATA));
		}
	}
	if ( (nd = findElementByType(effect, "diffuse")) ) {
		const NodeDescription * colorInstance = findElementByType(nd, "color");
		if (colorInstance != nullptr) {
			materialDesc->setString(Consts::ATTR_MATERIAL_DIFFUSE, colorInstance->getString(DAE_DATA));
		}
		std::deque<const NodeDescription *> textureInstances;
		findElementsByType(nd, "texture", textureInstances);
		const size_t textureCount = textureInstances.size();
		for(size_t i = 0; i < textureCount; ++i) {
			const std::map<std::string, int>::const_iterator iter = textureUnitBindings.find(textureInstances[i]->getString(DAE_ATTR_TEXCOORD));
			int texUnit;
			if (iter != textureUnitBindings.end()) {
				texUnit = iter->second;
				if(texUnit > 7){
					WARN("Texture Unit > TEX7 is not supported!");
					texUnit = 0;
				}
			}
			else texUnit = 0;

			NodeDescription * texture = createTexture(textureInstances[i]);
			texture->setString(Consts::ATTR_TEXTURE_UNIT, StringUtils::toString(texUnit));
			stateDescriptions.push_back(texture);
		}
	}
	if ( (nd = findElementByType(effect, "specular")) ) {
		const NodeDescription * colorInstance = findElementByType(nd, "color");
		if (colorInstance != nullptr) {
			materialDesc->setString(Consts::ATTR_MATERIAL_SPECULAR, colorInstance->getString(DAE_DATA));
		}
	}
	if ( (nd = findElementByType(effect, "shininess")) ) {
		const NodeDescription * floatInstance = findElementByType(nd, "float");
		if (floatInstance != nullptr) {
			materialDesc->setString(Consts::ATTR_MATERIAL_SHININESS, floatInstance->getString(DAE_DATA));
		}
	}

	// BLENDING
	nd = findElementByType(effect, "transparency");
	if (nd != nullptr) {
		const NodeDescription * floatInstance = findElementByType(nd, "float");
		if (floatInstance != nullptr) {
			float value = Util::StringUtils::toNumber<float>(floatInstance->getString(DAE_DATA));
			if (flag_invertTransparency) {
				value = 1.0f - value;
			}
			if (value < 1.0f) {
				auto transparency = new NodeDescription;
				transparency->setString(Consts::TYPE, Consts::TYPE_STATE);
				transparency->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_BLENDING);
				transparency->setString(Consts::ATTR_BLEND_CONST_ALPHA, Util::StringUtils::toString(value));

				stateDescriptions.push_back(transparency);
			}
		}
	}

	// finalize
	const std::string effectId = effect->getString(DAE_ATTR_ID);

	if(stateDescriptions.size()==1){ // single state
		NodeDescription * d = stateDescriptions.front();
		d->setString(Consts::ATTR_STATE_ID, "M:"+effectId ); // should be a Material

		material.stateDescription = d;
	}else{ // group of states
		auto group = new NodeDescription;

		group->setString(Consts::TYPE, Consts::TYPE_STATE);
		group->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_GROUP );
		group->setString(Consts::ATTR_STATE_ID, "G:"+effectId );
		for(auto & stateDescription : stateDescriptions){
			addToMinSGChildren(group, stateDescription);
		}
		material.stateDescription = group;
	}
	return true;
}

//! (internal)
NodeDescription * VisitorContext::findElementByRef(const NodeDescription * sourceElement, const Util::StringIdentifier & refAttrName) const {
	std::string id = sourceElement->getString(refAttrName);
	if (id.empty()) {
		return nullptr;
	}
	if (id.at(0) != '#') {
		// Search for local sid.
		std::string sid = "sid:" + sourceElement->getString(DAE_SUBTREE_ID) + ':' + id;
		NodeDescription * nodeDesc = findElementById(sid, false);
		if (nodeDesc != nullptr)
			return nodeDesc; //maybe id is global id without '#'!
	} else {
		// Search for global id.
		id = id.substr(1);
	}
	return findElementById(id);
}

//! (internal)
NodeDescription * VisitorContext::findElementById(const std::string & id, bool warn) const {
	const std::map<std::string, NodeDescription *>::const_iterator it = allElementsRegistry.find(id);
	if (it == allElementsRegistry.end()) {
		if (warn) WARN("Referenced id \"" + id + "\" was not found.");
		return nullptr;
	}
	return it->second;
}

//! (static,internal) Used by createMeshes(...)
#ifdef MINSG_EXT_SKELETAL_ANIMATION
Rendering::Mesh * VisitorContext::createSingleMesh(const std::deque<VertexPart> & vertexParts, const std::vector<uint32_t> & indices, const uint32_t triangleCount,
										const uint16_t maxIndexOffset, std::map<uint32_t, SkinningPairs> weights)
#else
Rendering::Mesh * VisitorContext::createSingleMesh(const std::deque<VertexPart> & vertexParts, const std::vector<uint32_t> & indices, const uint32_t triangleCount,
										const uint16_t maxIndexOffset)
#endif
{
	// Create mesh description and data.
	VertexDescription vd;
	// Order for our vertex data in Mesh
	std::deque<VertexPart> orderedParts;

	// fill orderedParts and create corresponding vertexDescription
	for (auto & vertexPart : vertexParts) {
		if (vertexPart.type == VertexPart::POSITION) {
			const VertexAttribute & attr = vd.getAttribute(VertexAttributeIds::POSITION);
			if ( !attr.empty() ) {
				WARN("POSITION used multiple times.");
				return nullptr;
			}
			vd.appendPosition3D();
			// make sure positions stands at the beginning
			orderedParts.push_front( vertexPart );
		} else if (vertexPart.type == VertexPart::NORMAL) {
			const VertexAttribute & attr = vd.getAttribute(VertexAttributeIds::NORMAL);
			if ( !attr.empty() ) {
				WARN("NORMAL used multiple times.");
				return nullptr;
			}
			vd.appendNormalFloat();
			orderedParts.push_back( vertexPart );
		} else if (vertexPart.type == VertexPart::COLOR) {
			const VertexAttribute & attr = vd.getAttribute(VertexAttributeIds::COLOR);
			if ( !attr.empty() ) {
				WARN("COLOR used multiple times.");
				return nullptr;
			}
			vd.appendFloatAttribute(VertexAttributeIds::COLOR, vertexPart.stride);
			orderedParts.push_back( vertexPart );

		} else if (vertexPart.type == VertexPart::TEXCOORD) {
			bool inserted = false;
			for(uint_fast8_t i = 0; i < 8; ++i) {
				const Util::StringIdentifier texCoordId = VertexAttributeIds::getTextureCoordinateIdentifier(i);
				const VertexAttribute & attr = vd.getAttribute(texCoordId);
				if(attr.empty()){
					vd.appendFloatAttribute(texCoordId, 2);
					orderedParts.push_back(vertexPart);
					inserted = true;
					break;
				}
			}
			if (!inserted) {
				WARN("Limit of TEXCOORD definitions exceeded (skipped).");
			}
		} else {
			WARN("Unknown type (skipped).");
		}
	}

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	const Util::StringIdentifier	ATTR_ID_WEIGHTS1("sg_Weights1");
	const Util::StringIdentifier	ATTR_ID_WEIGHTS2("sg_Weights2");
	const Util::StringIdentifier	ATTR_ID_WEIGHTS3("sg_Weights3");
	const Util::StringIdentifier	ATTR_ID_WEIGHTS4("sg_Weights4");

	const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX1("sg_WeightsIndex1");
	const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX2("sg_WeightsIndex2");
	const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX3("sg_WeightsIndex3");
	const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX4("sg_WeightsIndex4");

	const Util::StringIdentifier	ATTR_ID_WEIGHTSCOUNT("sg_WeightsCount");

	if(!weights.empty())
	{
		if ( ! vd.getAttribute(ATTR_ID_WEIGHTS1).empty() ) {
			WARN("WEIGHTS used multiple times.");
			return nullptr;
		}

		vd.appendFloatAttribute(ATTR_ID_WEIGHTS1,4);
		vd.appendFloatAttribute(ATTR_ID_WEIGHTS2,4);
		vd.appendFloatAttribute(ATTR_ID_WEIGHTS3,4);
		vd.appendFloatAttribute(ATTR_ID_WEIGHTS4,4);

		if ( ! vd.getAttribute(ATTR_ID_WEIGHTSINDEX1).empty() ) {
			WARN("WEIGHTS used multiple times.");
			return nullptr;
		}
		vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX1,4);
		vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX2,4);
		vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX3,4);
		vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX4,4);

		if ( ! vd.getAttribute(ATTR_ID_WEIGHTSCOUNT).empty() ) {
			WARN("WEIGHTS used multiple times.");
			return nullptr;
		}
		vd.appendFloatAttribute(ATTR_ID_WEIGHTSCOUNT,1);
	}
	const VertexAttribute &  weightAttr1 = vd.getAttribute(ATTR_ID_WEIGHTS1);
	const VertexAttribute &  weightAttr2 = vd.getAttribute(ATTR_ID_WEIGHTS2);
	const VertexAttribute &  weightAttr3 = vd.getAttribute(ATTR_ID_WEIGHTS3);
	const VertexAttribute &  weightAttr4 = vd.getAttribute(ATTR_ID_WEIGHTS4);

	const VertexAttribute &  weightAttrIndex1 = vd.getAttribute(ATTR_ID_WEIGHTSINDEX1);
	const VertexAttribute &  weightAttrIndex2 = vd.getAttribute(ATTR_ID_WEIGHTSINDEX2);
	const VertexAttribute &  weightAttrIndex3 = vd.getAttribute(ATTR_ID_WEIGHTSINDEX3);
	const VertexAttribute &  weightAttrIndex4 = vd.getAttribute(ATTR_ID_WEIGHTSINDEX4);

	const VertexAttribute &  weightAttrCount = vd.getAttribute(ATTR_ID_WEIGHTSCOUNT);
#endif

	// create mesh
	auto mesh = new Mesh();
	MeshIndexData & iData = mesh->openIndexData();
	iData.allocate(3 * triangleCount);

	MeshVertexData & vData = mesh->openVertexData();
	vData.allocate(3 * triangleCount, vd); // to simplify the creation process, each index points first poins to its own vertex.


	uint32_t * indexData = iData.data();
	uint32_t indexPos = 0;
#ifdef MINSG_EXT_SKELETAL_ANIMATION
	uint32_t vertexPos = 0;
	uint32_t j=0;
#endif
	for (uint32_t i = 0; i < 3 * triangleCount; ++i) {
		float * vertexData = reinterpret_cast<float *> (vData[i]);
		for (std::deque<VertexPart>::const_iterator partIt=orderedParts.begin(); partIt!=orderedParts.end();++partIt){
			const VertexPart & part = *partIt;
			uint32_t pos = indices[indexPos + part.indexOffset] * part.stride;
#ifdef MINSG_EXT_SKELETAL_ANIMATION
			if((j % orderedParts.size()) == 0)
				vertexPos = indices[indexPos + part.indexOffset];
#endif
			for (uint_fast8_t v = 0; v < part.stride; ++v) {
				*vertexData = part.data[pos];
				++vertexData;
				++pos;
			}
#ifdef MINSG_EXT_SKELETAL_ANIMATION
			++j;
#endif
		}
		indexPos += (maxIndexOffset + 1);
#ifdef MINSG_EXT_SKELETAL_ANIMATION
		if(!weights.empty())
		{
			vertexData = reinterpret_cast<float *> (vData[i]+weightAttr1.getOffset());
			for(uint32_t k=0; k<weights[vertexPos].vcount && k<4; ++k)
				vertexData[k] = weights[vertexPos].weight[k];

			vertexData = reinterpret_cast<float *>(vData[i]+weightAttrIndex1.getOffset());
			for(uint32_t k=0; k<weights[vertexPos].vcount && k<4; ++k)
				vertexData[k] = static_cast<float>(weights[vertexPos].jointId[k]);

			if(weights[vertexPos].vcount > 4)
			{
				vertexData = reinterpret_cast<float *> (vData[i]+weightAttr2.getOffset());
				for(uint32_t k=4; k<weights[vertexPos].vcount && k<8; ++k)
					vertexData[k-4] = weights[vertexPos].weight[k];

				vertexData = reinterpret_cast<float *>(vData[i]+weightAttrIndex2.getOffset());
				for(uint32_t k=4; k<weights[vertexPos].vcount && k<8; ++k)
					vertexData[k-4] = static_cast<float>(weights[vertexPos].jointId[k]);
			}

			if(weights[vertexPos].vcount > 8)
			{
				vertexData = reinterpret_cast<float *> (vData[i]+weightAttr3.getOffset());
				for(uint32_t k=8; k<weights[vertexPos].vcount && k<12; ++k)
					vertexData[k-8] = weights[vertexPos].weight[k];

				vertexData = reinterpret_cast<float *>(vData[i]+weightAttrIndex3.getOffset());
				for(uint32_t k=8; k<weights[vertexPos].vcount && k<12; ++k)
					vertexData[k-8] = static_cast<float>(weights[vertexPos].jointId[k]);
			}

			if(weights[vertexPos].vcount > 12)
			{
				vertexData = reinterpret_cast<float *> (vData[i]+weightAttr4.getOffset());
				for(uint32_t k=12; k<weights[vertexPos].vcount && k<16; ++k)
					vertexData[k-12] = weights[vertexPos].weight[k];

				vertexData = reinterpret_cast<float *>(vData[i]+weightAttrIndex4.getOffset());
				for(uint32_t k=12; k<weights[vertexPos].vcount && k<16; ++k)
					vertexData[k-12] = static_cast<float>(weights[vertexPos].jointId[k]);
			}

			vertexData = reinterpret_cast<float *>(vData[i]+weightAttrCount.getOffset());
			vertexData[0] = weights[vertexPos].vcount;

		}
#endif

		*indexData = i;
		++indexData;
	}

	iData.updateIndexRange();
	vData.updateBoundingBox();

	// remove redundant data
	MeshUtils::eliminateDuplicateVertices(mesh);

	return mesh;
}

/*!	*/
bool VisitorContext::createMeshes(const NodeDescription * geoDesc, mesh_list_t & meshList) {
	if (geoDesc == nullptr) {
		return false;
	}

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	// The first and only child should be a mesh.

	bool isAnimation = false;
	std::map<uint32_t, SkinningPairs> weights;
	NodeDescriptionList * geoChildren = dynamic_cast<NodeDescriptionList *> (geoDesc->getValue(DAE_CHILDREN));
	if (geoChildren == nullptr) {
		WARN("Invalid geometry description.");
		return false;
	}

	NodeDescription *meshDesc = nullptr;
	NodeDescription *skinDesc = nullptr;
	if(!geoChildren->empty())
	{
		meshDesc = dynamic_cast<NodeDescription *> (geoChildren->front());

		for (auto & elem : *geoChildren)
		{
			if(dynamic_cast<NodeDescription *>(elem.get())->getString(DAE_TAG_TYPE) == "controller")
			{
				skinDesc = dynamic_cast<NodeDescription *> (dynamic_cast<NodeDescriptionList *> ((dynamic_cast<NodeDescription *>(elem.get()))->getValue(DAE_CHILDREN))->front());
				isAnimation = true;
			}
		}
	}

	if(meshDesc == nullptr)
		WARN("No gemetry description.");

	// create list with all weights for animation
	if(isAnimation)
	{
		const NodeDescription *weightDesc = findElementByType(skinDesc, Consts::NODE_TYPE_SKEL_VERTEX_WEIGHT);

		if(weightDesc == nullptr)
			WARN("No weights for skinning.");

		uint32_t offsetJoint=0;
		uint32_t offsetWeight=0;

		uint32_t countWeights = Util::StringUtils::toNumber<uint32_t> (weightDesc->getString(DAE_ATTR_COUNT));
		if(countWeights < 1)
			WARN("Incompatible number of weight numbers ( count < 1 ).");

		// map <jointId, vector with skinning information>
		std::vector<float> weightsVector;
		std::vector<string> nameVector;
		std::deque<unsigned long> vcountVector;
		const NodeDescriptionList *weightChildren = dynamic_cast<const NodeDescriptionList *> (weightDesc->getValue(DAE_CHILDREN));
		for(auto & elem : *weightChildren)
		{
			const NodeDescription *item = dynamic_cast<const NodeDescription *>(elem.get());
			if(item == nullptr)
				continue;
            
			std::string itemSemantic = item->getString(DAE_ATTR_SEMANTIC);
            if(itemSemantic.empty())
                itemSemantic = item->getString(DAE_TAG_TYPE);
			if(itemSemantic == "JOINT")
			{
				offsetJoint = Util::StringUtils::toNumber<uint32_t> (item->getString(DAE_ATTR_OFFSET));

				const NodeDescription * vSource = findElementByRef(item, DAE_ATTR_SOURCE);
				const NodeDescriptionList *sourceChildren = dynamic_cast<const NodeDescriptionList *> (vSource->getValue(DAE_CHILDREN));
				std::map<string, uint32_t>::iterator jointIt;
				for(auto & sourceChildren_it2 : *sourceChildren)
				{
					NodeDescription *nameArray = dynamic_cast<NodeDescription *>(sourceChildren_it2.get());
					if(nameArray->getString(DAE_TAG_TYPE) == "Name_array")
					{
						std::string ids = nameArray->getString(DAE_DATA);

						istringstream iss(ids);
						copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(nameVector));

						for(auto & nameVector_i : nameVector)
						{
							jointIt = jointMap.find(nameVector_i);
							if(jointIt == jointMap.end())
								WARN("Wrong Joint IDs.");
						}

						break;
					}
				}
			}
			else if(itemSemantic == "WEIGHT")
			{
				offsetWeight = Util::StringUtils::toNumber<uint32_t> (item->getString(DAE_ATTR_OFFSET));

				const NodeDescription *weightsArrayDesc = dynamic_cast<const NodeDescription *> ((dynamic_cast<const NodeDescriptionList *> (findElementByRef(item, DAE_ATTR_SOURCE)->getValue(DAE_CHILDREN)))->front());
				float_data_t * weightsValues = dynamic_cast<float_data_t *>(weightsArrayDesc->getValue(DAE_DATA));
				if (weightsValues == nullptr) {
					WARN("Wrong data format.");
					return false;
				}
				std::copy(weightsValues->ref().begin(), weightsValues->ref().end(), std::back_inserter(weightsVector));
			}
			else if(item->getString(DAE_TAG_TYPE) == "vcount")
			{
				Util::StringUtils::extractUnsignedLongs(item->getString(DAE_DATA), vcountVector);

				for(uint32_t i=0; i<countWeights; ++i)
				{
					SkinningPairs pair;
					pair.vcount = vcountVector[i];
					weights[i] = pair;
				}
			}
			else if(itemSemantic == "v")
			{
				std::deque<unsigned long> vIndexVector;
				Util::StringUtils::extractUnsignedLongs(item->getString(DAE_DATA), vIndexVector);

				int j=0;
				unsigned int k=0;
				for(unsigned int i=0; i<vIndexVector.size();)
				{
					for(k=0; k<vcountVector[j]; ++k)
					{
						weights[j].jointId.push_back(vIndexVector[i+offsetJoint]);
						weights[j].weight.push_back(weightsVector[vIndexVector[i+offsetWeight]]);
						i+=2;
					}
					j++;
				}
			}
		}
	}
#else
	//
	const NodeDescriptionList * geoChildren = dynamic_cast<const NodeDescriptionList *> (geoDesc->getValue(DAE_CHILDREN));
	if (geoChildren == nullptr) {
		WARN("Invalid geometry description.");
		return false;
	}
	const NodeDescription * meshDesc = nullptr;

	// search for mesh-child (<mesh>-child)
	for (NodeDescriptionList::const_iterator it = geoChildren->begin(); it != geoChildren->end(); ++it)	{
		const NodeDescription * childDesc = dynamic_cast<NodeDescription *>(it->get());
			if(childDesc!=nullptr && childDesc->getString(DAE_TAG_TYPE)=="mesh"){
				meshDesc = childDesc;
				break;
			}
	}
	if( meshDesc==nullptr){
		WARN("Invalid geometry description: No <mesh>-tag found.");
		return false;
	}
#endif

	// Find vertices and primitive elements, ignore the rest.
	// TODO: Remove this first step and process vertices and primitives directly.
	const NodeDescription * vertices = nullptr;
	std::deque<const NodeDescription *> primitives;
	const NodeDescriptionList * meshChildren = dynamic_cast<const NodeDescriptionList *> (meshDesc->getValue(DAE_CHILDREN));
	for (auto & elem : *meshChildren) {
		const NodeDescription * tempDesc = dynamic_cast<const NodeDescription *>(elem.get());
		if (tempDesc == nullptr) {
			continue;
		}
		const std::string type = tempDesc->getString(DAE_TAG_TYPE);
		if(type == "vertices") {
			if(vertices != nullptr) {
				WARN("Multiple vertices descriptions found.");
				return false;
			}
			vertices = tempDesc;
		}
		if (type == "lines"
				|| type == "linestrips"
				|| type == "polygons"
				|| type == "polylist"
				|| type == "triangles"
				|| type == "trifans"
				|| type == "tristrips") {
			primitives.push_back(tempDesc);
		}
	}
	if(vertices == nullptr) {
		WARN("No vertices found.");
		return false;
	}
	if (primitives.empty()) {
		WARN("No primitives found.");
		return false;
	}

	for (auto & primitive : primitives) {
		const std::string type = (primitive)->getString(DAE_TAG_TYPE);

		if (type == "polylist") {
			   std::deque<VertexPart> vertexParts;
			std::vector<uint32_t> indices;
			uint32_t triangleCount = Util::StringUtils::toNumber<uint32_t>((primitive)->getString(DAE_ATTR_COUNT));
			if (triangleCount == 0) {
//				WARN("Zero " + type + " found.");
				++stats_zeroPolylistCounter;
				continue;
			}

			//first check only <vcount> tag and find out if it contains only triangles (3) or quads (4)
			const NodeDescription * vCountNode = findElementByType(primitive, "vcount");
			if (vCountNode == nullptr) {
				WARN("No <vcount> node found inside polylist.");
				continue;
			}

			std::string vCountData = vCountNode->getString(DAE_DATA);
			std::deque<unsigned long> vCountValues;
			Util::StringUtils::extractUnsignedLongs(vCountData, vCountValues);

			bool valid = true;
			const size_t vCountSize = vCountValues.size();
			for(uint_fast32_t i = 0; i < vCountSize; ++i){
				if (vCountValues[i] != 3 && vCountValues[i] != 4) {
					valid = false;
					break;
				}
			}

			if (!valid || vCountValues.empty()) {
				continue;
			}
			const NodeDescriptionList * children = dynamic_cast<const NodeDescriptionList *> ((primitive)->getValue(DAE_CHILDREN));
			if (children == nullptr) {
				WARN("Empty description of " + type + " found.");
				continue;
			}
			uint16_t maxIndexOffset = 0;
			for (auto & elem : *children) {
				const NodeDescription * child = dynamic_cast<NodeDescription *>(elem.get());

				if (child->getString(DAE_TAG_TYPE) == "input") {
					// <input> (shared)
					// Mandatory attributes.
					const uint16_t offset = Util::StringUtils::toNumber<uint16_t>(child->getString(DAE_ATTR_OFFSET));
					std::string semantic = child->getString(DAE_ATTR_SEMANTIC);

					maxIndexOffset = std::max(maxIndexOffset, offset);

					const NodeDescription * source = findElementByRef(child, DAE_ATTR_SOURCE);
					if (source == nullptr) {
						WARN("Invalid source for " + type + '.');
						continue;
					}

					if (source->getString(DAE_TAG_TYPE) == "vertices") {
						const NodeDescriptionList * vChildren = dynamic_cast<const NodeDescriptionList *> (source->getValue(DAE_CHILDREN));
						if (vChildren == nullptr) {
							WARN("Empty description of vertices found.");
							continue;
						}
						for (auto & vChildren_vIt : *vChildren) {
							const NodeDescription * vChild = dynamic_cast<NodeDescription *>(vChildren_vIt.get());
							if (vChild->getString(DAE_TAG_TYPE) == "input") {
								// <input> (unshared)
								// Mandatory attribute.
								semantic = vChild->getString(DAE_ATTR_SEMANTIC);

								const NodeDescription * vSource = findElementByRef(vChild, DAE_ATTR_SOURCE);
								if (vSource == nullptr) {
									WARN("Invalid source for vertices.");
									continue;
								}

								vertexParts.push_back(VertexPart());
								bool result = vertexParts.back().fill(semantic, offset, vSource);
								if (!result) {
									vertexParts.pop_back();
									continue;
								}
							}
						}
					} else {
						vertexParts.push_back(VertexPart());
						bool result = vertexParts.back().fill(semantic, offset, source);
						if (!result) {
							vertexParts.pop_back();
							continue;
						}
					}
				} else if (child->getString(DAE_TAG_TYPE) == "p") {

					const std::string indexData = child->getString(DAE_DATA);
					// Convert string to numbers immediately.
					std::deque<unsigned long> pIndexVector;
					Util::StringUtils::extractUnsignedLongs(indexData, pIndexVector);

					if (pIndexVector.empty())
						continue;
					/* 2-------1
					 * |	   |
					 * |       |
					 * |       |
					 * 3-------4    is decomposed into:
					 *
					 * 2-------1
					 * |	 '
					 * |   '
					 * | '
					 * 3
					 *         1
					 *       ' |
					 *     '   |
					 *   '     |
					 * 3-------4
					 */

					int curIndex = 0;
					for(uint_fast32_t i = 0; i < vCountSize; ++i){
						if (vCountValues[i] == 4) {
							triangleCount++; //increment triangle count

							int insertPIndexVertex1 = curIndex + 3 * (maxIndexOffset + 1); //index to insert vertex copy of 3 for new triangle
							int insertPIndexVertex2 = insertPIndexVertex1 + 2 * (maxIndexOffset + 1); //index to insert vertex copy of 1 for new triangle

							//new vertex (copy of 3)
							std::deque<unsigned long> toInsert1(pIndexVector.begin() + curIndex + 2 * (maxIndexOffset + 1), pIndexVector.begin() + curIndex + 3 * (maxIndexOffset + 1));
							pIndexVector.insert(pIndexVector.begin() + insertPIndexVertex1, toInsert1.begin(), toInsert1.end());

							//new vertex (copy of 1)
							std::deque<unsigned long> toInsert2(pIndexVector.begin() + curIndex, pIndexVector.begin() + curIndex + maxIndexOffset + 1);
							pIndexVector.insert(pIndexVector.begin() + insertPIndexVertex2, toInsert2.begin(), toInsert2.end());

							curIndex += 2 * (maxIndexOffset + 1); //p vector is longer now, so increment index
						}
						curIndex += vCountValues[i] * (maxIndexOffset + 1); //increment index (also for 3-vertex polys)
					}

					// Copy to vector.
					indices.insert(indices.end(), pIndexVector.begin(), pIndexVector.end());
				}
			}
#ifdef MINSG_EXT_SKELETAL_ANIMATION
			Mesh * mesh = createSingleMesh(vertexParts, indices, triangleCount, maxIndexOffset, weights);
#else
			Mesh * mesh = createSingleMesh(vertexParts, indices, triangleCount, maxIndexOffset);
#endif
			if(mesh == nullptr) {
				WARN("Error creating mesh.");
				continue;
			}
			meshList.push_back(std::make_pair(mesh, (primitive)->getString(DAE_ATTR_MATERIAL)));
			++stats_meshCounter;
		}
		else if (type == "triangles" || type == "polygons") {
			   std::deque<VertexPart> vertexParts;
			std::vector<uint32_t> indices;
			uint32_t triangleCount = Util::StringUtils::toNumber<uint32_t>((primitive)->getString(DAE_ATTR_COUNT));
			if (triangleCount == 0) {
				WARN("Zero " + type + " found.");
				continue;
			}

			const NodeDescriptionList * children = dynamic_cast<const NodeDescriptionList *> ((primitive)->getValue(DAE_CHILDREN));
			if (children == nullptr) {
				WARN("Empty description of " + type + " found.");
				continue;
			}
			uint16_t maxIndexOffset = 0;
			for (auto & elem : *children) {
				const NodeDescription * child = dynamic_cast<NodeDescription *>(elem.get());

				if (child->getString(DAE_TAG_TYPE) == "input") {
					// <input> (shared)
					// Mandatory attributes.
					const uint16_t offset = Util::StringUtils::toNumber<uint16_t>(child->getString(DAE_ATTR_OFFSET));
					std::string semantic = child->getString(DAE_ATTR_SEMANTIC);

					maxIndexOffset = std::max(maxIndexOffset, offset);

					const NodeDescription * source = findElementByRef(child, DAE_ATTR_SOURCE);
					if (source == nullptr) {
						WARN("Invalid source for " + type + '.');
						continue;
					}

					if (source->getString(DAE_TAG_TYPE) == "vertices") {
						const NodeDescriptionList * vChildren = dynamic_cast<const NodeDescriptionList *> (source->getValue(DAE_CHILDREN));
						if (vChildren == nullptr) {
							WARN("Empty description of vertices found.");
							continue;
						}
						for (auto & vChildren_vIt : *vChildren) {
							const NodeDescription * vChild = dynamic_cast<NodeDescription *>(vChildren_vIt.get());
							if (vChild->getString(DAE_TAG_TYPE) == "input") {
								// <input> (unshared)
								// Mandatory attribute.
								semantic = vChild->getString(DAE_ATTR_SEMANTIC);

								const NodeDescription * vSource = findElementByRef(vChild, DAE_ATTR_SOURCE);
								if (vSource == nullptr) {
									WARN("Invalid source for vertices.");
									continue;
								}

								vertexParts.push_back(VertexPart());
								bool result = vertexParts.back().fill(semantic, offset, vSource);
								if (!result) {
									vertexParts.pop_back();
									continue;
								}
							}
						}
					} else {
						vertexParts.push_back(VertexPart());
						bool result = vertexParts.back().fill(semantic, offset, source);
						if (!result) {
							vertexParts.pop_back();
							continue;
						}
					}
				} else if (child->getString(DAE_TAG_TYPE) == "p") {
					const std::string indexData = child->getString(DAE_DATA);
					// Convert string to numbers immediately.
					std::deque<unsigned long> indicesTemp;
					Util::StringUtils::extractUnsignedLongs(indexData, indicesTemp);
					// Copy to vector.
					indices.insert(indices.end(), indicesTemp.begin(), indicesTemp.end());
				}
			}
#ifdef MINSG_EXT_SKELETAL_ANIMATION
			Mesh * mesh = createSingleMesh(vertexParts, indices, triangleCount, maxIndexOffset, weights);
#else
			Mesh * mesh = createSingleMesh(vertexParts, indices, triangleCount, maxIndexOffset);
#endif
			++stats_meshCounter;

			if(mesh == nullptr) {
				WARN("Error creating mesh.");
				continue;
			}
			meshList.push_back(std::make_pair(mesh, (primitive)->getString(DAE_ATTR_MATERIAL)));

		} else {
			WARN("Ignoring unsupported primitive type \"" + type + "\".");
		}
	}
	return true;
}

/*!	*/
NodeDescription * VisitorContext::createTexture(const NodeDescription * textureDesc) const {
	if (textureDesc == nullptr) {
		return nullptr;
	}

	NodeDescription * refNode = findElementByRef(textureDesc, DAE_ATTR_TEXTURE);
	if (refNode == nullptr) {
		WARN("Sampler/Image node not found.");
		return nullptr;
	}

	NodeDescription * imageNode;
	if (refNode->getString(DAE_TAG_TYPE) != "image") {
		//refNode is a sampler node (or parent of sampler node)
		const NodeDescription * sourceNode = findElementByType(refNode, "source");
		if (sourceNode == nullptr) {
			WARN("Sampler source not found.");
			return nullptr;
		}

		// Source is not a global id, but a local sid.
		const NodeDescription * surfaceNode = findElementById("sid:" + sourceNode->getString(DAE_SUBTREE_ID) + ':' + sourceNode->getString(DAE_DATA));
		if (surfaceNode == nullptr) {
			WARN("Surface not found.");
			return nullptr;
		}

		const NodeDescription * surfaceInitFromNode = findElementByType(surfaceNode, "init_from");
		if (surfaceInitFromNode == nullptr) {
			WARN("Surface init_from not found.");
			return nullptr;
		}
		std::string imageName = surfaceInitFromNode->getString(DAE_DATA);

		// imageName does not directly reference a file but is an id of an <image> node
		imageNode = findElementById(imageName);
		if (imageNode == nullptr) {
			WARN("Image not found.");
			return nullptr;
		}
	}
	else {
		//refNode is already an image node
		imageNode = refNode;
	}

	const NodeDescription * imageInitFromNode = findElementByType(imageNode, "init_from");
	if (imageInitFromNode == nullptr) {
		WARN("Image init_from not found.");
		return nullptr;
	}
	const std::string fileName = imageInitFromNode->getString(DAE_DATA);

//	std::cout << "Found texture: " << fileName << "\n";

	auto texture = new NodeDescription;
	texture->setString(Consts::TYPE, Consts::TYPE_STATE);
	texture->setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_TEXTURE);
	auto dataDesc = new NodeDescription;
	dataDesc->setString(Consts::TYPE, Consts::TYPE_DATA);
	dataDesc->setString(Consts::ATTR_DATA_TYPE, "image");
	dataDesc->setString(Consts::ATTR_TEXTURE_FILENAME, fileName);
	addToMinSGChildren(texture, dataDesc);
	return texture;
}

const NodeDescription * loadScene(std::istream & in, bool invertTransparency) {
	VisitorContext context(invertTransparency);
	using namespace std::placeholders;
	Util::MicroXML::Reader::traverse(in,
									 std::bind(std::mem_fn(&VisitorContext::enter), &context, _1, _2),
									 std::bind(std::mem_fn(&VisitorContext::leave), &context, _1),
									 std::bind(std::mem_fn(&VisitorContext::data), &context, _1, _2));

	// Clone the scene to prevent its deletion.
	return context.scene->clone();
}

}
}
}
