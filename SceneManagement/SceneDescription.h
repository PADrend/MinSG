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

typedef Util::GenericAttributeMap DescriptionMap;
typedef Util::GenericAttributeList DescriptionArray;
typedef Util::WrapperAttribute<std::vector<float> > floatVecWrapper_t;
typedef Util::WrapperAttribute<std::vector<uint32_t> > uint32VecWrapper_t;

namespace Consts {
typedef const char * const cStr_t; // string constant

// ------------------------------------------------------
// structural description attributes

MINSGAPI extern const Util::StringIdentifier CHILDREN; 		//!< data field of a DescriptionMap containing a DescriptionArray of subentries
MINSGAPI extern const Util::StringIdentifier DATA_BLOCK;
MINSGAPI extern const Util::StringIdentifier DEFINITIONS;
MINSGAPI extern const Util::StringIdentifier TYPE;

// ------------------------------------------------------

//  TYPEs
MINSGAPI extern cStr_t TYPE_ATTRIBUTE;
MINSGAPI extern cStr_t TYPE_STATE;
MINSGAPI extern cStr_t TYPE_NODE;
MINSGAPI extern cStr_t TYPE_DATA;
MINSGAPI extern cStr_t TYPE_ADDITIONAL_DATA;
MINSGAPI extern cStr_t TYPE_BEHAVIOUR;
MINSGAPI extern cStr_t TYPE_SCENE;

// 	------------------------------------------------------

//!	@name TYPE = TYPE_SCENE
//	@{
MINSGAPI extern const Util::StringIdentifier ATTR_SCENE_VERSION;
//	@}
// 	------------------------------------------------------

//!	@name TYPE = TYPE_NODE
//	@{
// general attributes
MINSGAPI extern const Util::StringIdentifier ATTR_NODE_TYPE;
MINSGAPI extern const Util::StringIdentifier ATTR_NODE_ID;
MINSGAPI extern const Util::StringIdentifier ATTR_FLAG_CLOSED;

MINSGAPI extern const Util::StringIdentifier ATTR_RENDERING_LAYERS;

MINSGAPI extern const Util::StringIdentifier ATTR_FIXED_BB;
MINSGAPI extern const Util::StringIdentifier ATTR_SRT_POS;
MINSGAPI extern const Util::StringIdentifier ATTR_SRT_DIR;
MINSGAPI extern const Util::StringIdentifier ATTR_SRT_UP;
MINSGAPI extern const Util::StringIdentifier ATTR_SRT_SCALE;
MINSGAPI extern const Util::StringIdentifier ATTR_MATRIX;

// NODE_TYPEs
MINSGAPI extern cStr_t NODE_TYPE_GEOMETRY;
MINSGAPI extern cStr_t NODE_TYPE_CLONE;
MINSGAPI extern cStr_t NODE_TYPE_LIGHT;
MINSGAPI extern cStr_t NODE_TYPE_LIGHT_DIRECTIONAL; // remove??
MINSGAPI extern cStr_t NODE_TYPE_LIGHT_POINT;	// remove??
MINSGAPI extern cStr_t NODE_TYPE_LIGHT_SPOT;	// remove??
MINSGAPI extern cStr_t NODE_TYPE_LIST;
MINSGAPI extern cStr_t NODE_TYPE_CAMERA;

// specialized attributes and values
// NODE_TYPE = NODE_TYPE_CAMERA
MINSGAPI extern const Util::StringIdentifier ATTR_CAM_NEAR;
MINSGAPI extern const Util::StringIdentifier ATTR_CAM_FAR;
MINSGAPI extern const Util::StringIdentifier ATTR_CAM_ANGLE; 
MINSGAPI extern const Util::StringIdentifier ATTR_CAM_RATIO; 
MINSGAPI extern const Util::StringIdentifier ATTR_CAM_FRUSTUM; // angles for persp. camera, clipping planes for ortho. camera
MINSGAPI extern const Util::StringIdentifier ATTR_CAM_VIEWPORT;
MINSGAPI extern const Util::StringIdentifier ATTR_CAM_TYPE;
MINSGAPI extern cStr_t CAM_TYPE_ORTHOGRAPHIC;
MINSGAPI extern cStr_t CAM_TYPE_PERSPECTIVE;

// NODE_TYPE = NODE_TYPE_CLONE
MINSGAPI extern const Util::StringIdentifier ATTR_CLONE_SOURCE;

// NODE_TYPE = NODE_TYPE_LIGHT
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_TYPE;
MINSGAPI extern cStr_t LIGHT_TYPE_POINT;
MINSGAPI extern cStr_t LIGHT_TYPE_DIRECTIONAL;
MINSGAPI extern cStr_t LIGHT_TYPE_SPOT;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_AMBIENT;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_DIFFUSE;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_SPECULAR;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_CONSTANT_ATTENUATION;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_LINEAR_ATTENUATION;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_QUADRATIC_ATTENUATION;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_SPOT_CUTOFF;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHT_SPOT_EXPONENT;
//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_STATE
//	@{
// general attributes
MINSGAPI extern const Util::StringIdentifier ATTR_STATE_ID;
MINSGAPI extern const Util::StringIdentifier ATTR_STATE_TYPE;

// STATE_TYPEs
MINSGAPI extern cStr_t STATE_TYPE_REFERENCE;
MINSGAPI extern cStr_t STATE_TYPE_ALPHA_TEST;
MINSGAPI extern cStr_t STATE_TYPE_BLENDING;
MINSGAPI extern cStr_t STATE_TYPE_COLOR;
MINSGAPI extern cStr_t STATE_TYPE_CULL_FACE;
MINSGAPI extern cStr_t STATE_TYPE_GROUP;
MINSGAPI extern cStr_t STATE_TYPE_MATERIAL;
MINSGAPI extern cStr_t STATE_TYPE_POLYGON_MODE;
MINSGAPI extern cStr_t STATE_TYPE_POINT_PARAMETER;
MINSGAPI extern cStr_t STATE_TYPE_SHADER;
MINSGAPI extern cStr_t STATE_TYPE_STATEFULSHADER; // ext
MINSGAPI extern cStr_t STATE_TYPE_TEXTURE;
MINSGAPI extern cStr_t STATE_TYPE_TRANSPARENCY_RENDERER;
MINSGAPI extern cStr_t STATE_TYPE_LIGHTING_STATE;
MINSGAPI extern cStr_t STATE_TYPE_SHADER_UNIFORM; 

// ATTR_STATE_TYPE = STATE_TYPE_REFERENCE
MINSGAPI extern const Util::StringIdentifier ATTR_REFERENCED_STATE_ID;

// ATTR_STATE_TYPE = STATE_TYPE_TRANSPARENCY_RENDERER
MINSGAPI extern const Util::StringIdentifier ATTR_TRANSPARENY_USE_PREMULTIPLIED_ALPHA;

// ATTR_STATE_TYPE = STATE_TYPE_SHADER | STATE_TYPE_STATEFULSHADER
MINSGAPI extern const Util::StringIdentifier ATTR_SHADER_OBJ_FILENAME;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADER_USES_SG_UNIFORMS;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADER_USES_CLASSIC_GL;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADER_NAME;
MINSGAPI extern const Util::StringIdentifier STATE_ATTR_SHADER_NAME;
MINSGAPI extern cStr_t DATA_TYPE_GLSL_FS;
MINSGAPI extern cStr_t DATA_TYPE_GLSL_VS;
MINSGAPI extern cStr_t DATA_TYPE_GLSL_GS;
MINSGAPI extern cStr_t DATA_TYPE_GLSL_USAGE;
MINSGAPI extern const Util::StringIdentifier STATE_ATTR_SHADER_FILES; 

// ATTR_STATE_TYPE = STATE_TYPE_ALPHA_TEST
MINSGAPI extern const Util::StringIdentifier ATTR_ALPHA_TEST_MODE;
MINSGAPI extern const Util::StringIdentifier ATTR_ALPHA_REF_VALUE;
MINSGAPI extern const Util::StringIdentifier ATTR_ALPHA_TEST_MODE_OLD;
MINSGAPI extern const Util::StringIdentifier ATTR_ALPHA_REF_VALUE_OLD;

// ATTR_STATE_TYPE = STATE_TYPE_BLENDING
MINSGAPI extern const Util::StringIdentifier ATTR_BLEND_EQUATION;
MINSGAPI extern const Util::StringIdentifier ATTR_BLEND_FUNC_SRC;
MINSGAPI extern const Util::StringIdentifier ATTR_BLEND_FUNC_DST;
MINSGAPI extern const Util::StringIdentifier ATTR_BLEND_CONST_ALPHA;
MINSGAPI extern const Util::StringIdentifier ATTR_BLEND_DEPTH_MASK;

// ATTR_STATE_TYPE = STATE_TYPE_LIGHTING_STATE
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHTING_LIGHT_ID;
MINSGAPI extern const Util::StringIdentifier ATTR_LIGHTING_ENABLE_LIGHTING;

// ATTR_STATE_TYPE = STATE_TYPE_COLOR
MINSGAPI extern const Util::StringIdentifier ATTR_COLOR_VALUE;

// ATTR_STATE_TYPE = STATE_TYPE_MATERIAL
MINSGAPI extern const Util::StringIdentifier ATTR_MATERIAL_AMBIENT;
MINSGAPI extern const Util::StringIdentifier ATTR_MATERIAL_DIFFUSE;
MINSGAPI extern const Util::StringIdentifier ATTR_MATERIAL_SPECULAR;
MINSGAPI extern const Util::StringIdentifier ATTR_MATERIAL_EMISSION;
MINSGAPI extern const Util::StringIdentifier ATTR_MATERIAL_SHININESS;

// ATTR_STATE_TYPE = STATE_TYPE_CULL_FACE
MINSGAPI extern const Util::StringIdentifier ATTR_CULL_FACE;

// ATTR_STATE_TYPE = STATE_TYPE_POLYGON_MODE
MINSGAPI extern const Util::StringIdentifier ATTR_POLYGON_MODE;
MINSGAPI extern const Util::StringIdentifier ATTR_POLYGON_MODE_OLD;


// ATTR_STATE_TYPE = STATE_TYPE_POINT_PARAMETER
MINSGAPI extern const Util::StringIdentifier ATTR_POINT_SIZE;
//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_DATA
//	@{
MINSGAPI extern const Util::StringIdentifier ATTR_DATA_TYPE;

MINSGAPI extern const Util::StringIdentifier ATTR_DATA_ENCODING;
MINSGAPI extern const Util::StringIdentifier ATTR_DATA_FORMAT;
MINSGAPI extern cStr_t DATA_ENCODING_BASE64;

MINSGAPI extern cStr_t DATA_TYPE_SHADER_UNIFORM;

// DATA_TYPE = "texture/..."
MINSGAPI extern const Util::StringIdentifier ATTR_TEXTURE_FILENAME;
MINSGAPI extern const Util::StringIdentifier ATTR_TEXTURE_ID;
MINSGAPI extern const Util::StringIdentifier ATTR_TEXTURE_UNIT;
MINSGAPI extern const Util::StringIdentifier ATTR_TEXTURE_NUM_LAYERS;
MINSGAPI extern const Util::StringIdentifier ATTR_TEXTURE_TYPE;

// DATA_TYPE = "mesh"
MINSGAPI extern const Util::StringIdentifier ATTR_MESH_FILENAME;
MINSGAPI extern const Util::StringIdentifier ATTR_MESH_BB;
MINSGAPI extern const Util::StringIdentifier ATTR_MESH_DATA;

// DATA_TYPE = DATA_TYPE_SHADER_UNIFORM
MINSGAPI extern const Util::StringIdentifier ATTR_SHADER_UNIFORM_NAME;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADER_UNIFORM_VALUES;
MINSGAPI extern const Util::StringIdentifier ATTR_SHADER_UNIFORM_TYPE;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_BOOL;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC2B;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC3B;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC4B;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_FLOAT;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC2F;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC3F;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC4F;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_INT;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC2I;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC3I;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_VEC4I;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_MATRIX_2X2F;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_MATRIX_3X3F;
MINSGAPI extern cStr_t SHADER_UNIFORM_TYPE_MATRIX_4X4F;
//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_ATTRIBUTE
//	@{
MINSGAPI extern const Util::StringIdentifier ATTR_ATTRIBUTE_NAME;
MINSGAPI extern const Util::StringIdentifier ATTR_ATTRIBUTE_VALUE;
MINSGAPI extern const Util::StringIdentifier ATTR_ATTRIBUTE_TYPE;

MINSGAPI extern cStr_t ATTRIBUTE_TYPE_BOOL;
MINSGAPI extern cStr_t ATTRIBUTE_TYPE_FLOAT;
MINSGAPI extern cStr_t ATTRIBUTE_TYPE_GENERIC;
MINSGAPI extern cStr_t ATTRIBUTE_TYPE_INT;
MINSGAPI extern cStr_t ATTRIBUTE_TYPE_JSON;
MINSGAPI extern cStr_t ATTRIBUTE_TYPE_NUMBER;
MINSGAPI extern cStr_t ATTRIBUTE_TYPE_STRING;
//	@}

// 	------------------------------------------------------

//!	@name TYPE = TYPE_BEHAVIOUR
//	@{
MINSGAPI extern const Util::StringIdentifier ATTR_BEHAVIOUR_TYPE;
//	}

// ----------------------------------

//!	@name Skeletal Animation
//	@{	
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_ANCHORID;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_RIGIDJOINT;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_RIGIDOFFSETMATRIX;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_RIGIDSTACKING;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_JOINT;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_JOINTS;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_SKELETALOBJECT;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_ANIMATIONSAMPLE;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_ARMATURE;
MINSGAPI extern cStr_t STATE_TYPE_SKEL_SKELETALRENDERERSTATE; // deprecated!!!   Still in cause of old MinSG files
MINSGAPI extern cStr_t STATE_TYPE_SKEL_SKELETALHARDWARERENDERERSTATE;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_VERTEX_WEIGHT;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_CONTROLLER;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_SKIN;
MINSGAPI extern cStr_t NODE_TYPE_SKEL_JOINT_STRUCTURE;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_JOINTID;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETON;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_ID_TYPE_ARMATURE_MATRIX;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_TIMELINE;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALSAMPLERDATA;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALINTERPOLATIONTYPE;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONSTARTTIME;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONTARGET;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONNAME;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_INVERSEBINDMATRIX;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_BINDMATRIX;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONPOSEDESCRIPTION;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALSTARTANIMATION;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALTOANIMATIONS;
MINSGAPI extern const Util::StringIdentifier ATTR_SKEL_SKELETALFROMANIMATIONS;
MINSGAPI extern cStr_t BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA;
//	@}

// ----------------------------------

}

}
}
#endif // MINSG_SCENEDESCRIPTION_H
