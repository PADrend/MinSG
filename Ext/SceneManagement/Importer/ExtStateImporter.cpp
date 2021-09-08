/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtStateImporter.h"
#include "../ExtConsts.h"

#include "../../../SceneManagement/SceneManager.h"
#include "../../../SceneManagement/SceneDescription.h"
#include "../../../SceneManagement/ImportFunctions.h"
#include "../../../SceneManagement/Importer/ImportContext.h"
#include "../../../SceneManagement/Importer/ImporterTools.h"

#include "../../../Core/Nodes/GroupNode.h"
#include "../../../Core/Nodes/ListNode.h"
#include "../../../Core/Nodes/Node.h"
#include "../../../Core/Nodes/LightNode.h"

#include "../../OcclusionCulling/OccRenderer.h"
#include "../../OcclusionCulling/CHCppRenderer.h"
#include "../../States/BudgetAnnotationState.h"
#include "../../States/MirrorState.h"
#include "../../States/ProjSizeFilterState.h"
#include "../../States/SkyboxState.h"
#include "../../States/LODRenderer.h"
#include "../../States/ShadowState.h"
#include "../../States/SkinningState.h"
#include "../../States/IBLEnvironmentState.h"
#include "../../States/PbrMaterialState.h"

#ifdef MINSG_EXT_COLORCUBES
#include "../../ColorCubes/ColorCubeRenderer.h"
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#include "../../SkeletalAnimation/Renderer/SkeletalSoftwareRendererState.h"
#include "../../SkeletalAnimation/Renderer/SkeletalHardwareRendererState.h"
#include "../../SkeletalAnimation/SkeletalNode.h"
#endif

#ifdef MINSG_EXT_BLUE_SURFELS
#include "../../BlueSurfels/SurfelRenderer.h"
#include "../../BlueSurfels/Strategies/AbstractSurfelStrategy.h"
#endif // MINSG_EXT_BLUE_SURFELS

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "../../MultiAlgoRendering/AlgoSelector.h"
#include "../../MultiAlgoRendering/SampleContext.h"
#include "../../MultiAlgoRendering/SampleStorage.h"
#include "../../MultiAlgoRendering/SurfelRenderer.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#endif

#ifdef MINSG_EXT_SVS
#include "../../SVS/BudgetRenderer.h"
#include "../../SVS/Helper.h"
#include "../../SVS/Renderer.h"
#endif

#include <Rendering/Shader/Uniform.h>
#include <Rendering/Serialization/Serialization.h>

#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>
#include <Util/StringIdentifier.h>
#include <Util/Macros.h>
#include <Util/Encoding.h>

#include <cstddef>
#include <deque>
#include <list>
#include <functional>
#include <cassert>
#include <memory>

