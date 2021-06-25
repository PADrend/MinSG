/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtStateExporter.h"

#include "../ExtConsts.h"

#include "../../../Core/Nodes/LightNode.h"

#include "../../../SceneManagement/SceneDescription.h"
#include "../../../SceneManagement/SceneManager.h"
#include "../../../SceneManagement/Exporter/ExporterTools.h"

#include "../../OcclusionCulling/OccRenderer.h"
#include "../../OcclusionCulling/CHCppRenderer.h"
#include "../../States/BudgetAnnotationState.h"
#include "../../States/MirrorState.h"
#include "../../States/ProjSizeFilterState.h"
#include "../../States/LODRenderer.h"
#include "../../States/ShadowState.h"
#include "../../States/SkinningState.h"
#include "../../States/IBLEnvironmentState.h"
#include "../../States/PbrMaterialState.h"


#ifdef MINSG_EXT_BLUE_SURFELS
#include "../../BlueSurfels/SurfelRenderer.h"
#include "../../BlueSurfels/Strategies/AbstractSurfelStrategy.h"
#endif // MINSG_EXT_BLUE_SURFELS

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#include "../../SkeletalAnimation/Renderer/SkeletalHardwareRendererState.h"
#include "../../SkeletalAnimation/Renderer/SkeletalSoftwareRendererState.h"
#endif

#ifdef MINSG_EXT_COLORCUBES
#include "../../ColorCubes/ColorCubeRenderer.h"
#endif

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "../../MultiAlgoRendering/AlgoSelector.h"
#include "../../MultiAlgoRendering/SurfelRenderer.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#endif

#ifdef MINSG_EXT_SVS
#include "../../SVS/BudgetRenderer.h"
#include "../../SVS/Helper.h"
#include "../../SVS/Renderer.h"
#endif

#include <Rendering/Texture/TextureUtils.h>
#include <Util/Serialization/Serialization.h>
#include <Util/Encoding.h>
#include <Util/Graphics/Bitmap.h>

#include <cassert>

namespace MinSG {
namespace SceneManagement {

static void describeOccRenderer(ExporterContext &,DescriptionMap & desc,State *) {
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_OCC_RENDERER);
}

static void describeCHCppRenderer(ExporterContext &,DescriptionMap & desc,State * state) {
	auto chcpp = dynamic_cast<CHCppRenderer *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_CHCPP_RENDERER);
	desc.setValue(Consts::ATTR_CHCPP_VISIBILITYTHRESHOLD, Util::GenericAttribute::createNumber<uint16_t>(chcpp->getVisibilityThreshold()));
	desc.setValue(Consts::ATTR_CHCPP_MAXPREVINVISNODESBATCHSIZE, Util::GenericAttribute::createNumber<uint16_t>(chcpp->getMaxPrevInvisNodesBatchSize()));
	desc.setValue(Consts::ATTR_CHCPP_SKIPPEDFRAMESTILLQUERY, Util::GenericAttribute::createNumber<uint16_t>(chcpp->getSkippedFramesTillQuery()));
	desc.setValue(Consts::ATTR_CHCPP_MAXDEPTHFORTIGHTBOUNDINGVOLUMES, Util::GenericAttribute::createNumber<uint16_t>(chcpp->getMaxDepthForTightBoundingVolumes()));
	desc.setValue(Consts::ATTR_CHCPP_MAXAREADERIVATIONFORTIGHTBOUNDINGVOLUMES, Util::GenericAttribute::createNumber<float>(chcpp->getMaxAreaDerivationForTightBoundingVolumes()));
}

static void describeBudgetAnnotationState(ExporterContext &,DescriptionMap & desc,State * state) {
	auto bas = dynamic_cast<BudgetAnnotationState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_BUDGET_ANNOTATION_STATE);
	desc.setValue(Consts::ATTR_BAS_ANNOTATION_ATTRIBUTE, Util::GenericAttribute::create(bas->getAnnotationAttribute().toString()));
	desc.setValue(Consts::ATTR_BAS_BUDGET, Util::GenericAttribute::create(bas->getBudget()));
	desc.setValue(Consts::ATTR_BAS_DISTRIBUTION_TYPE, Util::GenericAttribute::create(BudgetAnnotationState::distributionTypeToString(bas->getDistributionType())));
}

