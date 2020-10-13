/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_EXT_CONSTS_H
#define MINSG_EXT_CONSTS_H

#include <Util/GenericAttribute.h>
#include <cstdint>

namespace MinSG {
namespace SceneManagement {
namespace Consts {

typedef const char * const cStr_t; // string constant

// ------------------------------------------------------------
//! Various States
MINSGAPI extern cStr_t STATE_TYPE_OCC_RENDERER; // ext
MINSGAPI extern cStr_t STATE_TYPE_SKYBOX; // ext
MINSGAPI extern cStr_t STATE_TYPE_OCC_RENDERER; // ext

// ------------------------------------------------------------
//!	@name Billboard
//	@{
MINSGAPI extern cStr_t NODE_TYPE_BILLBOARD;
MINSGAPI extern const Util::StringIdentifier ATTR_BILLBOARD_RECT;
MINSGAPI extern const Util::StringIdentifier ATTR_BILLBOARD_ROTATE_UP;
MINSGAPI extern const Util::StringIdentifier ATTR_BILLBOARD_ROTATE_RIGHT;
//	@}
// ------------------------------------------------------------
//!	@name Budget annotation state
//	@{
MINSGAPI extern cStr_t STATE_TYPE_BUDGET_ANNOTATION_STATE;
MINSGAPI extern const Util::StringIdentifier ATTR_BAS_ANNOTATION_ATTRIBUTE;
MINSGAPI extern const Util::StringIdentifier ATTR_BAS_BUDGET;
MINSGAPI extern const Util::StringIdentifier ATTR_BAS_DISTRIBUTION_TYPE;
//	@}
// ------------------------------------------------------------
//!	@name CHC++
//	@{
MINSGAPI extern cStr_t STATE_TYPE_CHCPP_RENDERER; // ext
MINSGAPI extern const Util::StringIdentifier ATTR_CHCPP_VISIBILITYTHRESHOLD;
MINSGAPI extern const Util::StringIdentifier ATTR_CHCPP_MAXPREVINVISNODESBATCHSIZE;
MINSGAPI extern const Util::StringIdentifier ATTR_CHCPP_SKIPPEDFRAMESTILLQUERY;
MINSGAPI extern const Util::StringIdentifier ATTR_CHCPP_MAXDEPTHFORTIGHTBOUNDINGVOLUMES;
MINSGAPI extern const Util::StringIdentifier ATTR_CHCPP_MAXAREADERIVATIONFORTIGHTBOUNDINGVOLUMES;
//	@}
// ------------------------------------------------------------
//!	@name Color cubes
//	@{
MINSGAPI extern cStr_t STATE_TYPE_COLOR_CUBE_RENDERER;
MINSGAPI extern const Util::StringIdentifier ATTR_COLOR_CUBE_RENDERER_HIGHLIGHT;
//	@}
// ------------------------------------------------------------
//!	@name GenericMetaNode
//	@{
MINSGAPI extern cStr_t NODE_TYPE_GENERIC_META_NODE;
MINSGAPI extern const Util::StringIdentifier ATTR_GENERIC_META_NODE_BB;
//	@}
// ------------------------------------------------------------
//!	@name Mirror state
//	@{
MINSGAPI extern cStr_t STATE_TYPE_MIRROR_STATE;
MINSGAPI extern const Util::StringIdentifier ATTR_MIRROR_TEXTURE_SIZE;
//	@}
// ------------------------------------------------------------
//!	@name BlueSurfels
// @{
#ifdef MINSG_EXT_BLUE_SURFELS
MINSGAPI extern cStr_t STATE_TYPE_SURFEL_RENDERER;
#endif
// @}
// ------------------------------------------------------------
//!	@name MultiAlgoRendering
// @{
#ifdef MINSG_EXT_MULTIALGORENDERING
MINSGAPI extern cStr_t NODE_TYPE_MULTIALGOGROUPNODE;
MINSGAPI extern const Util::StringIdentifier ATTR_MAGN_NODEID;
MINSGAPI extern cStr_t STATE_TYPE_ALGOSELECTOR;
MINSGAPI extern cStr_t STATE_TYPE_MAR_SURFEL_RENDERER;
MINSGAPI extern const Util::StringIdentifier ATTR_MAR_SURFEL_COUNT_FACTOR;
MINSGAPI extern const Util::StringIdentifier ATTR_MAR_SURFEL_SIZE_FACTOR;
MINSGAPI extern const Util::StringIdentifier ATTR_MAR_SURFEL_FORCE_FLAG;
MINSGAPI extern const Util::StringIdentifier ATTR_MAR_SURFEL_MAX_AUTO_SIZE;
#endif
// @}
// ------------------------------------------------------------
//!	@name Particle system
//	@{
MINSGAPI extern cStr_t NODE_TYPE_PARTICLESYSTEM;

MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_RENDERER;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MAX_PARTICLE_COUNT;

MINSGAPI extern cStr_t BEHAVIOUR_TYPE_PARTICLE_ANIMATOR; // ext
MINSGAPI extern cStr_t BEHAVIOUR_TYPE_PARTICLE_BOX_EMITTER; // ext
MINSGAPI extern cStr_t BEHAVIOUR_TYPE_PARTICLE_FADE_OUT_AFFECTOR; // ext
MINSGAPI extern cStr_t BEHAVIOUR_TYPE_PARTICLE_GRAVITY_AFFECTOR; // ext
MINSGAPI extern cStr_t BEHAVIOUR_TYPE_PARTICLE_POINT_EMITTER; // ext
MINSGAPI extern cStr_t BEHAVIOUR_TYPE_PARTICLE_REFLECTION_AFFECTOR; // ext

MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_DIRECTION;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_DIR_VARIANCE;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_EMIT_BOUNDS;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_GRAVITY;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MIN_HEIGHT;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MAX_HEIGHT;	
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MIN_WIDTH;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MAX_WIDTH;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MIN_SPEED;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MAX_SPEED;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MIN_LIFE;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MAX_LIFE;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MIN_COLOR;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_MAX_COLOR;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_OFFSET;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_PER_SECOND;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_REFLECTION_ADHERENCE;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_REFLECTION_PLANE;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_REFLECTION_REFLECTIVENESS;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_SPAWN_NODE;
MINSGAPI extern const Util::StringIdentifier ATTR_PARTICLE_TIME_OFFSET;
//	@}
// ------------------------------------------------------------
//!	@name Path node and waypoint
//	@{
MINSGAPI extern cStr_t NODE_TYPE_PATH;
MINSGAPI extern cStr_t NODE_TYPE_WAYPOINT;
MINSGAPI extern const Util::StringIdentifier ATTR_PATHNODE_LOOPING;
MINSGAPI extern const Util::StringIdentifier ATTR_WAYPOINT_TIME;
MINSGAPI extern cStr_t BEHAVIOUR_TYPE_FOLLOW_PATH;
// behaviour attributes
MINSGAPI extern const Util::StringIdentifier ATTR_FOLLOW_PATH_PATH_ID;
//	@}
// ------------------------------------------------------------
//!	@name Projected size filter state
//	@{
MINSGAPI extern cStr_t STATE_TYPE_PROJ_SIZE_FILTER_STATE;
MINSGAPI extern const Util::StringIdentifier ATTR_PSFS_MAXIMUM_PROJECTED_SIZE;
MINSGAPI extern const Util::StringIdentifier ATTR_PSFS_MINIMUM_DISTANCE;
MINSGAPI extern const Util::StringIdentifier ATTR_PSFS_SOURCE_CHANNEL;
MINSGAPI extern const Util::StringIdentifier ATTR_PSFS_TARGET_CHANNEL;
MINSGAPI extern const Util::StringIdentifier ATTR_PSFS_FORCE_CLOSED_NODES;
//	@}
// ------------------------------------------------------------
//!	@name LOD Renderer
//	@{
MINSGAPI extern cStr_t STATE_TYPE_LOD_RENDERER;
MINSGAPI extern const Util::StringIdentifier ATTR_LOD_RENDERER_MIN_COMPLEXITY;
MINSGAPI extern const Util::StringIdentifier ATTR_LOD_RENDERER_MAX_COMPLEXITY;
MINSGAPI extern const Util::StringIdentifier ATTR_LOD_RENDERER_REL_COMPLEXITY;
MINSGAPI extern const Util::StringIdentifier ATTR_LOD_RENDERER_SOURCE_CHANNEL;
//	@}
// ------------------------------------------------------------
//!	@name Spherical Visibility Sampling
//	@{
MINSGAPI extern cStr_t STATE_TYPE_SVS_RENDERER;
MINSGAPI extern const Util::StringIdentifier ATTR_SVS_INTERPOLATION_METHOD;
MINSGAPI extern const Util::StringIdentifier ATTR_SVS_RENDERER_SPHERE_OCCLUSION_TEST;
MINSGAPI extern const Util::StringIdentifier ATTR_SVS_RENDERER_GEOMETRY_OCCLUSION_TEST;
MINSGAPI extern cStr_t STATE_TYPE_SVS_BUDGETRENDERER;
MINSGAPI extern const Util::StringIdentifier ATTR_SVS_BUDGETRENDERER_BUDGET;
//	@}
// ------------------------------------------------------------
//!	@name Valuated region
//	@{
MINSGAPI extern cStr_t NODE_TYPE_VALUATED_REGION;
MINSGAPI extern const Util::StringIdentifier ATTR_VALREGION_BOX;
MINSGAPI extern const Util::StringIdentifier ATTR_VALREGION_RESOLUTION;
MINSGAPI extern const Util::StringIdentifier ATTR_VALREGION_COLOR;
MINSGAPI extern const Util::StringIdentifier ATTR_VALREGION_HEIGHT;
MINSGAPI extern const Util::StringIdentifier ATTR_VALREGION_VALUE;
MINSGAPI extern const Util::StringIdentifier ATTR_VALREGION_VALUE_TYPE;
//	@}
// ------------------------------------------------------------
//!	@name Shadow state
//	@{
MINSGAPI extern cStr_t STATE_TYPE_SHADOW_STATE;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADOW_TEXTURE_SIZE;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADOW_LIGHT_NODE;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADOW_STATIC;
//	@}
// ------------------------------------------------------------

}
}
}

#endif // MINSG_EXT_CONSTS_H