namespace MinSG {
namespace SceneManagement {

template < typename T >
static T * convertToTNode(Node * node) {
	T * t = dynamic_cast<T *>(node);
	if(t == nullptr)
		WARN(std::string(node != nullptr ? node->getTypeName() : "nullptr") + " can not be casted to " + T::getClassName());
	return t;
}

#ifdef MINSG_EXT_BLUE_SURFELS

static bool importSurfelRenderer(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parent) {
	if(type != Consts::STATE_TYPE_SURFEL_RENDERER) 
		return false;
	
	Util::Reference<BlueSurfels::SurfelRenderer> state = new BlueSurfels::SurfelRenderer;
	const DescriptionArray* children = dynamic_cast<const DescriptionArray*>(d.getValue(Consts::CHILDREN));

	for(const auto* child : ImporterTools::filterElements(BlueSurfels::TYPE_STRATEGY, children)) {
		auto* strategy = BlueSurfels::importStrategy(child);
		if(strategy)
			state->addSurfelStrategy(strategy);
	}

	ImporterTools::finalizeState(ctxt, state.get(), d);
	parent->addState(state.get());
	return true;
}

#endif // MINSG_EXT_BLUE_SURFELS

#ifdef MINSG_EXT_MULTIALGORENDERING

static void finalizeAlgoSelector(ImportContext & ctxt, MAR::AlgoSelector * as){
	as->getSampleContext()->getSampleStorage()->initNodeIndices(ctxt.getRootNode());
}

static bool importAlgoSelector(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parent) {
	if(type != Consts::STATE_TYPE_ALGOSELECTOR) // check parent != nullptr is done by SceneManager
		return false;

	Util::FileName file(ctxt.fileName);
	file.setEnding(".mar");
	auto in = Util::FileUtils::openForReading(file);

	MAR::AlgoSelector * as;
	if(in)
		as = MAR::AlgoSelector::create(*(in.get()));
	else{
		WARN("could not load sampling data");
		return false;
	}
	ctxt.addFinalizingAction(std::bind(finalizeAlgoSelector, std::placeholders::_1, as));

	ImporterTools::finalizeState(ctxt, as, d);
	parent->addState(as);
	return true;
}

static bool importMARSurfelRenderer(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parent) {
	if(type != Consts::STATE_TYPE_MAR_SURFEL_RENDERER) // check parent != nullptr is done by SceneManager
		return false;
	
	Util::Reference<MAR::SurfelRenderer> sr = new MAR::SurfelRenderer();
	sr->setSurfelCountFactor(d.getFloat(Consts::ATTR_MAR_SURFEL_COUNT_FACTOR, 8.0f));
	sr->setSurfelSizeFactor(d.getFloat(Consts::ATTR_MAR_SURFEL_SIZE_FACTOR, 1.0f));
	sr->setMaxAutoSurfelSize(d.getFloat(Consts::ATTR_MAR_SURFEL_MAX_AUTO_SIZE, 10.0f));
	sr->setForceSurfels(d.getBool(Consts::ATTR_MAR_SURFEL_FORCE_FLAG, false));
	
	ImporterTools::finalizeState(ctxt, sr.get(), d);
	parent->addState(sr.get());
	return true;
}
#endif

#ifdef MINSG_EXT_COLORCUBES
static bool importColorCubeRenderer(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_COLOR_CUBE_RENDERER) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new ColorCubeRenderer;
	state->setHighlightEnabled(d.getBool(Consts::ATTR_COLOR_CUBE_RENDERER_HIGHLIGHT, false));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}
#endif

static bool importOccRenderer(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_OCC_RENDERER) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new OccRenderer;

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importCHCppRenderer(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_CHCPP_RENDERER) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new CHCppRenderer(
		d.getUInt(Consts::ATTR_CHCPP_VISIBILITYTHRESHOLD),
		d.getUInt(Consts::ATTR_CHCPP_MAXPREVINVISNODESBATCHSIZE),
		d.getUInt(Consts::ATTR_CHCPP_SKIPPEDFRAMESTILLQUERY),
		d.getUInt(Consts::ATTR_CHCPP_MAXDEPTHFORTIGHTBOUNDINGVOLUMES),
		d.getFloat(Consts::ATTR_CHCPP_MAXAREADERIVATIONFORTIGHTBOUNDINGVOLUMES)
	);

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importBudgetAnnotationState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & desc, Node * parent) {
	if(stateType != Consts::STATE_TYPE_BUDGET_ANNOTATION_STATE) { // check parent != nullptr is done by SceneManager
		return false;
	}

	auto bas = new BudgetAnnotationState;
	if(desc.contains(Consts::ATTR_BAS_ANNOTATION_ATTRIBUTE)) {
		bas->setAnnotationAttribute(desc.getString(Consts::ATTR_BAS_ANNOTATION_ATTRIBUTE));
	}
	if(desc.contains(Consts::ATTR_BAS_BUDGET)) {
		bas->setBudget(desc.getDouble(Consts::ATTR_BAS_BUDGET));
	}
	if(desc.contains(Consts::ATTR_BAS_DISTRIBUTION_TYPE)) {
		bas->setDistributionType(BudgetAnnotationState::distributionTypeFromString(desc.getString(Consts::ATTR_BAS_DISTRIBUTION_TYPE)));
	}

	ImporterTools::finalizeState(ctxt, bas, desc);
	parent->addState(bas);
	return true;
}