static void describeMirrorState(ExporterContext &,DescriptionMap & desc,State * state) {
	auto ms = dynamic_cast<MirrorState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_MIRROR_STATE);
	desc.setValue(Consts::ATTR_MIRROR_TEXTURE_SIZE, Util::GenericAttribute::createNumber<uint16_t>(ms->getTextureSize()));
}

static void describeProjSizeFilterState(ExporterContext &,DescriptionMap & desc,State * state) {
	auto psfs = dynamic_cast<ProjSizeFilterState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_PROJ_SIZE_FILTER_STATE);
	desc.setValue(Consts::ATTR_PSFS_MAXIMUM_PROJECTED_SIZE, Util::GenericAttribute::createNumber(psfs->getMaximumProjSize()));
	desc.setValue(Consts::ATTR_PSFS_MINIMUM_DISTANCE, Util::GenericAttribute::createNumber(psfs->getMinimumDistance()));
	desc.setValue(Consts::ATTR_PSFS_SOURCE_CHANNEL, Util::GenericAttribute::createString(psfs->getSourceChannel().toString()));
	desc.setValue(Consts::ATTR_PSFS_TARGET_CHANNEL, Util::GenericAttribute::createString(psfs->getTargetChannel().toString()));
	desc.setValue(Consts::ATTR_PSFS_FORCE_CLOSED_NODES, Util::GenericAttribute::createBool(psfs->isForceClosed()));
}

static void describeShadowState(ExporterContext & ctx,DescriptionMap & desc,State * state) {
	auto ss = dynamic_cast<ShadowState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SHADOW_STATE);
	desc.setValue(Consts::ATTR_SHADOW_LIGHT_NODE, Util::GenericAttribute::createString(ctx.sceneManager.getNodeId(ss->getLight()).toString()));
	desc.setValue(Consts::ATTR_SHADOW_TEXTURE_SIZE, Util::GenericAttribute::createNumber<uint16_t>(ss->getTextureSize()));
	desc.setValue(Consts::ATTR_SHADOW_STATIC, Util::GenericAttribute::createBool(ss->isStatic()));
}

#ifdef MINSG_EXT_BLUE_SURFELS
static void describeSurfelRenderer(ExporterContext &,DescriptionMap & desc,State * state) {	
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SURFEL_RENDERER);
  for(auto strategy : dynamic_cast<BlueSurfels::SurfelRenderer*>(state)->getSurfelStrategies()) {
    ExporterTools::addChildEntry(desc, BlueSurfels::exportStrategy(strategy));
  }
}
#endif // MINSG_EXT_BLUE_SURFELS

#ifdef MINSG_EXT_COLORCUBES
static void describeColorCubeRenderer(ExporterContext &,DescriptionMap & desc,State * state) {
	auto cr = dynamic_cast<ColorCubeRenderer*>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_COLOR_CUBE_RENDERER);
	desc.setString(Consts::ATTR_COLOR_CUBE_RENDERER_HIGHLIGHT, Util::StringUtils::toString(cr->isHighlightEnabled()));
}
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
static void describeSkeletalRendererState(ExporterContext &,DescriptionMap & desc,State * state) {
	auto srs = dynamic_cast<SkeletalAbstractRendererState *>(state);
	std::stringstream ss;
	ss << srs->getBindMatrix();
	desc.setString(Consts::ATTR_SKEL_BINDMATRIX, ss.str());
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SKEL_SKELETALRENDERERSTATE);
}
#endif

