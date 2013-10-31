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
#include "../../../SceneManagement/Importer/ImportContext.h"
#include "../../../SceneManagement/Importer/ImporterTools.h"

#include "../../../Core/Nodes/GroupNode.h"
#include "../../../Core/Nodes/ListNode.h"
#include "../../../Core/Nodes/Node.h"

#include "../../OcclusionCulling/OccRenderer.h"
#include "../../OcclusionCulling/CHCppRenderer.h"
#include "../../States/BudgetAnnotationState.h"
#include "../../States/MirrorState.h"
#include "../../States/ProjSizeFilterState.h"
#include "../../States/SkyboxState.h"
#include "../../States/LODRenderer.h"

#ifdef MINSG_EXT_COLORCUBES
#include "../../ColorCubes/ColorCubeRenderer.h"
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#include "../../SkeletalAnimation/Renderer/SkeletalSoftwareRendererState.h"
#include "../../SkeletalAnimation/Renderer/SkeletalHardwareRendererState.h"
#include "../../SkeletalAnimation/SkeletalNode.h"
#endif

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "../../MultiAlgoRendering/AlgoSelector.h"
#include "../../MultiAlgoRendering/SampleContext.h"
#include "../../MultiAlgoRendering/SampleStorage.h"
#include "../../MultiAlgoRendering/SurfelRenderer.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#endif

#ifdef MINSG_EXT_SPHERICALSAMPLING
#include "../../SphericalSampling/BudgetRenderer.h"
#include "../../SphericalSampling/Helper.h"
#include "../../SphericalSampling/Renderer.h"
#endif

#include <Rendering/Shader/Uniform.h>

#include <Util/GenericAttribute.h>
#include <Util/StringIdentifier.h>
#include <Util/Macros.h>

#include <cstddef>
#include <deque>
#include <list>
#include <functional>
#include <cassert>

namespace MinSG {
namespace SceneManagement {

template < typename T >
static T * convertToTNode(Node * node) {
	T * t = dynamic_cast<T *>(node);
	if(t == nullptr)
		WARN(std::string(node != nullptr ? node->getTypeName() : "nullptr") + " can not be casted to " + T::getClassName());
	return t;
}

#ifdef MINSG_EXT_MULTIALGORENDERING

static void finalizeAlgoSelector(ImportContext & ctxt, MAR::AlgoSelector * as){
	as->getSampleContext()->getSampleStorage()->initNodeIndices(ctxt.getRootNode());
}

static  bool importAlgoSelector(ImportContext & ctxt, const std::string & type, const NodeDescription & d, Node * parent) {
	if(type != Consts::STATE_TYPE_ALGOSELECTOR) // check parent != nullptr is done by SceneManager
		return false;

	Util::FileName file(ctxt.fileName);
	file.setEnding(".mar");
	std::unique_ptr<std::istream> in(Util::FileUtils::openForReading(file));

	MAR::AlgoSelector * as;
	if(in.get() != nullptr)
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

static  bool importMARSurfelRenderer(ImportContext & ctxt, const std::string & type, const NodeDescription & d, Node * parent) {
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
static bool importColorCubeRenderer(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_COLOR_CUBE_RENDERER) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new ColorCubeRenderer;
	state->setHighlightEnabled(d.getBool(Consts::ATTR_COLOR_CUBE_RENDERER_HIGHLIGHT, false));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}
#endif

static bool importOccRenderer(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_OCC_RENDERER) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new OccRenderer;

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importCHCppRenderer(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
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

static bool importBudgetAnnotationState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & desc, Node * parent) {
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

static bool importMirrorState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_MIRROR_STATE) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new MirrorState(d.getUInt(Consts::ATTR_MIRROR_TEXTURE_SIZE));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

static bool importProjSizeFilterState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
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

static bool importSkyboxState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SKYBOX) // check parent != nullptr is done by SceneManager
		return false;

	static const Util::StringIdentifier ATTR_SKYBOX_FILE("file");
	SkyboxState * state = SkyboxState::createSkybox(d.getString(ATTR_SKYBOX_FILE));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

#ifdef MINSG_EXT_SKELETAL_ANIMATION
static bool importSkeletalHardwareRendererState(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
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

#ifdef MINSG_EXT_SPHERICALSAMPLING
static bool importSVSRenderer(ImportContext & ctxt, const std::string & stateType, const NodeDescription & desc, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SVS_RENDERER) { // check parent != nullptr is done by SceneManager
		return false;
	}

	auto renderer = new SphericalSampling::Renderer;
	if(desc.contains(Consts::ATTR_SVS_INTERPOLATION_METHOD)) {
		renderer->setInterpolationMethod(SphericalSampling::interpolationFromString(desc.getString(Consts::ATTR_SVS_INTERPOLATION_METHOD)));
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

static bool importSVSBudgetRenderer(ImportContext & ctxt, const std::string & stateType, const NodeDescription & desc, Node * parent) {
	if(stateType != Consts::STATE_TYPE_SVS_BUDGETRENDERER) { // check parent != nullptr is done by SceneManager
		return false;
	}

	auto renderer = new SphericalSampling::BudgetRenderer;
	if(desc.contains(Consts::ATTR_SVS_BUDGETRENDERER_BUDGET)) {
		renderer->setBudget(desc.getValue(Consts::ATTR_SVS_BUDGETRENDERER_BUDGET)->toUnsignedInt());
	}

	ImporterTools::finalizeState(ctxt, renderer, desc);
	parent->addState(renderer);
	return true;
}
#endif

static bool importLODRenderer(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
	if(stateType != Consts::STATE_TYPE_LOD_RENDERER) // check parent != nullptr is done by SceneManager
		return false;

	auto state = new LODRenderer();

	state->setMaxComplexity(d.getUInt(Consts::ATTR_LOD_RENDERER_MAX_COMPLEXITY));
	state->setMinComplexity(d.getUInt(Consts::ATTR_LOD_RENDERER_MIN_COMPLEXITY));
	state->setRelComplexity(d.getUInt(Consts::ATTR_LOD_RENDERER_REL_COMPLEXITY));
	state->setSourceChannel(d.getString(Consts::ATTR_LOD_RENDERER_SOURCE_CHANNEL));

	ImporterTools::finalizeState(ctxt, state, d);
	parent->addState(state);
	return true;
}

//! template for new importers
// static bool importXY(ImportContext & ctxt, const std::string & stateType, const NodeDescription & d, Node * parent) {
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

void initExtStateImporter(SceneManager & sm) {

	sm.addStateImporter(&importLODRenderer);
	sm.addStateImporter(&importCHCppRenderer);
	sm.addStateImporter(&importOccRenderer);
	sm.addStateImporter(&importBudgetAnnotationState);
	sm.addStateImporter(&importMirrorState);
	sm.addStateImporter(&importProjSizeFilterState);
	sm.addStateImporter(&importSkyboxState);

#ifdef MINSG_EXT_COLORCUBES
	sm.addStateImporter(&importColorCubeRenderer);
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	sm.addStateImporter(&importSkeletalHardwareRendererState);
#endif

#ifdef MINSG_EXT_MULTIALGORENDERING
	sm.addStateImporter(&importAlgoSelector);
	sm.addStateImporter(&importMARSurfelRenderer);
#endif

#ifdef MINSG_EXT_SPHERICALSAMPLING
	sm.addStateImporter(&importSVSRenderer);
	sm.addStateImporter(&importSVSBudgetRenderer);
#endif
}

}
}