static bool importMirrorState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_MIRROR_STATE) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new MirrorState(d.getUInt(Consts::ATTR_MIRROR_TEXTURE_SIZE));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importProjSizeFilterState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_PROJ_SIZE_FILTER_STATE) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new ProjSizeFilterState();
	Util::GenericAttribute * ga;

	ga = d.getValue(Consts::ATTR_PSFS_MAXIMUM_PROJECTED_SIZE);
	if(ga) state->setMaximumProjSize(ga->toFloat());

	ga = d.getValue(Consts::ATTR_PSFS_MINIMUM_DISTANCE);
	if(ga) state->setMinimumDistance(ga->toFloat());

	ga = d.getValue(Consts::ATTR_PSFS_SOURCE_CHANNEL);
	if(ga) state->setSourceChannel(ga->toString());

	ga = d.getValue(Consts::ATTR_PSFS_TARGET_CHANNEL);
	if(ga) state->setTargetChannel(ga->toString());

	ga = d.getValue(Consts::ATTR_PSFS_FORCE_CLOSED_NODES);
	if(ga) state->setForceClosed(ga->toBool());

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importSkyboxState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SKYBOX) // check parent != nullptr is done by SceneManager
		return false;

	static const Util::StringIdentifier ATTR_SKYBOX_FILE("file");
	SkyboxState * state = SkyboxState::createSkybox(d.getString(ATTR_SKYBOX_FILE));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importShadowState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SHADOW_STATE) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new ShadowState(d.getUInt(Consts::ATTR_SHADOW_TEXTURE_SIZE));
	state->setLight(dynamic_cast<LightNode*>(ctxt.sceneManager.getRegisteredNode(d.getString(Consts::ATTR_SHADOW_LIGHT_NODE))));
	state->setStatic(d.getBool(Consts::ATTR_SHADOW_STATIC, false));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

#ifdef MINSG_EXT_SKELETAL_ANIMATION
static bool importSkeletalHardwareRendererState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SKEL_SKELETALHARDWARERENDERERSTATE &&
       stateType != Consts::STATE_TYPE_SKEL_SKELETALRENDERERSTATE) // check parent != nullptr is done by SceneManager
		return false;

	SkeletalAbstractRendererState * state = new SkeletalHardwareRendererState();
	state->setBindMatrix(Util::StringUtils::toFloats(d.getString(Consts::ATTR_SKEL_BINDMATRIX)));

    parent->addState(state);
	ImporterTools::finalizeState(ctxt, state, d);
	return true;
}
#endif

#ifdef MINSG_EXT_SVS
static bool importSVSRenderer(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & desc, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SVS_RENDERER) { // check parent != nullptr is done by SceneManager
		return false;
	}

	auto renderer = new SVS::Renderer;
	if(desc.contains(Consts::ATTR_SVS_INTERPOLATION_METHOD)) {
		renderer->setInterpolationMethod(SVS::interpolationFromString(desc.getString(Consts::ATTR_SVS_INTERPOLATION_METHOD)));
	}
	if(desc.contains(Consts::ATTR_SVS_RENDERER_SPHERE_OCCLUSION_TEST)) {
		if(desc.getBool(Consts::ATTR_SVS_RENDERER_SPHERE_OCCLUSION_TEST)) {
			renderer->enableSphereOcclusionTest();
		} else {
			renderer->disableSphereOcclusionTest();
		}
	}
	if(desc.contains(Consts::ATTR_SVS_RENDERER_GEOMETRY_OCCLUSION_TEST)) {
		if(desc.getBool(Consts::ATTR_SVS_RENDERER_GEOMETRY_OCCLUSION_TEST)) {
			renderer->enableGeometryOcclusionTest();
		} else {
			renderer->disableGeometryOcclusionTest();
		}
	}

	ImporterTools::finalizeState(ctxt, renderer, desc);
	parent->addState(renderer);
	return true;
}

static bool importSVSBudgetRenderer(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & desc, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SVS_BUDGETRENDERER) { // check parent != nullptr is done by SceneManager
		return false;
	}

	auto renderer = new SVS::BudgetRenderer;
	if(desc.contains(Consts::ATTR_SVS_BUDGETRENDERER_BUDGET)) {
		renderer->setBudget(desc.getValue(Consts::ATTR_SVS_BUDGETRENDERER_BUDGET)->toUnsignedInt());
	}

	ImporterTools::finalizeState(ctxt, renderer, desc);
	parent->addState(renderer);
	return true;
}
#endif

