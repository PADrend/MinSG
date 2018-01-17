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

#ifdef MINSG_EXT_BLUE_SURFELS
#include "../../BlueSurfels/SurfelRenderer.h"
#include "../../BlueSurfels/SurfelRenderer_FixedSize.h"
#include "../../BlueSurfels/SurfelRenderer_Budget.h"
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
	auto renderer = dynamic_cast<BlueSurfels::SurfelRenderer *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SURFEL_RENDERER);
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_COUNT_FACTOR, Util::GenericAttribute::createNumber(renderer->getCountFactor()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_MAX_SIZE, Util::GenericAttribute::createNumber(renderer->getMaxSideLength()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_MIN_SIZE, Util::GenericAttribute::createNumber(renderer->getMinSideLength()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_SIZE_FACTOR, Util::GenericAttribute::createNumber(renderer->getSizeFactor()));
}
static void describeSurfelRendererFixedSize(ExporterContext &,DescriptionMap & desc,State * state) {
	auto renderer = dynamic_cast<BlueSurfels::SurfelRendererFixedSize *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SURFEL_RENDERER_FIXED_SIZE);
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_COUNT_FACTOR, Util::GenericAttribute::createNumber(renderer->getCountFactor()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_SIZE_FACTOR, Util::GenericAttribute::createNumber(renderer->getSizeFactor()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_SURFEL_SIZE, Util::GenericAttribute::createNumber(renderer->getSurfelSize()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_MAX_SURFEL_SIZE, Util::GenericAttribute::createNumber(renderer->getMaxSurfelSize()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_MAX_TIME, Util::GenericAttribute::createNumber(renderer->getMaxFrameTime()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_ADAPTIVE, Util::GenericAttribute::createBool(renderer->isAdaptive()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_FOVEATED, Util::GenericAttribute::createBool(renderer->isFoveated()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_BLENDFACTOR, Util::GenericAttribute::createNumber(renderer->getBlendFactor()));
	std::ostringstream zoneStream;
	auto zones = renderer->getFoveatZones();
	auto it = zones.begin();
	if(it != zones.end()) {
		zoneStream << it->first << " " << it->second;
		for(++it; it!=zones.end(); ++it)
			zoneStream << " " << it->first << " " << it->second;
	}
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_FOVEAT_ZONES, Util::GenericAttribute::createString(zoneStream.str()));
}
static void describeSurfelRendererBudget(ExporterContext &,DescriptionMap & desc,State * state) {
	auto renderer = dynamic_cast<BlueSurfels::SurfelRendererBudget *>(state);
	desc.setString(Consts::ATTR_STATE_TYPE, Consts::STATE_TYPE_SURFEL_RENDERER_BUDGET);
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_MAX_SURFEL_SIZE, Util::GenericAttribute::createNumber(renderer->getMaxSurfelSize()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_BUDGET, Util::GenericAttribute::createNumber(renderer->getBudget()));
	desc.setValue(Consts::ATTR_SURFEL_RENDERER_MAX_INCR, Util::GenericAttribute::createNumber(renderer->getMaxIncrement()));
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

void initExtStateExporter() {
	ExporterTools::registerStateExporter(OccRenderer::getClassId(),&describeOccRenderer);
	ExporterTools::registerStateExporter(CHCppRenderer::getClassId(),&describeCHCppRenderer);
	ExporterTools::registerStateExporter(BudgetAnnotationState::getClassId(),&describeBudgetAnnotationState);
	ExporterTools::registerStateExporter(MirrorState::getClassId(),&describeMirrorState);
	ExporterTools::registerStateExporter(ProjSizeFilterState::getClassId(),&describeProjSizeFilterState);
	ExporterTools::registerStateExporter(MinSG::LODRenderer::getClassId(),&describeLODRenderer);
	ExporterTools::registerStateExporter(ShadowState::getClassId(),&describeShadowState);

#ifdef MINSG_EXT_BLUE_SURFELS
	ExporterTools::registerStateExporter(BlueSurfels::SurfelRenderer::getClassId(),&describeSurfelRenderer);
	ExporterTools::registerStateExporter(BlueSurfels::SurfelRendererFixedSize::getClassId(),&describeSurfelRendererFixedSize);
	ExporterTools::registerStateExporter(BlueSurfels::SurfelRendererBudget::getClassId(),&describeSurfelRendererBudget);
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
