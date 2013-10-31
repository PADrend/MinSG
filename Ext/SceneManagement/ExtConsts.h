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
extern cStr_t STATE_TYPE_OCC_RENDERER; // ext
extern cStr_t STATE_TYPE_SKYBOX; // ext
extern cStr_t STATE_TYPE_OCC_RENDERER; // ext

// ------------------------------------------------------------
//!	@name Billboard
//	@{
extern cStr_t NODE_TYPE_BILLBOARD;
extern const Util::StringIdentifier ATTR_BILLBOARD_RECT;
extern const Util::StringIdentifier ATTR_BILLBOARD_ROTATE_UP;
extern const Util::StringIdentifier ATTR_BILLBOARD_ROTATE_RIGHT;
//	@}
// ------------------------------------------------------------
//!	@name Budget annotation state
//	@{
extern cStr_t STATE_TYPE_BUDGET_ANNOTATION_STATE;
extern const Util::StringIdentifier ATTR_BAS_ANNOTATION_ATTRIBUTE;
extern const Util::StringIdentifier ATTR_BAS_BUDGET;
extern const Util::StringIdentifier ATTR_BAS_DISTRIBUTION_TYPE;
//	@}
// ------------------------------------------------------------
//!	@name CHC++
//	@{
extern cStr_t STATE_TYPE_CHCPP_RENDERER; // ext
extern const Util::StringIdentifier ATTR_CHCPP_VISIBILITYTHRESHOLD;
extern const Util::StringIdentifier ATTR_CHCPP_MAXPREVINVISNODESBATCHSIZE;
extern const Util::StringIdentifier ATTR_CHCPP_SKIPPEDFRAMESTILLQUERY;
extern const Util::StringIdentifier ATTR_CHCPP_MAXDEPTHFORTIGHTBOUNDINGVOLUMES;
extern const Util::StringIdentifier ATTR_CHCPP_MAXAREADERIVATIONFORTIGHTBOUNDINGVOLUMES;
//	@}
// ------------------------------------------------------------
//!	@name Color cubes
//	@{
extern cStr_t STATE_TYPE_COLOR_CUBE_RENDERER;
extern const Util::StringIdentifier ATTR_COLOR_CUBE_RENDERER_HIGHLIGHT;
//	@}
// ------------------------------------------------------------
//!	@name GenericMetaNode
//	@{
extern cStr_t NODE_TYPE_GENERIC_META_NODE;
extern const Util::StringIdentifier ATTR_GENERIC_META_NODE_BB;
//	@}
// ------------------------------------------------------------
//!	@name Mirror state
//	@{
extern cStr_t STATE_TYPE_MIRROR_STATE;
extern const Util::StringIdentifier ATTR_MIRROR_TEXTURE_SIZE;
//	@}
// ------------------------------------------------------------
//!	@name MultiAlgoRendering
// @{
#ifdef MINSG_EXT_MULTIALGORENDERING
extern cStr_t NODE_TYPE_MULTIALGOGROUPNODE;
extern const Util::StringIdentifier ATTR_MAGN_NODEID;
extern cStr_t STATE_TYPE_ALGOSELECTOR;
extern cStr_t STATE_TYPE_MAR_SURFEL_RENDERER;
extern const Util::StringIdentifier ATTR_MAR_SURFEL_COUNT_FACTOR;
extern const Util::StringIdentifier ATTR_MAR_SURFEL_SIZE_FACTOR;
extern const Util::StringIdentifier ATTR_MAR_SURFEL_FORCE_FLAG;
extern const Util::StringIdentifier ATTR_MAR_SURFEL_MAX_AUTO_SIZE;
#endif
// @}
// ------------------------------------------------------------
//!	@name Particle system
//	@{
extern cStr_t NODE_TYPE_PARTICLESYSTEM;

extern const Util::StringIdentifier ATTR_PARTICLE_RENDERER;
extern const Util::StringIdentifier ATTR_PARTICLE_MAX_PARTICLE_COUNT;

extern cStr_t BEHAVIOUR_TYPE_PARTICLE_POINT_EMITTER; // ext
extern cStr_t BEHAVIOUR_TYPE_PARTICLE_BOX_EMITTER; // ext
extern cStr_t BEHAVIOUR_TYPE_PARTICLE_GRAVITY_AFFECTOR; // ext
extern cStr_t BEHAVIOUR_TYPE_PARTICLE_FADE_OUT_AFFECTOR; // ext
extern cStr_t BEHAVIOUR_TYPE_PARTICLE_ANIMATOR; // ext