#ifdef MINSG_EXT_MULTIALGORENDERING
static void describeAlgoSelector(ExporterContext & ctxt,DescriptionMap & desc,State * state) {
	auto as = dynamic_cast<MAR::AlgoSelector*>(state);
	assert(as);
	
	desc.setValue(Consts::TYPE, Util::GenericAttribute::createString(Consts::TYPE_STATE));
	desc.setValue(Consts::ATTR_STATE_TYPE,Util::GenericAttribute::createString(Consts::STATE_TYPE_ALGOSELECTOR));

	Util::FileName file(ctxt.sceneFile);
	file.setEnding(".mar");
	auto out = Util::FileUtils::openForWriting(file);
	as->write(*(out.get()));
}
static void describeMARSurfelRenderer(ExporterContext &,DescriptionMap & desc,State * state) {
	auto sr = dynamic_cast<MAR::SurfelRenderer*>(state);
	assert(sr);
	
	desc.setValue(Consts::TYPE, Util::GenericAttribute::createString(Consts::TYPE_STATE));
	desc.setValue(Consts::ATTR_STATE_TYPE,Util::GenericAttribute::createString(Consts::STATE_TYPE_MAR_SURFEL_RENDERER));
	desc.setValue(Consts::ATTR_MAR_SURFEL_COUNT_FACTOR, Util::GenericAttribute::createNumber<float>(sr->getSurfelCountFactor()));
	desc.setValue(Consts::ATTR_MAR_SURFEL_SIZE_FACTOR, Util::GenericAttribute::createNumber<float>(sr->getSurfelSizeFactor()));
	desc.setValue(Consts::ATTR_MAR_SURFEL_MAX_AUTO_SIZE, Util::GenericAttribute::createNumber<float>(sr->getMaxAutoSurfelSize()));
	desc.setValue(Consts::ATTR_MAR_SURFEL_FORCE_FLAG, Util::GenericAttribute::createBool(sr->getForceSurfels()));
}
#endif

#ifdef MINSG_EXT_SVS
static void describeSVSRenderer(ExporterContext &, DescriptionMap & desc, State * state) {
	auto renderer = dynamic_cast<SVS::Renderer *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SVS_RENDERER);
	desc.setString(Consts::ATTR_SVS_INTERPOLATION_METHOD, SVS::interpolationToString(renderer->getInterpolationMethod()));
	desc.setValue(Consts::ATTR_SVS_RENDERER_SPHERE_OCCLUSION_TEST, Util::GenericAttribute::createBool(renderer->isSphereOcclusionTestEnabled()));
	desc.setValue(Consts::ATTR_SVS_RENDERER_GEOMETRY_OCCLUSION_TEST, Util::GenericAttribute::createBool(renderer->isGeometryOcclusionTestEnabled()));
}

static void describeSVSBudgetRenderer(ExporterContext &, DescriptionMap & desc, State * state) {
	auto renderer = dynamic_cast<SVS::BudgetRenderer *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SVS_BUDGETRENDERER);
	desc.setValue(Consts::ATTR_SVS_BUDGETRENDERER_BUDGET, Util::GenericAttribute::createNumber<uint32_t>(renderer->getBudget()));
}
#endif

static void describeLODRenderer(ExporterContext &,DescriptionMap & desc,State * state) {
	auto renderer = dynamic_cast<LODRenderer *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_LOD_RENDERER);
	desc.setValue(Consts::ATTR_LOD_RENDERER_MAX_COMPLEXITY, Util::GenericAttribute::createNumber(renderer->getMaxComplexity()));
	desc.setValue(Consts::ATTR_LOD_RENDERER_MIN_COMPLEXITY, Util::GenericAttribute::createNumber(renderer->getMinComplexity()));
	desc.setValue(Consts::ATTR_LOD_RENDERER_REL_COMPLEXITY, Util::GenericAttribute::createNumber(renderer->getRelComplexity()));
	desc.setValue(Consts::ATTR_LOD_RENDERER_SOURCE_CHANNEL, Util::GenericAttribute::createString(renderer->getSourceChannel().toString()));
}

