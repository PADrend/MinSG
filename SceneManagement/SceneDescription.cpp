/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "SceneDescription.h"
#include "../Core/NodeAttributeModifier.h"
#include <Util/AttributeProvider.h>

namespace MinSG {
namespace SceneManagement {
namespace Consts {

const Util::StringIdentifier DATA_BLOCK("_DataBlock_");
const Util::StringIdentifier DEFINITIONS("_Definitions_");

const Util::StringIdentifier TYPE("_Type_");
cStr_t TYPE_ATTRIBUTE="attribute";
cStr_t TYPE_STATE="state";
cStr_t TYPE_NODE="node";
cStr_t TYPE_DATA="data";
cStr_t TYPE_BEHAVIOUR="behaviour";
cStr_t TYPE_SCENE="scene";
cStr_t TYPE_ADDITIONAL_DATA("additional_data");

const Util::StringIdentifier ATTR_SCENE_VERSION("version");

cStr_t NODE_TYPE_GEOMETRY="geometry";
cStr_t NODE_TYPE_CLONE="clone";
cStr_t NODE_TYPE_LIGHT_DIRECTIONAL="lightDirectional";
cStr_t NODE_TYPE_LIGHT_POINT="lightPoint";
cStr_t NODE_TYPE_LIGHT_SPOT="lightSpot";
cStr_t NODE_TYPE_LIGHT="light";
cStr_t NODE_TYPE_LIST="list";
cStr_t NODE_TYPE_CAMERA="camera";

//! Skeletal Animation
const Util::StringIdentifier ATTR_SKEL_ANCHORID("RIGIDANCHORID");
cStr_t NODE_TYPE_SKEL_RIGIDJOINT="RIGIDJOINT";
const Util::StringIdentifier ATTR_SKEL_RIGIDOFFSETMATRIX("RIGIDOFFSETMATRIX");
const Util::StringIdentifier ATTR_SKEL_RIGIDSTACKING("RIGIDSTACKING");
cStr_t NODE_TYPE_SKEL_JOINT="JOINT";
cStr_t NODE_TYPE_SKEL_JOINTS="JOINTS";
cStr_t NODE_TYPE_SKEL_SKELETALOBJECT="skeletalobject";
cStr_t NODE_TYPE_SKEL_ARMATURE="Armature";
cStr_t BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA="AnimationData";
cStr_t NODE_TYPE_SKEL_ANIMATIONSAMPLE="sample";
cStr_t STATE_TYPE_SKEL_SKELETALRENDERERSTATE="SkeletalRendererState";
cStr_t STATE_TYPE_SKEL_SKELETALHARDWARERENDERERSTATE="SkeletalHardwareRendererState";
cStr_t NODE_TYPE_SKEL_VERTEX_WEIGHT="vertex_weights";
cStr_t NODE_TYPE_SKEL_CONTROLLER="controller";
cStr_t NODE_TYPE_SKEL_SKIN="skin";
cStr_t NODE_TYPE_SKEL_JOINT_STRUCTURE="jointstructure";
const Util::StringIdentifier ATTR_SKEL_JOINTID("JointNodeId");
const Util::StringIdentifier ATTR_SKEL_SKELETON("skeleton");
const Util::StringIdentifier ATTR_SKEL_ID_TYPE_ARMATURE_MATRIX("Armature_Matrix");
const Util::StringIdentifier ATTR_SKEL_TIMELINE("Timeline");
const Util::StringIdentifier ATTR_SKEL_SKELETALSAMPLERDATA("SkeletalSampleData");
const Util::StringIdentifier ATTR_SKEL_SKELETALINTERPOLATIONTYPE("SkeletalAnimationInterpolationType");
const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONSTARTTIME("SkeletalAnimationStartTime");
const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONTARGET("SkeletalAnimationTarget");
const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONNAME("SkeletalAnimationName");
const Util::StringIdentifier ATTR_SKEL_INVERSEBINDMATRIX("INV_BIND_MATRIX");
const Util::StringIdentifier ATTR_SKEL_BINDMATRIX("bind_matrix");
const Util::StringIdentifier ATTR_SKEL_SKELETALANIMATIONPOSEDESCRIPTION("pose_description");
const Util::StringIdentifier ATTR_SKEL_SKELETALSTARTANIMATION("SkeletalStartAnimation");
const Util::StringIdentifier ATTR_SKEL_SKELETALTOANIMATIONS("SkeletalAnimationSources");
const Util::StringIdentifier ATTR_SKEL_SKELETALFROMANIMATIONS("SkeletalAnimationTargets");

// ------------------------------------------------------------

cStr_t STATE_TYPE_REFERENCE="ref";
cStr_t STATE_TYPE_ALPHA_TEST="alphaTest";
cStr_t STATE_TYPE_BLENDING="blending";
cStr_t STATE_TYPE_COLOR="color";
cStr_t STATE_TYPE_CULL_FACE="cull_face";
cStr_t STATE_TYPE_GROUP="group";
cStr_t STATE_TYPE_MATERIAL="material";
cStr_t STATE_TYPE_POLYGON_MODE="polygon_mode";
cStr_t STATE_TYPE_SHADER="shader";
cStr_t STATE_TYPE_STATEFULSHADER="statefulShader";
cStr_t STATE_TYPE_TEXTURE="texture";
cStr_t STATE_TYPE_TRANSPARENCY_RENDERER="transparency_renderer";
cStr_t STATE_TYPE_LIGHTING_STATE="lighting_state";
cStr_t STATE_TYPE_SHADER_UNIFORM="shader_uniform";


const Util::StringIdentifier CHILDREN("_Children_");

const Util::StringIdentifier ATTR_DATA_TYPE("type");
const Util::StringIdentifier ATTR_DATA_FORMAT("format");
const Util::StringIdentifier ATTR_DATA_ENCODING("encoding");
cStr_t DATA_ENCODING_BASE64="base64";

const Util::StringIdentifier ATTR_FLAG_CLOSED("closed");

const Util::StringIdentifier ATTR_NODE_TYPE("type");
const Util::StringIdentifier ATTR_NODE_ID("id");
const Util::StringIdentifier ATTR_STATE_ID("id");
const Util::StringIdentifier ATTR_STATE_TYPE("type");
const Util::StringIdentifier ATTR_BEHAVIOUR_TYPE("type");

const Util::StringIdentifier ATTR_REFERENCED_STATE_ID("refId");

const Util::StringIdentifier ATTR_FIXED_BB("fixed_bb");

const Util::StringIdentifier ATTR_SRT_POS("pos");
const Util::StringIdentifier ATTR_SRT_DIR("dir");
const Util::StringIdentifier ATTR_SRT_UP("up");
const Util::StringIdentifier ATTR_SRT_SCALE("scale");

const Util::StringIdentifier ATTR_MATRIX("matrix");


// shader
const Util::StringIdentifier ATTR_SHADER_OBJ_FILENAME("file");
const Util::StringIdentifier ATTR_SHADER_USES_SG_UNIFORMS("usesSGUniforms");
const Util::StringIdentifier ATTR_SHADER_USES_CLASSIC_GL("usesClassicGL");
cStr_t DATA_TYPE_GLSL_FS="shader/glsl_fs";
cStr_t DATA_TYPE_GLSL_VS="shader/glsl_vs";
cStr_t DATA_TYPE_GLSL_GS="shader/glsl_gs";
cStr_t DATA_TYPE_GLSL_USAGE="shader/glsl_usage";
cStr_t DATA_TYPE_SHADER_UNIFORM="uniform";
const Util::StringIdentifier STATE_ATTR_SHADER_NAME(NodeAttributeModifier::create("shaderName",NodeAttributeModifier::PRIVATE_ATTRIBUTE));
const Util::StringIdentifier ATTR_SHADER_NAME("shaderName");
const Util::StringIdentifier ATTR_SHADER_UNIFORM_NAME("name");
const Util::StringIdentifier ATTR_SHADER_UNIFORM_VALUES("values");
const Util::StringIdentifier ATTR_SHADER_UNIFORM_TYPE("dataType");

cStr_t SHADER_UNIFORM_TYPE_BOOL="bool";
cStr_t SHADER_UNIFORM_TYPE_VEC2B="vec2b";
cStr_t SHADER_UNIFORM_TYPE_VEC3B="vec3b";
cStr_t SHADER_UNIFORM_TYPE_VEC4B="vec4b";
cStr_t SHADER_UNIFORM_TYPE_FLOAT="float";
cStr_t SHADER_UNIFORM_TYPE_VEC2F="vec2f";
cStr_t SHADER_UNIFORM_TYPE_VEC3F="vec3f";
cStr_t SHADER_UNIFORM_TYPE_VEC4F="vec4f";
cStr_t SHADER_UNIFORM_TYPE_INT="int";
cStr_t SHADER_UNIFORM_TYPE_VEC2I="vec2i";
cStr_t SHADER_UNIFORM_TYPE_VEC3I="vec3i";
cStr_t SHADER_UNIFORM_TYPE_VEC4I="vec4i";
cStr_t SHADER_UNIFORM_TYPE_MATRIX_2X2F="mat2x2f";
cStr_t SHADER_UNIFORM_TYPE_MATRIX_3X3F="mat3x3f";
cStr_t SHADER_UNIFORM_TYPE_MATRIX_4X4F="mat4x4f";

const Util::StringIdentifier STATE_ATTR_SHADER_FILES(NodeAttributeModifier::create("shaderFiles",NodeAttributeModifier::PRIVATE_ATTRIBUTE));

const Util::StringIdentifier ATTR_ALPHA_TEST_MODE_OLD("func");
const Util::StringIdentifier ATTR_ALPHA_REF_VALUE_OLD("ref");

const Util::StringIdentifier ATTR_ALPHA_TEST_MODE("alpha_func");
const Util::StringIdentifier ATTR_ALPHA_REF_VALUE("ref_value");

const Util::StringIdentifier ATTR_BLEND_EQUATION("equation");
const Util::StringIdentifier ATTR_BLEND_FUNC_SRC("srcFunc");
const Util::StringIdentifier ATTR_BLEND_FUNC_DST("dstFunc");
const Util::StringIdentifier ATTR_BLEND_CONST_ALPHA("constAlpha");
const Util::StringIdentifier ATTR_BLEND_DEPTH_MASK("depthMask");

const Util::StringIdentifier ATTR_TEXTURE_FILENAME("filename");
const Util::StringIdentifier ATTR_TEXTURE_ID("imageId");
const Util::StringIdentifier ATTR_TEXTURE_UNIT("texture_unit");

const Util::StringIdentifier ATTR_MESH_FILENAME("filename");
const Util::StringIdentifier ATTR_MESH_BB("bb");
const Util::StringIdentifier ATTR_MESH_DATA("data");

const Util::StringIdentifier ATTR_ATTRIBUTE_NAME("name");
const Util::StringIdentifier ATTR_ATTRIBUTE_VALUE("value");
const Util::StringIdentifier ATTR_ATTRIBUTE_TYPE("type");

cStr_t ATTRIBUTE_TYPE_BOOL="bool";
cStr_t ATTRIBUTE_TYPE_FLOAT="float";
cStr_t ATTRIBUTE_TYPE_GENERIC="generic";
cStr_t ATTRIBUTE_TYPE_INT="int";
cStr_t ATTRIBUTE_TYPE_JSON="json";
cStr_t ATTRIBUTE_TYPE_NUMBER="Number";
cStr_t ATTRIBUTE_TYPE_STRING="string";

const Util::StringIdentifier ATTR_CLONE_SOURCE("sourceId");

const Util::StringIdentifier ATTR_CAM_NEAR("near");
const Util::StringIdentifier ATTR_CAM_FAR("far");
const Util::StringIdentifier ATTR_CAM_ANGLE("angle");
const Util::StringIdentifier ATTR_CAM_RATIO("ratio");

const Util::StringIdentifier ATTR_LIGHT_TYPE("lighttype");
cStr_t LIGHT_TYPE_POINT = "point";
cStr_t LIGHT_TYPE_DIRECTIONAL = "directional";
cStr_t LIGHT_TYPE_SPOT = "spot";
const Util::StringIdentifier ATTR_LIGHT_AMBIENT("ambient");
const Util::StringIdentifier ATTR_LIGHT_DIFFUSE("diffuse");
const Util::StringIdentifier ATTR_LIGHT_SPECULAR("specular");
const Util::StringIdentifier ATTR_LIGHT_CONSTANT_ATTENUATION("constant_attenuation");
const Util::StringIdentifier ATTR_LIGHT_LINEAR_ATTENUATION("linear_attenuation");
const Util::StringIdentifier ATTR_LIGHT_QUADRATIC_ATTENUATION("quadratic_attenuation");
const Util::StringIdentifier ATTR_LIGHT_SPOT_CUTOFF("spot_cutoff");
const Util::StringIdentifier ATTR_LIGHT_SPOT_EXPONENT("spot_exponent");

const Util::StringIdentifier ATTR_LIGHTING_LIGHT_ID("lightId");

const Util::StringIdentifier ATTR_COLOR_VALUE("value");

const Util::StringIdentifier ATTR_MATERIAL_AMBIENT("ambient");
const Util::StringIdentifier ATTR_MATERIAL_DIFFUSE("diffuse");
const Util::StringIdentifier ATTR_MATERIAL_SPECULAR("specular");
const Util::StringIdentifier ATTR_MATERIAL_SHININESS("shininess");

const Util::StringIdentifier ATTR_TRANSPARENY_USE_PREMULTIPLIED_ALPHA("use_premultimplied_alpha");

const Util::StringIdentifier ATTR_CULL_FACE("faces");
const Util::StringIdentifier ATTR_POLYGON_MODE("polygon_mode");
const Util::StringIdentifier ATTR_POLYGON_MODE_OLD("mode");

}


}
}