extern const Util::StringIdentifier ATTR_PARTICLE_PER_SECOND;
extern const Util::StringIdentifier ATTR_PARTICLE_DIRECTION;
extern const Util::StringIdentifier ATTR_PARTICLE_DIR_VARIANCE;
extern const Util::StringIdentifier ATTR_PARTICLE_MIN_HEIGHT;
extern const Util::StringIdentifier ATTR_PARTICLE_MAX_HEIGHT;	
extern const Util::StringIdentifier ATTR_PARTICLE_MIN_WIDTH;
extern const Util::StringIdentifier ATTR_PARTICLE_MAX_WIDTH;
extern const Util::StringIdentifier ATTR_PARTICLE_MIN_SPEED;
extern const Util::StringIdentifier ATTR_PARTICLE_MAX_SPEED;
extern const Util::StringIdentifier ATTR_PARTICLE_MIN_LIFE;
extern const Util::StringIdentifier ATTR_PARTICLE_MAX_LIFE;
extern const Util::StringIdentifier ATTR_PARTICLE_MIN_COLOR;
extern const Util::StringIdentifier ATTR_PARTICLE_MAX_COLOR;
extern const Util::StringIdentifier ATTR_PARTICLE_SPAWN_NODE;
extern const Util::StringIdentifier ATTR_PARTICLE_TIME_OFFSET;
extern const Util::StringIdentifier ATTR_PARTICLE_GRAVITY;
extern const Util::StringIdentifier ATTR_PARTICLE_OFFSET;
extern const Util::StringIdentifier ATTR_PARTICLE_EMIT_BOUNDS;
//	@}
// ------------------------------------------------------------
//!	@name Path node and waypoint
//	@{
extern cStr_t NODE_TYPE_PATH;
extern cStr_t NODE_TYPE_WAYPOINT;
extern const Util::StringIdentifier ATTR_PATHNODE_LOOPING;
extern const Util::StringIdentifier ATTR_WAYPOINT_TIME;
extern cStr_t BEHAVIOUR_TYPE_FOLLOW_PATH;
// behaviour attributes
extern const Util::StringIdentifier ATTR_FOLLOW_PATH_PATH_ID;
//	@}
// ------------------------------------------------------------
//!	@name Projected size filter state
//	@{
extern cStr_t STATE_TYPE_PROJ_SIZE_FILTER_STATE;
extern const Util::StringIdentifier ATTR_PSFS_MAXIMUM_PROJECTED_SIZE;
extern const Util::StringIdentifier ATTR_PSFS_MINIMUM_DISTANCE;
extern const Util::StringIdentifier ATTR_PSFS_SOURCE_CHANNEL;
extern const Util::StringIdentifier ATTR_PSFS_TARGET_CHANNEL;
extern const Util::StringIdentifier ATTR_PSFS_FORCE_CLOSED_NODES;
//	@}
// ------------------------------------------------------------
//!	@name LOD Renderer
//	@{
extern cStr_t STATE_TYPE_LOD_RENDERER;
extern const Util::StringIdentifier ATTR_LOD_RENDERER_MIN_COMPLEXITY;
extern const Util::StringIdentifier ATTR_LOD_RENDERER_MAX_COMPLEXITY;
extern const Util::StringIdentifier ATTR_LOD_RENDERER_REL_COMPLEXITY;
extern const Util::StringIdentifier ATTR_LOD_RENDERER_SOURCE_CHANNEL;
//	@}
// ------------------------------------------------------------
//!	@name Spherical Visibility Sampling
//	@{
extern cStr_t STATE_TYPE_SVS_RENDERER;
extern const Util::StringIdentifier ATTR_SVS_INTERPOLATION_METHOD;
extern const Util::StringIdentifier ATTR_SVS_RENDERER_SPHERE_OCCLUSION_TEST;
extern const Util::StringIdentifier ATTR_SVS_RENDERER_GEOMETRY_OCCLUSION_TEST;
extern cStr_t STATE_TYPE_SVS_BUDGETRENDERER;
extern const Util::StringIdentifier ATTR_SVS_BUDGETRENDERER_BUDGET;
//	@}
// ------------------------------------------------------------
//!	@name Valuated region
//	@{
extern cStr_t NODE_TYPE_VALUATED_REGION;
extern const Util::StringIdentifier ATTR_VALREGION_BOX;
extern const Util::StringIdentifier ATTR_VALREGION_RESOLUTION;
extern const Util::StringIdentifier ATTR_VALREGION_COLOR;
extern const Util::StringIdentifier ATTR_VALREGION_HEIGHT;
extern const Util::StringIdentifier ATTR_VALREGION_VALUE;
extern const Util::StringIdentifier ATTR_VALREGION_VALUE_TYPE;
//	@}

}
}
}

#endif // MINSG_EXT_CONSTS_H