static void describeSkinningState(ExporterContext & ctx,DescriptionMap & desc,State * state) {
	auto skin = dynamic_cast<SkinningState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SKINNING_STATE);
	
	const auto& joints = skin->getJoints();
	const auto& inverseBindMatrices = skin->getInverseBindMatrices();

	for(size_t i=0; i<joints.size(); ++i) {
		auto jointId = ctx.sceneManager.getNodeId(joints[i].get());
		std::unique_ptr<DescriptionMap> joint(new DescriptionMap);
		joint->setString(Consts::TYPE, Consts::TYPE_SKINNING_JOINT);
		joint->setString(Consts::ATTR_SKINNING_JOINT_ID, jointId.toString());
		std::stringstream ss;
		ss << inverseBindMatrices[i];
		joint->setString(Consts::ATTR_SKINNING_INVERSE_BINDING_MATRIX, ss.str());
		ExporterTools::addChildEntry(desc, std::move(joint));
	}
}

static void exportIBLEnvState(ExporterContext &,DescriptionMap & desc,State * state) {
	auto envState = dynamic_cast<IBLEnvironmentState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_IBL_ENV_STATE);
	desc.setValue(Consts::ATTR_IBL_ENV_LOD, Util::GenericAttribute::createNumber(envState->getLOD()));
	desc.setValue(Consts::ATTR_IBL_ENV_DRAW_ENV, Util::GenericAttribute::createBool(envState->isDrawEnvironmentEnabled()));
	
	const Util::FileName hdrFilename(envState->getHdrFile());
	if(!hdrFilename.empty()) {
		desc.setString(Consts::ATTR_IBL_ENV_FILE, hdrFilename.toShortString());
	} else {
		Rendering::Texture * texture = envState->getEnvironmentMap();
		if( texture ) {
			std::unique_ptr<DescriptionMap> dataDesc(new DescriptionMap);
			dataDesc->setString(Consts::ATTR_DATA_TYPE,"image");

			const Util::FileName texFilename(texture->getFileName());
			if(texFilename.empty()) {
				auto bitmap = Rendering::TextureUtils::createBitmapFromLocalTexture(*texture);

				std::ostringstream stream;
				if(bitmap && Util::Serialization::saveBitmap(*bitmap.get(), "hdr", stream)) {
					const std::string streamString = stream.str();
					const std::string encodedData = Util::encodeBase64(std::vector<uint8_t>(streamString.begin(), streamString.end()));

					dataDesc->setString(Consts::ATTR_DATA_ENCODING,"base64");
					dataDesc->setString(Consts::ATTR_DATA_FORMAT,"png");
					dataDesc->setString(Consts::DATA_BLOCK,encodedData);
				}else{
					WARN("Texture has no local data."); //... and downloading here is not possible as there is no renderingContext.
				}
			} else {
				dataDesc->setString(Consts::ATTR_TEXTURE_FILENAME, texFilename.toShortString());
			}
			ExporterTools::addDataEntry(desc, std::move(dataDesc));
		}
	}
}

static void exportTextureData(DescriptionMap& desc, Rendering::Texture* texture, const Util::StringIdentifier& id) {
	if( texture ) {
		std::unique_ptr<DescriptionMap> dataDesc(new DescriptionMap);
		dataDesc->setString(Consts::ATTR_DATA_TYPE,"image");
		if(!id.empty())
			dataDesc->setString(Consts::ATTR_TEXTURE_ID, id.toString());
		if( texture->getNumLayers()!=1 )
			dataDesc->setString(Consts::ATTR_TEXTURE_NUM_LAYERS, Util::StringUtils::toString(texture->getNumLayers()));
		if( texture->getTextureType()!=Rendering::TextureType::TEXTURE_2D )
			dataDesc->setString(Consts::ATTR_TEXTURE_TYPE, Util::StringUtils::toString( static_cast<uint32_t>(texture->getTextureType())));

		const Util::FileName texFilename(texture->getFileName());
		if(texFilename.empty()) {
			auto bitmap = Rendering::TextureUtils::createBitmapFromLocalTexture(*texture);

			std::ostringstream stream;
			if(bitmap && Util::Serialization::saveBitmap(*bitmap.get(), "png", stream)) {
				const std::string streamString = stream.str();
				const std::string encodedData = Util::encodeBase64(std::vector<uint8_t>(streamString.begin(), streamString.end()));

				dataDesc->setString(Consts::ATTR_DATA_ENCODING,"base64");
				dataDesc->setString(Consts::ATTR_DATA_FORMAT,"png");
				dataDesc->setString(Consts::DATA_BLOCK,encodedData);
			}else{
				WARN("Texture has no local data."); //... and downloading here is not possible as there is no renderingContext.
			}
		} else {
//			// make path to texture relative to scene (if mesh lies below the scene)
//			Util::FileUtils::makeRelativeIfPossible(ctxt.sceneFile, texFilename);
			dataDesc->setString(Consts::ATTR_TEXTURE_FILENAME, texFilename.toShortString());
		}
		ExporterTools::addDataEntry(desc, std::move(dataDesc));
	}
}

