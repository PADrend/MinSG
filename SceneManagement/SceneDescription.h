/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_SCENEDESCRIPTION_H
#define MINSG_SCENEDESCRIPTION_H


#include <Util/GenericAttribute.h>
#include <cstdint>

namespace Util {
class AttributeProvider;
}
namespace MinSG {
class Node;
class State;

namespace SceneManagement {

typedef Util::GenericAttributeMap NodeDescription;
typedef Util::GenericAttributeList NodeDescriptionList;
typedef Util::WrapperAttribute<std::vector<float> > floatVecWrapper_t;
typedef Util::WrapperAttribute<std::vector<uint32_t> > uint32VecWrapper_t;

namespace Consts {
typedef const char * const cStr_t; // string constant

// ------------------------------------------------------
// structural description attributes

extern const Util::StringIdentifier CHILDREN; 		//!< data field of a NodeDescription containing a NodeDescriptionList of subentries
extern const Util::StringIdentifier DATA_BLOCK;
extern const Util::StringIdentifier DEFINITIONS;
extern const Util::StringIdentifier TYPE;

// ------------------------------------------------------

//  TYPEs
extern cStr_t TYPE_ATTRIBUTE;
extern cStr_t TYPE_STATE;
extern cStr_t TYPE_NODE;
extern cStr_t TYPE_DATA;
extern cStr_t TYPE_ADDITIONAL_DATA;
extern cStr_t TYPE_BEHAVIOUR;
extern cStr_t TYPE_SCENE;

// 	------------------------------------------------------

//!	@name TYPE = TYPE_SCENE
//	@{
extern const Util::StringIdentifier ATTR_SCENE_VERSION;
//	@}
// 	------------------------------------------------------

//!	@name TYPE = TYPE_NODE
//	@{
// general attributes
extern const Util::StringIdentifier ATTR_NODE_TYPE;
extern const Util::StringIdentifier ATTR_NODE_ID;
extern const Util::StringIdentifier ATTR_FLAG_CLOSED;

extern const Util::StringIdentifier ATTR_FIXED_BB;
extern const Util::StringIdentifier ATTR_SRT_POS;
extern const Util::StringIdentifier ATTR_SRT_DIR;
extern const Util::StringIdentifier ATTR_SRT_UP;
extern const Util::StringIdentifier ATTR_SRT_SCALE;
extern const Util::StringIdentifier ATTR_MATRIX;

// NODE_TYPEs
extern cStr_t NODE_TYPE_GEOMETRY;
extern cStr_t NODE_TYPE_CLONE;
extern cStr_t NODE_TYPE_LIGHT;
extern cStr_t NODE_TYPE_LIGHT_DIRECTIONAL; // remove??
extern cStr_t NODE_TYPE_LIGHT_POINT;	// remove??
extern cStr_t NODE_TYPE_LIGHT_SPOT;	// remove??
extern cStr_t NODE_TYPE_LIST;
extern cStr_t NODE_TYPE_CAMERA;

// specialized attributes and values
// NODE_TYPE = NODE_TYPE_CAMERA
extern const Util::StringIdentifier ATTR_CAM_NEAR;
extern const Util::StringIdentifier ATTR_CAM_FAR;
extern const Util::StringIdentifier ATTR_CAM_ANGLE;
extern const Util::StringIdentifier ATTR_CAM_RATIO;

// NODE_TYPE = NODE_TYPE_CLONE
extern const Util::StringIdentifier ATTR_CLONE_SOURCE;

// NODE_TYPE = NODE_TYPE_LIGHT
extern const Util::StringIdentifier ATTR_LIGHT_TYPE;
extern cStr_t LIGHT_TYPE_POINT;
extern cStr_t LIGHT_TYPE_DIRECTIONAL;
extern cStr_t LIGHT_TYPE_SPOT;
extern const Util::StringIdentifier ATTR_LIGHT_AMBIENT;
extern const Util::StringIdentifier ATTR_LIGHT_DIFFUSE;
extern const Util::StringIdentifier ATTR_LIGHT_SPECULAR;
extern const Util::StringIdentifier ATTR_LIGHT_CONSTANT_ATTENUATION;
extern const Util::StringIdentifier ATTR_LIGHT_LINEAR_ATTENUATION;
extern const Util::StringIdentifier ATTR_LIGHT_QUADRATIC_ATTENUATION;
extern const Util::StringIdentifier ATTR_LIGHT_SPOT_CUTOFF;
extern const Util::StringIdentifier ATTR_LIGHT_SPOT_EXPONENT;
//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_STATE
//	@{
// general attributes
extern const Util::StringIdentifier ATTR_STATE_ID;
extern const Util::StringIdentifier ATTR_STATE_TYPE;

// STATE_TYPEs
extern cStr_t STATE_TYPE_REFERENCE;
extern cStr_t STATE_TYPE_ALPHA_TEST;
extern cStr_t STATE_TYPE_BLENDING;
extern cStr_t STATE_TYPE_COLOR;
extern cStr_t STATE_TYPE_CULL_FACE;
extern cStr_t STATE_TYPE_GROUP;
extern cStr_t STATE_TYPE_MATERIAL;
extern cStr_t STATE_TYPE_POLYGON_MODE;
extern cStr_t STATE_TYPE_SHADER;
extern cStr_t STATE_TYPE_STATEFULSHADER; // ext
extern cStr_t STATE_TYPE_TEXTURE;
extern cStr_t STATE_TYPE_TRANSPARENCY_RENDERER;
extern cStr_t STATE_TYPE_LIGHTING_STATE;
extern cStr_t STATE_TYPE_SHADER_UNIFORM; 

// ATTR_STATE_TYPE = STATE_TYPE_REFERENCE
extern const Util::StringIdentifier ATTR_REFERENCED_STATE_ID;

// ATTR_STATE_TYPE = STATE_TYPE_TRANSPARENCY_RENDERER
extern const Util::StringIdentifier ATTR_TRANSPARENY_USE_PREMULTIPLIED_ALPHA;

// ATTR_STATE_TYPE = STATE_TYPE_SHADER | STATE_TYPE_STATEFULSHADER
extern const Util::StringIdentifier ATTR_SHADER_OBJ_FILENAME;
extern const Util::StringIdentifier ATTR_SHADER_USES_SG_UNIFORMS;
extern const Util::StringIdentifier ATTR_SHADER_USES_CLASSIC_GL;
extern const Util::StringIdentifier ATTR_SHADER_NAME;
extern const Util::StringIdentifier STATE_ATTR_SHADER_NAME;
extern cStr_t DATA_TYPE_GLSL_FS;
extern cStr_t DATA_TYPE_GLSL_VS;
extern cStr_t DATA_TYPE_GLSL_GS;
extern cStr_t DATA_TYPE_GLSL_USAGE;
extern const Util::StringIdentifier STATE_ATTR_SHADER_FILES; 

// ATTR_STATE_TYPE = STATE_TYPE_ALPHA_TEST
extern const Util::StringIdentifier ATTR_ALPHA_TEST_MODE;
extern const Util::StringIdentifier ATTR_ALPHA_REF_VALUE;
extern const Util::StringIdentifier ATTR_ALPHA_TEST_MODE_OLD;
extern const Util::StringIdentifier ATTR_ALPHA_REF_VALUE_OLD;

// ATTR_STATE_TYPE = STATE_TYPE_BLENDING
extern const Util::StringIdentifier ATTR_BLEND_EQUATION;
extern const Util::StringIdentifier ATTR_BLEND_FUNC_SRC;
extern const Util::StringIdentifier ATTR_BLEND_FUNC_DST;
extern const Util::StringIdentifier ATTR_BLEND_CONST_ALPHA;
extern const Util::StringIdentifier ATTR_BLEND_DEPTH_MASK;

// ATTR_STATE_TYPE = STATE_TYPE_LIGHTING_STATE
extern const Util::StringIdentifier ATTR_LIGHTING_LIGHT_ID;

// ATTR_STATE_TYPE = STATE_TYPE_COLOR
extern const Util::StringIdentifier ATTR_COLOR_VALUE;

// ATTR_STATE_TYPE = STATE_TYPE_MATERIAL
extern const Util::StringIdentifier ATTR_MATERIAL_AMBIENT;
extern const Util::StringIdentifier ATTR_MATERIAL_DIFFUSE;
extern const Util::StringIdentifier ATTR_MATERIAL_SPECULAR;
extern const Util::StringIdentifier ATTR_MATERIAL_SHININESS;

// ATTR_STATE_TYPE = STATE_TYPE_CULL_FACE
extern const Util::StringIdentifier ATTR_CULL_FACE;

// ATTR_STATE_TYPE = STATE_TYPE_POLYGON_MODE
extern const Util::StringIdentifier ATTR_POLYGON_MODE;
extern const Util::StringIdentifier ATTR_POLYGON_MODE_OLD;

//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_DATA
//	@{
extern const Util::StringIdentifier ATTR_DATA_TYPE;

extern const Util::StringIdentifier ATTR_DATA_ENCODING;
extern const Util::StringIdentifier ATTR_DATA_FORMAT;
extern cStr_t DATA_ENCODING_BASE64;

extern cStr_t DATA_TYPE_SHADER_UNIFORM;

// DATA_TYPE = "texture/..."
extern const Util::StringIdentifier ATTR_TEXTURE_FILENAME;
extern const Util::StringIdentifier ATTR_TEXTURE_ID;
extern const Util::StringIdentifier ATTR_TEXTURE_UNIT;

// DATA_TYPE = "mesh"
extern const Util::StringIdentifier ATTR_MESH_FILENAME;
extern const Util::StringIdentifier ATTR_MESH_BB;
extern const Util::StringIdentifier ATTR_MESH_DATA;

// DATA_TYPE = DATA_TYPE_SHADER_UNIFORM
extern const Util::StringIdentifier ATTR_SHADER_UNIFORM_NAME;
extern const Util::StringIdentifier ATTR_SHADER_UNIFORM_VALUES;
extern const Util::StringIdentifier ATTR_SHADER_UNIFORM_TYPE;
extern cStr_t SHADER_UNIFORM_TYPE_BOOL;
extern cStr_t SHADER_UNIFORM_TYPE_VEC2B;
extern cStr_t SHADER_UNIFORM_TYPE_VEC3B;
extern cStr_t SHADER_UNIFORM_TYPE_VEC4B;
extern cStr_t SHADER_UNIFORM_TYPE_FLOAT;
extern cStr_t SHADER_UNIFORM_TYPE_VEC2F;
extern cStr_t SHADER_UNIFORM_TYPE_VEC3F;
extern cStr_t SHADER_UNIFORM_TYPE_VEC4F;
extern cStr_t SHADER_UNIFORM_TYPE_INT;
extern cStr_t SHADER_UNIFORM_TYPE_VEC2I;
extern cStr_t SHADER_UNIFORM_TYPE_VEC3I;
extern cStr_t SHADER_UNIFORM_TYPE_VEC4I;
extern cStr_t SHADER_UNIFORM_TYPE_MATRIX_2X2F;
extern cStr_t SHADER_UNIFORM_TYPE_MATRIX_3X3F;
extern cStr_t SHADER_UNIFORM_TYPE_MATRIX_4X4F;
//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_ATTRIBUTE
//	@{
extern const Util::StringIdentifier ATTR_ATTRIBUTE_NAME;
extern const Util::StringIdentifier ATTR_ATTRIBUTE_VALUE;
extern const Util::StringIdentifier ATTR_ATTRIBUTE_TYPE;

extern cStr_t ATTRIBUTE_TYPE_BOOL;
extern cStr_t ATTRIBUTE_TYPE_FLOAT;
extern cStr_t ATTRIBUTE_TYPE_GENERIC;
extern cStr_t ATTRIBUTE_TYPE_INT;
extern cStr_t ATTRIBUTE_TYPE_JSON;
extern cStr_t ATTRIBUTE_TYPE_NUMBER;
extern cStr_t ATTRIBUTE_TYPE_STRING;
//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_BEHAVIOUR
//	@{
extern const Util::StringIdentifier ATTR_BEHAVIOUR_TYPE;
//	}

// ----------------------------------

//!	@name Skeletal Animation
//	@{	
extern const Util::StringIdentifier ATTR_SKEL_ANCHORID;
extern cStr_t NODE_TYPE_SKEL_RIGIDJOINT;
extern const Util::StringIdentifier ATTR_SKEL_RIGIDOFFSETMATRIX;
extern const Util::StringIdentifier ATTR_SKEL_RIGIDSTACKING;
extern cStr_t NODE_TYPE_SKEL_JOINT;
extern cStr_t NODE_TYPE_SKEL_JOINTS;
extern cStr_t NODE_TYPE_SKEL_SKELETALOBJECT;
extern cStr_t NODE_TYPE_SKEL_ANIMATIONSAMPLE;
extern cStr_t NODE_TYPE_SKEL_ARMATURE;
extern cStr_t STATE_TYPE_SKEL_SKELETALRENDERERSTATE; // deprecated!!!   Still in cause of old MinSG files
extern cStr_t STATE_TYPE_SKEL_SKELETALHARDWARERENDERERSTATE;
extern cStr_t NODE_TYPE_SKEL_VERTEX_WEIGHT;
extern cStr_t NODE_TYPE_SKEL_CONTROLLER;
extern cStr_t NODE_TYPE_SKEL_SKIN;
extern cStr_t NODE_TYPE_SKEL_JOINT_STRUCTURE;
extern const Util::StringIdentifier ATTR_SKEL_JOINTID;
extern const Util::StringIdentifier ATTR_SKEL_SKELETON;
extern const Util::StringIdentifier ATTR_SKEL_ID_TYPE_ARMATURE_MATRIX;
extern const Util::StringIdentifier ATTR_SKEL_TIMELINE;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALSAMPLERDATA;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALINTERPOLATIONTYPE;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONSTARTTIME;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONTARGET;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONNAME;
extern const Util::StringIdentifier ATTR_SKEL_INVERSEBINDMATRIX;
extern const Util::StringIdentifier ATTR_SKEL_BINDMATRIX;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONPOSEDESCRIPTION;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALSTARTANIMATION;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALTOANIMATIONS;
extern const Util::StringIdentifier ATTR_SKEL_SKELETALFROMANIMATIONS;
extern cStr_t BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA;
//	@}

// ----------------------------------

}

}
}
#endif // MINSG_SCENEDESCRIPTION_H
