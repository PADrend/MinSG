/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Helper.h"

#include "StdNodeVisitors.h"
#include "../Core/Nodes/GeometryNode.h"
#include "../Core/States/MaterialState.h"
#include "../Core/Nodes/ListNode.h"
#include "../Core/States/ShaderState.h"
#include "../Core/States/TextureState.h"
#include "../SceneManagement/SceneDescription.h"
#include "../Ext/KeyFrameAnimation/KeyFrameAnimationNode.h"
#include "../Core/FrameContext.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Serialization/StreamerMD2.h> // FIXME: Remove
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/ShaderObjectInfo.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>

#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>
#include <Util/StringUtils.h>
#include <Util/Utils.h>

#include <cassert>
#include <memory>

namespace MinSG {

/**
 * Transform the mesh depending on the given flags.
 *
 * @param mesh the mesh to be modified
 * @param flags the flags describing what to do
 *      - MESH_AUTO_CENTER mesh is translated: after translation the center bounding box of the mesh is in (0,0,0)
 *      - MESH_AUTO_SCALE mesh is scaled: after scaling the maximum extent of the boundingbox is 1
 *      - MESH_AUTO_CENTER_BOTTOM mesh is translated: after translation the center of the bottom of the bounding box is in (0,0,0)
 * @note if both of MESH_AUTO_CENTER and MESH_AUTO_CENTER_BOTTOM are enabled,
 *      both are executed. since MESH_AUTO_CENTER_BOTTOM is excecuted last,
 *      MESH_AUTO_CENTER has no effect. so don't do that ;-)
 * @author Claudius Jaehn
 */
static void finalize(Rendering::MeshVertexData & vData, unsigned flags) {
	if (flags & MESH_AUTO_CENTER) {
		Geometry::Matrix4x4f transMat;
		Geometry::Box bb = vData.getBoundingBox();
		transMat.translate(-bb.getCenter());
		Rendering::MeshUtils::transform(vData, transMat);
	}
	if (flags & MESH_AUTO_CENTER_BOTTOM) {
		Geometry::Matrix4x4f transMat;
		Geometry::Vec3f v = -vData.getBoundingBox().getCenter() + Geometry::Vec3f(0, vData.getBoundingBox().getExtentY() / 2.0, 0);
		transMat.translate(v);
		Rendering::MeshUtils::transform(vData, transMat);
	}
	if (flags & MESH_AUTO_SCALE) {
		Geometry::Matrix4x4f transMat;
		if (vData.getBoundingBox().getExtentMax() > 0) {
			transMat.scale(1.0 / vData.getBoundingBox().getExtentMax());
		}
		Rendering::MeshUtils::transform(vData, transMat);
	}
}

Node * loadModel(const Util::FileName & filename, unsigned flags, Geometry::Matrix4x4 * transMat) {
	std::unique_ptr<Util::GenericAttributeList> genericList(Rendering::Serialization::loadGeneric(filename));
	if(!genericList || genericList->empty()) {
		WARN("No Mesh created.");
		return nullptr;
	}

	// Material name to material mapping.
	std::map<std::string, Util::GenericAttributeMap *> materials;

	std::deque<Node *> nodes;
	for (auto & elem : *genericList) {
		Util::GenericAttributeMap *d = dynamic_cast<Util::GenericAttributeMap *>(elem.get());
		if(d==nullptr){
			WARN("loadModel: Loader must return Descriptions.");
			if(elem)
				std::cout << (elem)->toString()<<"\n";
			continue;
		}

		const std::string type=d->getString(Rendering::Serialization::DESCRIPTION_TYPE);
		if(type==Rendering::Serialization::DESCRIPTION_TYPE_MESH){ // \todo move this to separate function!
			Rendering::Serialization::MeshWrapper_t *meshWrapper = dynamic_cast<Rendering::Serialization::MeshWrapper_t*> (d->getValue(Rendering::Serialization::DESCRIPTION_DATA));

			Rendering::Mesh * mesh = meshWrapper ? meshWrapper->get() : nullptr;
			if (mesh != nullptr) {
				// \todo (CL) Shouldn't this be rearranged? Would allow: scale to 1.0 and then scale to fixed size.
				if (transMat)
					Rendering::MeshUtils::transform(mesh->openVertexData(), *transMat);

				if (flags) {
					finalize(mesh->openVertexData(), flags);
				}
			} else {
				WARN("Could not load geometry.");
			}

			Node * node = new GeometryNode();

			if (mesh != nullptr) {
				dynamic_cast<GeometryNode *> (node)->setMesh(mesh);
				mesh->setFileName(filename);
			}

			// add texture
			{
				std::string textureName = d->getString(Rendering::Serialization::DESCRIPTION_TEXTURE_FILE, "");
				if (!textureName.empty()) {
					std::cout << "TeX: " << textureName << std::endl;
					node->addState(loadTexture(Util::FileName(textureName), true));
				}
			}

			// add material
			std::string materialName = d->getString(Rendering::Serialization::DESCRIPTION_MATERIAL_NAME, "");

			if (!materialName.empty()) {
				std::map<std::string, Util::GenericAttributeMap *>::const_iterator materialIt = materials.find(materialName);
				if (materialIt != materials.end()) {
					Util::GenericAttributeMap * materialDesc = materialIt->second;
					std::string materialAmbient = materialDesc->getString(Rendering::Serialization::DESCRIPTION_MATERIAL_AMBIENT, "");
					std::string materialDiffuse = materialDesc->getString(Rendering::Serialization::DESCRIPTION_MATERIAL_DIFFUSE, "");
					std::string materialSpecular = materialDesc->getString(Rendering::Serialization::DESCRIPTION_MATERIAL_SPECULAR, "");
					std::string materialShininess = materialDesc->getString(Rendering::Serialization::DESCRIPTION_MATERIAL_SHININESS, "");

					if (!materialAmbient.empty() || !materialDiffuse.empty() || !materialSpecular.empty() || !materialShininess.empty()) {
						Rendering::MaterialParameters materialParams;

						if (!materialAmbient.empty()) {
							std::vector<float> values = Util::StringUtils::toFloats(materialAmbient);
							if (values.size() == 4) {
								materialParams.setAmbient(Util::Color4f(values[0], values[1], values[2], values[3]));
							} else if (values.size() == 3) {
								materialParams.setAmbient(Util::Color4f(values[0], values[1], values[2], 1.0f));
							} else {
								WARN("Invalid number of values in ambient material string \"" + materialAmbient + "\".");
							}
						}

						if (!materialDiffuse.empty()) {
							std::vector<float> values = Util::StringUtils::toFloats(materialDiffuse);
							if (values.size() == 4) {
								materialParams.setDiffuse(Util::Color4f(values[0], values[1], values[2], values[3]));
							} else if (values.size() == 3) {
								materialParams.setDiffuse(Util::Color4f(values[0], values[1], values[2], 1.0f));
							} else {
								WARN("Invalid number of values in diffuse material string \"" + materialDiffuse + "\".");
							}
						}

						if (!materialSpecular.empty()) {
							std::vector<float> values = Util::StringUtils::toFloats(materialSpecular);
							if (values.size() == 4) {
								materialParams.setSpecular(Util::Color4f(values[0], values[1], values[2], values[3]));
							} else if (values.size() == 3) {
								materialParams.setSpecular(Util::Color4f(values[0], values[1], values[2], 1.0f));
							} else {
								WARN("Invalid number of values in ambient material string \"" + materialSpecular + "\".");
							}
						}

						if (!materialShininess.empty()) {
							materialParams.setShininess(Util::StringUtils::toNumber<float>(materialShininess));
						}

						node->addState(new MaterialState(materialParams));
					}

					// Add texture from material.
					std::string materialTexture = materialDesc->getString(Rendering::Serialization::DESCRIPTION_TEXTURE_FILE, "");
					if (!materialTexture.empty()) {
						Util::FileName texturePath;
						Util::FileUtils::findFile(Util::FileName(materialTexture), filename.getDir(), texturePath);
						node->addState(loadTexture(texturePath, true));
					}
				}
			}

			nodes.push_back(node);
		}else if(type == Rendering::StreamerMD2::DESCRIPTION_TYPE_KEYFRAME_ANIMATION){

			using Rendering::StreamerMD2;

			Util::FileName * md2FileName = new Util::FileName(d->getString(Rendering::Serialization::DESCRIPTION_FILE));

			StreamerMD2::indexDataWrapper * indexDataWrapper = dynamic_cast<StreamerMD2::indexDataWrapper*> (d->getValue(StreamerMD2::DESCRIPTION_MESH_INDEX_DATA));
			const Rendering::MeshIndexData & indexData = indexDataWrapper->ref();

			StreamerMD2::framesDataWrapper * framesDataWrapper = dynamic_cast<StreamerMD2::framesDataWrapper*> (d->getValue(StreamerMD2::DESCRIPTION_KEYFRAMES_DATA));
			std::vector<Rendering::MeshVertexData> & framesData = framesDataWrapper->ref();


			if(!framesData.empty()){

				//apply transMat if given
				if(transMat){
					for(auto & framesData_i : framesData){
						Rendering::MeshUtils::transform(framesData_i,*transMat);
					}
				}

				//apply flags if given
				if(flags){
					if (flags & MESH_AUTO_CENTER) {
						Geometry::Box bb = framesData[0].getBoundingBox();
						for (unsigned short i = 1; i < framesData.size(); i++) {
							bb.include(framesData[i].getBoundingBox());
						}
						Geometry::Matrix4x4f translateMat;
						translateMat.translate(-bb.getCenter());
						for (auto & framesData_i : framesData) {
							Rendering::MeshUtils::transform(framesData_i, translateMat);
						}
					}
					if (flags & MESH_AUTO_CENTER_BOTTOM) {
						Geometry::Box bb = framesData[0].getBoundingBox();
						for (unsigned short i = 1; i < framesData.size(); i++) {
							bb.include(framesData[i].getBoundingBox());
						}
						Geometry::Vec3f v = -bb.getCenter() + Geometry::Vec3f(0, bb.getExtentY() / 2.0, 0);
						Geometry::Matrix4x4f translateMat;
						translateMat.translate(v);
						for (auto & framesData_i : framesData) {
							Rendering::MeshUtils::transform(framesData_i, translateMat);
						}
					}
					if (flags & MESH_AUTO_SCALE) {
						Geometry::Matrix4x4f translateMat;
						Geometry::Box bb = framesData[0].getBoundingBox();
						for (unsigned short i = 1; i < framesData.size(); i++) {
							bb.include(framesData[i].getBoundingBox());
						}
						if (bb.getExtentMax() > 0)
							translateMat.scale(1.0 / bb.getExtentMax());
						for (auto & framesData_i : framesData) {
							Rendering::MeshUtils::transform(framesData_i, translateMat);
						}
					}
				}
			}

			StreamerMD2::animationDataWrapper * animationDataWrapper = dynamic_cast<StreamerMD2::animationDataWrapper*> (d->getValue(StreamerMD2::DESCRIPTION_ANIMATIONS));
			std::map<std::string, std::vector<int> > animationData = animationDataWrapper->get();

			KeyFrameAnimationNode * keyFrameAnimationNode = new KeyFrameAnimationNode(indexData, framesData, animationData);

			StreamerMD2::textureFilesWrapper * textureFilesWrapper = dynamic_cast<StreamerMD2::textureFilesWrapper*> (d->getValue(StreamerMD2::DESCRIPTION_TEXTURE_FILES));
			std::vector<std::string> textureFiles = textureFilesWrapper->get();
			std::vector<TextureState *> textureStates;
			for(auto & textureFile : textureFiles) {
				Util::FileName newName;
				std::list<std::string> hints;
				hints.push_back(md2FileName->getDir());
				hints.push_back("");
				auto fileName = new Util::FileName(textureFile);
				if(Util::FileUtils::findFile(*fileName, hints, newName)) {
					textureStates.push_back(loadTexture(newName));
				}
				else{
					WARN(std::string("Cannot find file \"") + textureFile + "\".");
				}
			}
			//set first texture
			if(!textureStates.empty()) {
				keyFrameAnimationNode->addState(textureStates[0]);
			}

			nodes.push_back(keyFrameAnimationNode);
		} else if (type == Rendering::Serialization::DESCRIPTION_TYPE_MATERIAL) {
			Util::FileName mtlFile;
			Util::FileUtils::findFile(Util::FileName(d->getString(Rendering::Serialization::DESCRIPTION_FILE)), filename.getDir(), mtlFile);
			std::unique_ptr<Util::GenericAttributeList> mtlList(Rendering::Serialization::loadGeneric(mtlFile));
			if (mtlList.get() == nullptr || mtlList->empty()) {
				WARN("Error loading material library.");
				continue;
			}
			for (auto mtlIt = mtlList->begin(); mtlIt != mtlList->end(); ) {
				Util::GenericAttributeMap * mtlDesc = dynamic_cast<Util::GenericAttributeMap *> (mtlIt->get());
				if (mtlDesc == nullptr || mtlDesc->getString(Rendering::Serialization::DESCRIPTION_TYPE) != Rendering::Serialization::DESCRIPTION_TYPE_MATERIAL) {
					WARN("Error accessing material description");
					++mtlIt;
					continue;
				}
				materials.insert(std::make_pair(mtlDesc->getString(Rendering::Serialization::DESCRIPTION_MATERIAL_NAME), mtlDesc));
				// Remove material description from the list so that it will not be destroyed when the list is destroyed.
				mtlIt = mtlList->erase(mtlIt);
			}
		}else{
			WARN(std::string("Unknown data type: ")+type);
		}

	}

	if (nodes.empty()) {
		return nullptr;
	} else if (nodes.size() == 1) {
		return nodes.front();
	} else {
		auto ln = new ListNode();
		for (auto & node : nodes)
			ln->addChild(node);
		return ln;
	}
}

void destroy(Node * rootNode) {
	if(rootNode) {
		Util::Reference<Node> n(rootNode);
		n->destroy();
	}
}

void initShaderState(ShaderState * shaderState, 
					 const std::deque<std::string> & vsFiles,
					 const std::deque<std::string> & gsFiles,
					 const std::deque<std::string> & fsFiles,
					 Rendering::Shader::flag_t usage) {
	using namespace SceneManagement;
	assert(shaderState != nullptr);
	Rendering::Shader * shader = Rendering::Shader::createShader(usage);
	shaderState->setShader(shader);

	// store fileDescriptions as attribute: { Consts::STATE_ATTR_SHADER_FILES : [ fileDescription* ] } 
	auto fileDescriptions = new NodeDescriptionList;
	
	shaderState->setAttribute(Consts::STATE_ATTR_SHADER_FILES, fileDescriptions);

	for(const auto & vsFile : vsFiles) {
		shader->attachShaderObject(Rendering::ShaderObjectInfo::loadVertex(Util::FileName(vsFile)));
		auto nd = new NodeDescription;
		nd->setString(Consts::ATTR_DATA_TYPE, Consts::DATA_TYPE_GLSL_VS);
		nd->setString(Consts::ATTR_SHADER_OBJ_FILENAME, vsFile);
		fileDescriptions->push_back(nd);
	}
	for(const auto & gsFile : gsFiles) {
		shader->attachShaderObject(Rendering::ShaderObjectInfo::loadGeometry(Util::FileName(gsFile)));
		auto nd = new NodeDescription;
		nd->setString(Consts::ATTR_DATA_TYPE, Consts::DATA_TYPE_GLSL_GS);
		nd->setString(Consts::ATTR_SHADER_OBJ_FILENAME, gsFile);
		fileDescriptions->push_back(nd);
	}
	for(const auto & fsFile : fsFiles) {
		shader->attachShaderObject(Rendering::ShaderObjectInfo::loadFragment(Util::FileName(fsFile)));
		auto nd = new NodeDescription;
		nd->setString(Consts::ATTR_DATA_TYPE, Consts::DATA_TYPE_GLSL_FS);
		nd->setString(Consts::ATTR_SHADER_OBJ_FILENAME, fsFile);
		fileDescriptions->push_back(nd);
	}
}

TextureState * loadTexture(const Util::FileName & filename,
						   bool useMipmaps,
						   bool clampToEdge,
						   int textureUnit,
						   std::map<const std::string, Util::Reference<Rendering::Texture>> * textureRegistry) {
	Rendering::Texture * img = nullptr;
	if(textureRegistry != nullptr) {
		auto it = textureRegistry->find(filename.toString());
		if(it != textureRegistry->end()) {
			// Reuse existing texture from texture registry
			img = it->second.get();
		}
	}

	// Do not load a texture if it was already found in the texture registry
	if(img == nullptr) {
		img = Rendering::Serialization::loadTexture(filename, useMipmaps, clampToEdge);
		
		// Register the newly loaded texture
		if(img != nullptr && textureRegistry != nullptr) {
			(*textureRegistry)[filename.toString()] = img;
		}
	}

	if(img == nullptr) {
		WARN(std::string("Could not load texture: ") + filename.toString());
		img = Rendering::TextureUtils::createChessTexture(64, 64);
		img->setFileName(filename);
	}

	auto textureState = new TextureState;
	textureState->setTextureUnit(textureUnit);
	textureState->setTexture(img);
	return textureState;
}


void changeParentKeepTransformation(Util::Reference<Node> child, GroupNode * newParent){
		
	const Geometry::Matrix4x4f ncMat = (
		newParent != nullptr
		? newParent->getWorldMatrix().inverse() * child->getWorldMatrix()
		: child->getWorldMatrix()
	);

	if(newParent){
		newParent->addChild(child.get());
	}
	else if(child->hasParent()){
		child->getParent()->removeChild(child.get());
	}
	
	if(child->hasSRT() && ncMat.convertsSafelyToSRT()){
		child->setSRT(ncMat._toSRT());
	}
	else{
		child->setMatrix(ncMat);
	}
}

Geometry::Box combineNodesWorldBBs(const std::vector<Node*> & nodes){
	Geometry::Box b;
	b.invalidate();
	for(const auto & node : nodes)
		b.include(node->getWorldBB());
	return b;
}
}