static void exportPbrMaterialState(ExporterContext &,DescriptionMap & desc,State * state) {
	auto envState = dynamic_cast<PbrMaterialState *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_PBR_MATERIAL_STATE);
	const auto& material = envState->getMaterial();

	std::stringstream colStr;
	colStr << material.baseColor.factor;
	desc.setValue(Consts::ATTR_PBR_MAT_BASECOLOR_FACTOR, Util::GenericAttribute::createString(colStr.str())); 
	desc.setValue(Consts::ATTR_PBR_MAT_BASECOLOR_TEXCOORD, Util::GenericAttribute::createNumber(material.baseColor.texCoord)); 
	desc.setValue(Consts::ATTR_PBR_MAT_BASECOLOR_TEXUNIT, Util::GenericAttribute::createNumber(material.baseColor.texUnit)); 
	desc.setValue(Consts::ATTR_PBR_MAT_METALLICFACTOR, Util::GenericAttribute::createNumber(material.metallicRoughness.metallicFactor)); 
	desc.setValue(Consts::ATTR_PBR_MAT_ROUGHNESSFACTOR, Util::GenericAttribute::createNumber(material.metallicRoughness.roughnessFactor)); 
	desc.setValue(Consts::ATTR_PBR_MAT_METALLIC_ROUGHNESS_TEXCOORD, Util::GenericAttribute::createNumber(material.metallicRoughness.texCoord)); 
	desc.setValue(Consts::ATTR_PBR_MAT_METALLIC_ROUGHNESS_TEXUNIT, Util::GenericAttribute::createNumber(material.metallicRoughness.texUnit)); 
	desc.setValue(Consts::ATTR_PBR_MAT_NORMAL_SCALE, Util::GenericAttribute::createNumber(material.normal.scale)); 
	desc.setValue(Consts::ATTR_PBR_MAT_NORMAL_TEXCOORD, Util::GenericAttribute::createNumber(material.normal.texCoord)); 
	desc.setValue(Consts::ATTR_PBR_MAT_NORMAL_TEXUNIT, Util::GenericAttribute::createNumber(material.normal.texUnit)); 
	desc.setValue(Consts::ATTR_PBR_MAT_OCCLUSION_STRENGTH, Util::GenericAttribute::createNumber(material.occlusion.strength)); 
	desc.setValue(Consts::ATTR_PBR_MAT_OCCLUSION_TEXCOORD, Util::GenericAttribute::createNumber(material.occlusion.texCoord)); 
	desc.setValue(Consts::ATTR_PBR_MAT_OCCLUSION_TEXUNIT, Util::GenericAttribute::createNumber(material.occlusion.texUnit));
	std::stringstream emStr;
	emStr << material.emissive.factor;
	desc.setValue(Consts::ATTR_PBR_MAT_EMISSIVE_FACTOR, Util::GenericAttribute::createString(emStr.str())); 
	desc.setValue(Consts::ATTR_PBR_MAT_EMISSIVE_TEXCOORD, Util::GenericAttribute::createNumber(material.emissive.texCoord)); 
	desc.setValue(Consts::ATTR_PBR_MAT_EMISSIVE_TEXUNIT, Util::GenericAttribute::createNumber(material.emissive.texUnit)); 
	desc.setValue(Consts::ATTR_PBR_MAT_ALPHAMODE, Util::GenericAttribute::createNumber(static_cast<int32_t>(material.alphaMode))); 
	desc.setValue(Consts::ATTR_PBR_MAT_ALPHACUTOFF, Util::GenericAttribute::createNumber(material.alphaCutoff)); 
	desc.setValue(Consts::ATTR_PBR_MAT_DOUBLESIDED, Util::GenericAttribute::createBool(material.doubleSided)); 
	desc.setValue(Consts::ATTR_PBR_MAT_SKINNING, Util::GenericAttribute::createBool(material.useSkinning)); 
	desc.setValue(Consts::ATTR_PBR_MAT_IBL, Util::GenericAttribute::createBool(material.useIBL)); 
	desc.setValue(Consts::ATTR_PBR_MAT_SHADOW, Util::GenericAttribute::createBool(material.receiveShadow)); 
	desc.setValue(Consts::ATTR_PBR_MAT_SHADINGMODEL, Util::GenericAttribute::createNumber(static_cast<int32_t>(material.shadingModel))); 
	
	exportTextureData(desc, material.baseColor.texture.get(), Consts::ATTR_PBR_MAT_BASECOLOR_TEXTURE);
	exportTextureData(desc, material.metallicRoughness.texture.get(), Consts::ATTR_PBR_MAT_METALLIC_ROUGHNESS_TEXTURE);
	exportTextureData(desc, material.normal.texture.get(), Consts::ATTR_PBR_MAT_NORMAL_TEXTURE);
	exportTextureData(desc, material.occlusion.texture.get(), Consts::ATTR_PBR_MAT_OCCLUSION_TEXTURE);
	exportTextureData(desc, material.emissive.texture.get(), Consts::ATTR_PBR_MAT_EMISSIVE_TEXTURE);
}