static bool importLODRenderer(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_LOD_RENDERER) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new LODRenderer();

	state->setMaxComplexity(d.getUInt(Consts::ATTR_LOD_RENDERER_MAX_COMPLEXITY));
	state->setMinComplexity(d.getUInt(Consts::ATTR_LOD_RENDERER_MIN_COMPLEXITY));
	state->setRelComplexity(d.getFloat(Consts::ATTR_LOD_RENDERER_REL_COMPLEXITY));
	state->setSourceChannel(d.getString(Consts::ATTR_LOD_RENDERER_SOURCE_CHANNEL));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importSkinningState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SKINNING_STATE)
		return false;

	auto state = new SkinningState();

	std::vector<Geometry::Matrix4x4> inverseBindMatrices;
	std::vector<Util::StringIdentifier> jointIds;

	const DescriptionArray* children = dynamic_cast<const DescriptionArray*>(d.getValue(Consts::CHILDREN));

	for(const auto* child : ImporterTools::filterElements(Consts::TYPE_SKINNING_JOINT, children)) {
		auto jointId = child->getString(Consts::ATTR_SKINNING_JOINT_ID);
		std::stringstream ss(child->getString(Consts::ATTR_SKINNING_INVERSE_BINDING_MATRIX));
		Geometry::Matrix4x4 mat;
		ss >> mat;
		inverseBindMatrices.emplace_back(std::move(mat));
		jointIds.emplace_back(jointId);
	}

	// defer loading of joint nodes
	ctxt.addFinalizingAction([state, inverseBindMatrices, jointIds](ImportContext & ctxt) {
		for(size_t i=0; i<jointIds.size(); ++i) {
			auto node = ctxt.sceneManager.getRegisteredNode(jointIds[i]);
			if(node) {
				state->addJoint(node, inverseBindMatrices[i]);
			} else {
				WARN("Could not import skinning joint '" + jointIds[i].toString() + "'. No node by this name was found.");
			}
		}
	});

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importIBLEnvState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_IBL_ENV_STATE) // check parent != nullptr is done by SceneManager
		return false;

	std::unique_ptr<IBLEnvironmentState> state(new IBLEnvironmentState);
	state->setDrawEnvironment(d.getBool(Consts::ATTR_IBL_ENV_DRAW_ENV));
	state->setLOD(d.getFloat(Consts::ATTR_IBL_ENV_LOD));
	state->setRotationDeg(d.getFloat(Consts::ATTR_IBL_ENV_ROTATION));
	const Util::FileName hdrFilename(d.getString(Consts::ATTR_IBL_ENV_FILE));
	const auto dataDescList = ImporterTools::filterElements(Consts::TYPE_DATA, dynamic_cast<const DescriptionArray *>(d.getValue(Consts::CHILDREN)));
	if(!hdrFilename.empty()) {
		state->loadEnvironmentMapFromHDR(Util::FileName(d.getString(Consts::ATTR_IBL_ENV_FILE)));
	} else if(!dataDescList.empty()) {
		WARN_AND_RETURN_IF(dataDescList.size() != 1, "IBLEnvironmentState needs one data description. Got " + std::to_string(dataDescList.size()), false);

		// filename
		const DescriptionMap * dataDesc = dataDescList.front();

		// dataType
		const std::string dataType = dataDesc->getString(Consts::ATTR_DATA_TYPE);
		if(!Util::StringUtils::beginsWith(dataType.c_str(),"image")) {
			WARN(std::string("importTextureState: Unknown data type '")+dataType+"' (still trying to load the file though...)");
		}

		Util::Reference<Rendering::Texture> texture;
		const Util::FileName fileName(dataDesc->getString(Consts::ATTR_TEXTURE_FILENAME));
		if(fileName.empty()) {
			// Load image data from a Base64 encoded block.
			if(dataDesc->getString(Consts::ATTR_DATA_ENCODING) == Consts::DATA_ENCODING_BASE64) {
				const std::vector<uint8_t> rawData = Util::decodeBase64(dataDesc->getString(Consts::DATA_BLOCK));
				texture = Rendering::Serialization::loadTexture("hdr", std::string(rawData.begin(), rawData.end()), Rendering::TextureType::TEXTURE_CUBE_MAP, 6);
			} else {
				auto* textureWrapper = dynamic_cast<Util::ReferenceAttribute<Rendering::Texture>*>(dataDesc->getValue(Consts::ATTR_TEXTURE_DATA));
				if(!textureWrapper) {
					WARN("importTextureState: Invalid or no Texture data.");
				} else {
					// A Texture-Object is already contained in the description.
					texture = textureWrapper->get();
				}
			}
		} else {
			const auto location = ctxt.fileLocator.locateFile( fileName );
			const Util::FileName filename2 = location.first ? location.second : fileName;
			texture = Rendering::Serialization::loadTexture(filename2, Rendering::TextureType::TEXTURE_CUBE_MAP, 6);

			if( !texture ) {
				WARN(std::string("Could not load texture: ") + filename2.toString());
			}else{
				texture->setFileName(fileName);	// set original filename
			}
		}
		state->setEnvironmentMap(texture);
	}

	ImporterTools::finalizeState(ctxt, state.get(), d);
	parent->addState(state.release());
	return true;
}

