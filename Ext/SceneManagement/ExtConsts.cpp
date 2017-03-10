/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtConsts.h"

namespace MinSG {
namespace SceneManagement {
namespace Consts {

cStr_t STATE_TYPE_OCC_RENDERER="occ_renderer";
cStr_t STATE_TYPE_SKYBOX="skyBox";

// ---------------------------------------------------------------------------
// Billboard
cStr_t NODE_TYPE_BILLBOARD("billboard");
const Util::StringIdentifier ATTR_BILLBOARD_RECT("rect");
const Util::StringIdentifier ATTR_BILLBOARD_ROTATE_UP("rotateUpAxis");
const Util::StringIdentifier ATTR_BILLBOARD_ROTATE_RIGHT("rotateRightAxis");

// ---------------------------------------------------------------------------
// Budget annotation state
cStr_t STATE_TYPE_BUDGET_ANNOTATION_STATE = "budget_annotation_state";
const Util::StringIdentifier ATTR_BAS_ANNOTATION_ATTRIBUTE("annotation_attribute");
const Util::StringIdentifier ATTR_BAS_BUDGET("budget");
const Util::StringIdentifier ATTR_BAS_DISTRIBUTION_TYPE("distribution_type");

// ---------------------------------------------------------------------------
// CHC++
cStr_t STATE_TYPE_CHCPP_RENDERER="chcpp_renderer";
const Util::StringIdentifier ATTR_CHCPP_VISIBILITYTHRESHOLD("visibility_threshold");
const Util::StringIdentifier ATTR_CHCPP_MAXPREVINVISNODESBATCHSIZE("max_prev_invis_nodes_batch_size");
const Util::StringIdentifier ATTR_CHCPP_SKIPPEDFRAMESTILLQUERY("skipped_frames_till_query");
const Util::StringIdentifier ATTR_CHCPP_MAXDEPTHFORTIGHTBOUNDINGVOLUMES("max_depth");
const Util::StringIdentifier ATTR_CHCPP_MAXAREADERIVATIONFORTIGHTBOUNDINGVOLUMES("max_area_derivation");

// ---------------------------------------------------------------------------
// Color cubes
cStr_t STATE_TYPE_COLOR_CUBE_RENDERER = "color_cube_renderer";
const Util::StringIdentifier ATTR_COLOR_CUBE_RENDERER_HIGHLIGHT("highlight");

// ---------------------------------------------------------------------------
// GenericMetaNode
cStr_t NODE_TYPE_GENERIC_META_NODE = "generic_meta_node";
const Util::StringIdentifier ATTR_GENERIC_META_NODE_BB("bb");

// ---------------------------------------------------------------------------
// Mirror state
cStr_t STATE_TYPE_MIRROR_STATE = "mirror";
const Util::StringIdentifier ATTR_MIRROR_TEXTURE_SIZE("texture_size");

// ---------------------------------------------------------------------------
// BlueSurfels
#ifdef MINSG_EXT_BLUE_SURFELS
cStr_t STATE_TYPE_SURFEL_RENDERER = "SurfelRenderer";
const Util::StringIdentifier ATTR_SURFEL_RENDERER_COUNT_FACTOR("countFactor");
const Util::StringIdentifier ATTR_SURFEL_RENDERER_SIZE_FACTOR("sizeFactor");
const Util::StringIdentifier ATTR_SURFEL_RENDERER_MIN_SIZE("minSize");
const Util::StringIdentifier ATTR_SURFEL_RENDERER_MAX_SIZE("maxSize");
cStr_t STATE_TYPE_SURFEL_RENDERER2 = "SurfelRenderer2";
cStr_t STATE_TYPE_SURFEL_RENDERER_FIXED_SIZE = "SurfelRendererFixedSize";
cStr_t STATE_TYPE_SURFEL_RENDERER_BUDGET = "SurfelRendererBudget";
const Util::StringIdentifier ATTR_SURFEL_RENDERER_MAX_SURFEL_SIZE("maxSurfelSize");
#endif

// ---------------------------------------------------------------------------
// MultiAlgoRendering
#ifdef MINSG_EXT_MULTIALGORENDERING
cStr_t NODE_TYPE_MULTIALGOGROUPNODE="multiAlgoGroupNode";
const Util::StringIdentifier ATTR_MAGN_NODEID("magn_nodeid");
cStr_t STATE_TYPE_ALGOSELECTOR="algoSelector";
cStr_t STATE_TYPE_MAR_SURFEL_RENDERER="MARSurfelRenderer";
const Util::StringIdentifier ATTR_MAR_SURFEL_COUNT_FACTOR("magn_surfelCountFactor");
const Util::StringIdentifier ATTR_MAR_SURFEL_SIZE_FACTOR("mar_surfelSizeFactor");
const Util::StringIdentifier ATTR_MAR_SURFEL_FORCE_FLAG("mar_surfelForceFlag");
const Util::StringIdentifier ATTR_MAR_SURFEL_MAX_AUTO_SIZE("mar_surfelMaxAutoSize");
#endif

// ---------------------------------------------------------------------------
// Particle system
cStr_t NODE_TYPE_PARTICLESYSTEM = "particlesystem";
const Util::StringIdentifier ATTR_PARTICLE_RENDERER("renderer");
const Util::StringIdentifier ATTR_PARTICLE_MAX_PARTICLE_COUNT("maxParticleCount");

const Util::StringIdentifier ATTR_PARTICLE_PER_SECOND("perSecond");
const Util::StringIdentifier ATTR_PARTICLE_DIRECTION("direction");
const Util::StringIdentifier ATTR_PARTICLE_DIR_VARIANCE("dirVariance");
const Util::StringIdentifier ATTR_PARTICLE_MIN_HEIGHT("minHeight");
const Util::StringIdentifier ATTR_PARTICLE_MAX_HEIGHT("maxHeight");
const Util::StringIdentifier ATTR_PARTICLE_MIN_WIDTH("minWidth");
const Util::StringIdentifier ATTR_PARTICLE_MAX_WIDTH("maxWidth");
const Util::StringIdentifier ATTR_PARTICLE_MIN_SPEED("minSpeed");
const Util::StringIdentifier ATTR_PARTICLE_MAX_SPEED("maxSpeed");
const Util::StringIdentifier ATTR_PARTICLE_MIN_LIFE("minLife");
const Util::StringIdentifier ATTR_PARTICLE_MAX_LIFE("maxLife");
const Util::StringIdentifier ATTR_PARTICLE_MIN_COLOR("minColor");
const Util::StringIdentifier ATTR_PARTICLE_MAX_COLOR("maxColor");
const Util::StringIdentifier ATTR_PARTICLE_SPAWN_NODE("spawnNode");
const Util::StringIdentifier ATTR_PARTICLE_TIME_OFFSET("timeOffset");
const Util::StringIdentifier ATTR_PARTICLE_GRAVITY("gravity");
const Util::StringIdentifier ATTR_PARTICLE_OFFSET("offset");
const Util::StringIdentifier ATTR_PARTICLE_EMIT_BOUNDS("emitBounds");

const Util::StringIdentifier ATTR_PARTICLE_REFLECTION_PLANE("plane");
const Util::StringIdentifier ATTR_PARTICLE_REFLECTION_REFLECTIVENESS("reflect");
const Util::StringIdentifier ATTR_PARTICLE_REFLECTION_ADHERENCE("adherence");



cStr_t BEHAVIOUR_TYPE_PARTICLE_POINT_EMITTER = "ParticlePointEmitter";
cStr_t BEHAVIOUR_TYPE_PARTICLE_BOX_EMITTER = "ParticleBoxEmitter";
cStr_t BEHAVIOUR_TYPE_PARTICLE_GRAVITY_AFFECTOR = "ParticleGravityAffector";
cStr_t BEHAVIOUR_TYPE_PARTICLE_FADE_OUT_AFFECTOR = "ParticleFadeOutAffector";
cStr_t BEHAVIOUR_TYPE_PARTICLE_ANIMATOR = "ParticleAnimator";
cStr_t BEHAVIOUR_TYPE_PARTICLE_REFLECTION_AFFECTOR = "ParticleReflector";

// ---------------------------------------------------------------------------
// Path node and waypoint
cStr_t NODE_TYPE_PATH="path";
cStr_t NODE_TYPE_WAYPOINT="waypoint";
cStr_t BEHAVIOUR_TYPE_FOLLOW_PATH="followPath";

const Util::StringIdentifier ATTR_PATHNODE_LOOPING("looping");
const Util::StringIdentifier ATTR_WAYPOINT_TIME("time");
const Util::StringIdentifier ATTR_FOLLOW_PATH_PATH_ID("pathId");

// ---------------------------------------------------------------------------
// Projected size filter state
cStr_t STATE_TYPE_PROJ_SIZE_FILTER_STATE = "proj_size_filter_state";
const Util::StringIdentifier ATTR_PSFS_MAXIMUM_PROJECTED_SIZE("max_size");
const Util::StringIdentifier ATTR_PSFS_MINIMUM_DISTANCE("min_dist");
const Util::StringIdentifier ATTR_PSFS_SOURCE_CHANNEL("source_channel");
const Util::StringIdentifier ATTR_PSFS_TARGET_CHANNEL("dest_channel");
const Util::StringIdentifier ATTR_PSFS_FORCE_CLOSED_NODES("force_closed_nodes");

//----------------------------------------------------------------------------
// LOD Renderer
cStr_t STATE_TYPE_LOD_RENDERER = "lod_renderer";
const Util::StringIdentifier ATTR_LOD_RENDERER_MIN_COMPLEXITY("min_complexity");
const Util::StringIdentifier ATTR_LOD_RENDERER_MAX_COMPLEXITY("max_complexity");
const Util::StringIdentifier ATTR_LOD_RENDERER_REL_COMPLEXITY("rel_complexity");
const Util::StringIdentifier ATTR_LOD_RENDERER_SOURCE_CHANNEL("source_channel");

// ---------------------------------------------------------------------------
// Spherical Visibility Sampling
cStr_t STATE_TYPE_SVS_RENDERER = "spherical_sampling_renderer";
const Util::StringIdentifier ATTR_SVS_INTERPOLATION_METHOD("interpolation_method");
const Util::StringIdentifier ATTR_SVS_RENDERER_SPHERE_OCCLUSION_TEST("sphere_occlusion_test");
const Util::StringIdentifier ATTR_SVS_RENDERER_GEOMETRY_OCCLUSION_TEST("geometry_occlusion_test");
cStr_t STATE_TYPE_SVS_BUDGETRENDERER = "svs_budget_renderer";
const Util::StringIdentifier ATTR_SVS_BUDGETRENDERER_BUDGET("budget");

// ---------------------------------------------------------------------------
// Valuated region
cStr_t NODE_TYPE_VALUATED_REGION("region");
const Util::StringIdentifier ATTR_VALREGION_BOX("box");
const Util::StringIdentifier ATTR_VALREGION_RESOLUTION("resolution");
const Util::StringIdentifier ATTR_VALREGION_COLOR("color");
const Util::StringIdentifier ATTR_VALREGION_HEIGHT("height");
const Util::StringIdentifier ATTR_VALREGION_VALUE("value");
const Util::StringIdentifier ATTR_VALREGION_VALUE_TYPE("valueType");

// ---------------------------------------------------------------------------
// Shadow state
cStr_t STATE_TYPE_SHADOW_STATE = "shadow";
const Util::StringIdentifier ATTR_SHADOW_TEXTURE_SIZE("texture_size");
const Util::StringIdentifier ATTR_SHADOW_LIGHT_NODE("light_node");


}
}
}