void initExtStateExporter() {
	ExporterTools::registerStateExporter(OccRenderer::getClassId(),&describeOccRenderer);
	ExporterTools::registerStateExporter(CHCppRenderer::getClassId(),&describeCHCppRenderer);
	ExporterTools::registerStateExporter(BudgetAnnotationState::getClassId(),&describeBudgetAnnotationState);
	ExporterTools::registerStateExporter(MirrorState::getClassId(),&describeMirrorState);
	ExporterTools::registerStateExporter(ProjSizeFilterState::getClassId(),&describeProjSizeFilterState);
	ExporterTools::registerStateExporter(MinSG::LODRenderer::getClassId(),&describeLODRenderer);
	ExporterTools::registerStateExporter(ShadowState::getClassId(),&describeShadowState);
	ExporterTools::registerStateExporter(SkinningState::getClassId(),&describeSkinningState);
	ExporterTools::registerStateExporter(IBLEnvironmentState::getClassId(),&exportIBLEnvState);
	ExporterTools::registerStateExporter(PbrMaterialState::getClassId(),&exportPbrMaterialState);

#ifdef MINSG_EXT_BLUE_SURFELS
	ExporterTools::registerStateExporter(BlueSurfels::SurfelRenderer::getClassId(),&describeSurfelRenderer);
#endif // MINSG_EXT_BLUE_SURFELS

#ifdef MINSG_EXT_MULTIALGORENDERING
	ExporterTools::registerStateExporter(MAR::AlgoSelector::getClassId(),&describeAlgoSelector);
	ExporterTools::registerStateExporter(MAR::SurfelRenderer::getClassId(),&describeMARSurfelRenderer);
#endif

#ifdef MINSG_EXT_COLORCUBES
	ExporterTools::registerStateExporter(ColorCubeRenderer::getClassId(),&describeColorCubeRenderer);
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	ExporterTools::registerStateExporter(SkeletalHardwareRendererState::getClassId(),&describeSkeletalRendererState);
	ExporterTools::registerStateExporter(SkeletalSoftwareRendererState::getClassId(),&describeSkeletalRendererState);
#endif

#ifdef MINSG_EXT_SVS
	ExporterTools::registerStateExporter(SVS::Renderer::getClassId(), &describeSVSRenderer);
	ExporterTools::registerStateExporter(SVS::BudgetRenderer::getClassId(), &describeSVSBudgetRenderer);
#endif

}

}
}