static Rendering::Texture* importTextureFromData(ImportContext & ctxt, const DescriptionMap * dataDesc) {

	// dataType
	const std::string dataType = dataDesc->getString(Consts::ATTR_DATA_TYPE);
	if(!Util::StringUtils::beginsWith(dataType.c_str(),"image")) {
		WARN(std::string("importTextureState: Unknown data type '")+dataType+"' (still trying to load the file though...)");
	}

	const uint32_t numLayers = Util::StringUtils::toNumber<uint32_t>(dataDesc->getString(Consts::ATTR_TEXTURE_NUM_LAYERS, "1"));
	const std::string textureTypeString = dataDesc->getString(Consts::ATTR_TEXTURE_TYPE);
	const Rendering::TextureType textureType = textureTypeString.empty() ? 
													Rendering::TextureType::TEXTURE_2D :
													static_cast<Rendering::TextureType>(Util::StringUtils::toNumber<uint32_t>(textureTypeString));

	Util::Reference<Rendering::Texture> texture;
	const Util::FileName fileName(dataDesc->getString(Consts::ATTR_TEXTURE_FILENAME));
	if(fileName.empty()) {
		// Load image data from a Base64 encoded block.
		if(dataDesc->getString(Consts::ATTR_DATA_ENCODING) == Consts::DATA_ENCODING_BASE64) {
			const std::vector<uint8_t> rawData = Util::decodeBase64(dataDesc->getString(Consts::DATA_BLOCK));
			texture = Rendering::Serialization::loadTexture(
											dataDesc->getString(Consts::ATTR_DATA_FORMAT,"png"), std::string(rawData.begin(), rawData.end()),
											textureType, numLayers, 4);
		} else {
			auto* textureWrapper = dynamic_cast<Util::ReferenceAttribute<Rendering::Texture>*>(dataDesc->getValue(Consts::ATTR_TEXTURE_DATA));
			if(!textureWrapper) {
				WARN("importTextureState: Invalid or no Texture data.");
			} else {
				// A Texture-Object is already contained in the description.
				texture = textureWrapper->get();
			}
		}
	} else {
		const auto location = ctxt.fileLocator.locateFile( fileName );
		const Util::FileName filename2 = location.first ? location.second : fileName;
			
		if( (ctxt.importOptions & IMPORT_OPTION_USE_TEXTURE_REGISTRY) > 0) {
			auto it = ctxt.getTextureRegistry().find(filename2.toString());
			if(it != ctxt.getTextureRegistry().end()) {
				// Reuse existing texture from texture registry
				texture = it->second.get();
			}
		}
		if( !texture ){
			texture = Rendering::Serialization::loadTexture(filename2, textureType, numLayers, 4);
			if( (ctxt.importOptions & IMPORT_OPTION_USE_TEXTURE_REGISTRY) > 0) 
				 ctxt.getTextureRegistry()[filename2.toString()] = texture;
		}
		if( !texture ) {
			WARN(std::string("Could not load texture: ") + filename2.toString());
		}else{
			texture->setFileName(fileName);	// set original filename
		}
	}
	return texture.detachAndDecrease();
}

static bool importPbrMaterialState(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_PBR_MATERIAL_STATE) // check parent != nullptr is done by SceneManager
		return false;
	
	
	auto deserializeMatrix = [](const std::string& s) {
		auto values = Util::StringUtils::toFloats(s);
		return values.size() >= 9 ? Geometry::Matrix3x3(values.data()) : Geometry::Matrix3x3();
	};

	PbrMaterial material{};
	auto colValues = Util::StringUtils::toFloats(d.getString(Consts::ATTR_PBR_MAT_BASECOLOR_FACTOR));
	if(colValues.size() == 4)
		material.baseColor.factor = Util::Color4f(colValues);
	material.baseColor.texTransform = deserializeMatrix(d.getString(Consts::ATTR_PBR_MAT_BASECOLOR_TEXTRANSFORM));
	material.baseColor.texCoord = d.getUInt(Consts::ATTR_PBR_MAT_BASECOLOR_TEXCOORD, material.baseColor.texCoord);
	material.baseColor.texUnit = static_cast<uint8_t>(d.getUInt(Consts::ATTR_PBR_MAT_BASECOLOR_TEXUNIT, material.baseColor.texUnit));
	material.metallicRoughness.metallicFactor = d.getFloat(Consts::ATTR_PBR_MAT_METALLICFACTOR, material.metallicRoughness.metallicFactor);
	material.metallicRoughness.roughnessFactor = d.getFloat(Consts::ATTR_PBR_MAT_ROUGHNESSFACTOR, material.metallicRoughness.roughnessFactor);
	material.metallicRoughness.texTransform = deserializeMatrix(d.getString(Consts::ATTR_PBR_MAT_METALLIC_ROUGHNESS_TEXTRANSFORM));
	material.metallicRoughness.texCoord = d.getUInt(Consts::ATTR_PBR_MAT_METALLIC_ROUGHNESS_TEXCOORD, material.metallicRoughness.texCoord);
	material.metallicRoughness.texUnit = static_cast<uint8_t>(d.getUInt(Consts::ATTR_PBR_MAT_METALLIC_ROUGHNESS_TEXUNIT, material.metallicRoughness.texUnit));
	material.normal.scale = d.getFloat(Consts::ATTR_PBR_MAT_NORMAL_SCALE, material.normal.scale);
	material.normal.texTransform = deserializeMatrix(d.getString(Consts::ATTR_PBR_MAT_NORMAL_TEXTRANSFORM));
	material.normal.texCoord = d.getUInt(Consts::ATTR_PBR_MAT_NORMAL_TEXCOORD, material.normal.texCoord);
	material.normal.texUnit = static_cast<uint8_t>(d.getUInt(Consts::ATTR_PBR_MAT_NORMAL_TEXUNIT, material.normal.texUnit));
	material.occlusion.strength = d.getFloat(Consts::ATTR_PBR_MAT_OCCLUSION_STRENGTH, material.occlusion.strength);
	material.occlusion.texTransform = deserializeMatrix(d.getString(Consts::ATTR_PBR_MAT_OCCLUSION_TEXTRANSFORM));
	material.occlusion.texCoord = d.getUInt(Consts::ATTR_PBR_MAT_OCCLUSION_TEXCOORD, material.occlusion.texCoord);
	material.occlusion.texUnit = static_cast<uint8_t>(d.getUInt(Consts::ATTR_PBR_MAT_OCCLUSION_TEXUNIT, material.occlusion.texUnit));
	auto emValues = Util::StringUtils::toFloats(d.getString(Consts::ATTR_PBR_MAT_EMISSIVE_FACTOR));
	if(emValues.size() == 3)
		material.emissive.factor = Geometry::Vec3(emValues.data());
	material.emissive.texTransform = deserializeMatrix(d.getString(Consts::ATTR_PBR_MAT_EMISSIVE_TEXTRANSFORM));
	material.emissive.texCoord = d.getUInt(Consts::ATTR_PBR_MAT_EMISSIVE_TEXCOORD, material.emissive.texCoord);
	material.emissive.texUnit = static_cast<uint8_t>(d.getUInt(Consts::ATTR_PBR_MAT_EMISSIVE_TEXUNIT, material.emissive.texUnit));
	material.alphaMode = static_cast<PbrAlphaMode>(d.getInt(Consts::ATTR_PBR_MAT_ALPHAMODE, static_cast<int32_t>(material.alphaMode)));
	material.alphaCutoff = d.getFloat(Consts::ATTR_PBR_MAT_ALPHACUTOFF, material.alphaCutoff);
	material.ior = d.getFloat(Consts::ATTR_PBR_MAT_IOR, material.ior);
	material.doubleSided = d.getBool(Consts::ATTR_PBR_MAT_DOUBLESIDED, material.doubleSided);
	material.useSkinning = d.getBool(Consts::ATTR_PBR_MAT_SKINNING, material.useSkinning);
	material.useIBL = d.getBool(Consts::ATTR_PBR_MAT_IBL, material.useIBL);
	material.receiveShadow = d.getBool(Consts::ATTR_PBR_MAT_SHADOW, material.receiveShadow);
	material.shadingModel = static_cast<PbrShadingModel>(d.getInt(Consts::ATTR_PBR_MAT_SHADINGMODEL, static_cast<int32_t>(material.alphaMode)));

	const auto dataDescList = ImporterTools::filterElements(Consts::TYPE_DATA, dynamic_cast<const DescriptionArray *>(d.getValue(Consts::CHILDREN)));
	for(auto dataDesc : dataDescList) {
		auto imageId = Util::StringIdentifier(dataDesc->getString(Consts::ATTR_TEXTURE_ID));
		if(imageId == Consts::ATTR_PBR_MAT_BASECOLOR_TEXTURE) {
			material.baseColor.texture = importTextureFromData(ctxt, dataDesc);
		} else if(imageId == Consts::ATTR_PBR_MAT_METALLIC_ROUGHNESS_TEXTURE) {
			material.metallicRoughness.texture = importTextureFromData(ctxt, dataDesc);
		} else if(imageId == Consts::ATTR_PBR_MAT_NORMAL_TEXTURE) {
			material.normal.texture = importTextureFromData(ctxt, dataDesc);
		} else if(imageId == Consts::ATTR_PBR_MAT_OCCLUSION_TEXTURE) {
			material.occlusion.texture = importTextureFromData(ctxt, dataDesc);
		} else if(imageId == Consts::ATTR_PBR_MAT_EMISSIVE_TEXTURE) {
			material.emissive.texture = importTextureFromData(ctxt, dataDesc);
		}
	}

	std::unique_ptr<PbrMaterialState> state(new PbrMaterialState);
	state->setMaterial(material);
	state->setSearchPaths(ctxt.fileLocator.getSearchPaths());
	ImporterTools::finalizeState(ctxt, state.get(), d);
	parent->addState(state.release());
	return true;
}

//! template for new importers
// static bool importXY(ImportContext & ctxt, const std::string & stateType, const DescriptionMap & d, Node * parent) {
//  if(stateType != Consts::STATE_TYPE_XY) // check parent != nullptr is done by SceneManager
//      return false;
//
//  XY * state = new XY;
//
//  //TODO
//
//
//  ImporterTools::finalizeState(ctxt, state, d);
//  parent->addState(state);
//  return true;
// }

void initExtStateImporter() {

	ImporterTools::registerStateImporter(&importLODRenderer);
	ImporterTools::registerStateImporter(&importCHCppRenderer);
	ImporterTools::registerStateImporter(&importOccRenderer);
	ImporterTools::registerStateImporter(&importBudgetAnnotationState);
	ImporterTools::registerStateImporter(&importMirrorState);
	ImporterTools::registerStateImporter(&importProjSizeFilterState);
	ImporterTools::registerStateImporter(&importSkyboxState);
	ImporterTools::registerStateImporter(&importShadowState);
	ImporterTools::registerStateImporter(&importSkinningState);
	ImporterTools::registerStateImporter(&importIBLEnvState);
	ImporterTools::registerStateImporter(&importPbrMaterialState);

#ifdef MINSG_EXT_BLUE_SURFELS
	ImporterTools::registerStateImporter(&importSurfelRenderer);
#endif

#ifdef MINSG_EXT_COLORCUBES
	ImporterTools::registerStateImporter(&importColorCubeRenderer);
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	ImporterTools::registerStateImporter(&importSkeletalHardwareRendererState);
#endif

#ifdef MINSG_EXT_MULTIALGORENDERING
	ImporterTools::registerStateImporter(&importAlgoSelector);
	ImporterTools::registerStateImporter(&importMARSurfelRenderer);
#endif

#ifdef MINSG_EXT_SVS
	ImporterTools::registerStateImporter(&importSVSRenderer);
	ImporterTools::registerStateImporter(&importSVSBudgetRenderer);
#endif
}

}
}
