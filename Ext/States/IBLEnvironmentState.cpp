/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "IBLEnvironmentState.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Shader/ShaderObjectInfo.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Draw.h>
#include <Rendering/FBO.h>

static const std::string commonShaderFunctions = R"glsl(#version 330 core
	const float PI = 3.14159265359;
	float radicalInverse_VdC(uint bits) {
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return float(bits) * 2.3283064365386963e-10; // / 0x100000000
	}

	vec2 hammersley(uint i, uint N) {
		return vec2(float(i)/float(N), radicalInverse_VdC(i));
	}

	vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
		float a = roughness*roughness;

		float phi = 2.0 * PI * Xi.x;
		float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
		float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

		// from spherical coordinates to cartesian coordinates
		vec3 H;
		H.x = cos(phi) * sinTheta;
		H.y = sin(phi) * sinTheta;
		H.z = cosTheta;

		// from tangent-space vector to world-space sample vector
		vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
		vec3 tangent   = normalize(cross(up, N));
		vec3 bitangent = cross(N, tangent);

		vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
		return normalize(sampleVec);
	}

	float distributionGGX(float NdotH, float roughness) {
		float a      = roughness*roughness;
		float a2     = a*a;
		float NdotH2 = NdotH*NdotH;
		float num   = a2;
		float denom = (NdotH2 * (a2 - 1.0) + 1.0);
		denom = PI * denom * denom;
		return num / denom;
	}
	
	float geometrySchlickGGX(float NdotV, float roughness) {
		float a = roughness;
		float k = (a * a) / 2.0;

		float nom   = NdotV;
		float denom = NdotV * (1.0 - k) + k;

		return nom / denom;
	}
	
	float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
		float NdotV = max(dot(N, V), 0.0);
		float NdotL = max(dot(N, L), 0.0);
		float ggx2 = geometrySchlickGGX(NdotV, roughness);
		float ggx1 = geometrySchlickGGX(NdotL, roughness);

		return ggx1 * ggx2;
	} 
)glsl";

//--------------------

static const std::string equiToCubeVertShader = R"glsl(#version 330 core
	uniform mat4 sg_matrix_modelToClipping;
	layout(location=0) in vec3 sg_Position;
	out vec3 position_ms;
	void main() {
		position_ms = sg_Position;
		gl_Position = sg_matrix_modelToClipping * vec4(sg_Position, 1.0);
	}
)glsl";

//--------------------

static const std::string equiToCubeFragShader = R"glsl(#version 330 core
	uniform sampler2D equirectangularMap;
	in vec3 position_ms;
	out vec4 fragColor;
	const vec2 invAtan = vec2(0.1591, 0.3183);
	void main() {
		vec3 v = normalize(position_ms);
		vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
		uv *= invAtan;
		uv += 0.5;
		vec3 color = texture(equirectangularMap, uv).rgb;
		fragColor = vec4(color, 1.0);
	}
)glsl";

//--------------------

static const std::string environmentMapVertShader = R"glsl(#version 330 core
	uniform mat4 sg_matrix_modelToCamera;
	uniform mat4 sg_matrix_cameraToClipping;
	layout(location=0) in vec3 sg_Position;
	out vec3 position_ms;
	void main() {
		position_ms = sg_Position;
		mat4 modelToCameraEnv = mat4(mat3(sg_matrix_modelToCamera)); // remove translation from the view matrix
		vec4 position_clip = sg_matrix_cameraToClipping * modelToCameraEnv * vec4(sg_Position, 1.0);
		gl_Position = position_clip.xyww;
	}
)glsl";

//--------------------

static const std::string environmentMapFragShader = R"glsl(#version 330 core
	uniform samplerCube environmentMap;
	uniform float lod = 0.0;
	in vec3 position_ms;
	out vec4 fragColor;
	void main() {
		vec3 color = textureLod(environmentMap, position_ms, lod).rgb;
		color /= (color + vec3(1.0));
		color = pow(color, vec3(1.0/2.2));
		fragColor = vec4(color, 1.0);
	}
)glsl";

//--------------------

static const std::string irrConvFragShader = R"glsl(#version 330 core
	uniform samplerCube environmentMap;
	in vec3 position_ms;
	out vec4 fragColor;
	const float PI = 3.14159265359;
	void main() {
		vec3 normal = normalize(position_ms);
		vec3 irradiance = vec3(0.0);

		vec3 up = vec3(0.0, 1.0, 0.0);
		vec3 right = normalize(cross(up, normal));
		up = normalize(cross(normal, right));

		float sampleDelta = 0.025;
		float nrSamples = 0.0;
		for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
			for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
				// spherical to cartesian (in tangent space)
				vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
				// tangent space to world
				vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

				irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
				nrSamples++;
			}
		}
		irradiance = PI * irradiance * (1.0 / float(nrSamples));

		fragColor = vec4(irradiance, 1.0);
	}
)glsl";

//--------------------

static const std::string prefilterEnvFragShader = R"glsl(#version 330 core
	uniform samplerCube environmentMap;
	uniform float baseResolution;
	uniform float roughness;
	in vec3 position_ms;
	out vec4 fragColor;
	const float PI = 3.14159265359;

	vec2 hammersley(uint i, uint N);
	vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness);
	float distributionGGX(float NdotH, float roughness);

	void main() {
		vec3 N = normalize(position_ms);
		vec3 R = N;
		vec3 V = R;

		const uint SAMPLE_COUNT = 1024u;
		float totalWeight = 0.0;
		vec3 prefilteredColor = vec3(0.0);
		for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
			vec2 Xi = hammersley(i, SAMPLE_COUNT);
			vec3 H  = importanceSampleGGX(Xi, N, roughness);
			vec3 L  = normalize(2.0 * dot(V, H) * H - V);
			float NdotL = max(dot(N, L), 0.0);
			float NdotH  = max(dot(N, H), 0.0);
			float HdotV = max(dot(H, V), 0.0);

			float D   = distributionGGX(NdotH, roughness);
			float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;
			float saTexel  = 4.0 * PI / (6.0 * baseResolution * baseResolution);
			float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

			float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
			if(NdotL > 0.0) {
				prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
				totalWeight      += NdotL;
			}
		}
		prefilteredColor = prefilteredColor / totalWeight;

		fragColor = vec4(prefilteredColor, 1.0);
	}
)glsl";

static const std::string brdfLutVertShader = R"glsl(#version 330 core
	uniform mat4 sg_matrix_modelToClipping;
	layout(location=0) in vec3 sg_Position;
	layout(location=1) in vec2 sg_TexCoord0;
	out vec2 texCoords;
	void main() {
		texCoords = sg_TexCoord0;
		gl_Position = sg_matrix_modelToClipping * vec4(sg_Position, 1.0);
	}
)glsl";

//--------------------

static const std::string brdfLutFragShader = R"glsl(#version 330 core
	in vec2 texCoords;
	out vec2 fragColor;

	vec2 hammersley(uint i, uint N);
	vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness);
	float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness);

	vec2 integrateBRDF(float NdotV, float roughness) {
		vec3 V;
		V.x = sqrt(1.0 - NdotV*NdotV);
		V.y = 0.0;
		V.z = NdotV;
		
		float A = 0.0;
		float B = 0.0;

		vec3 N = vec3(0.0, 0.0, 1.0);

		const uint SAMPLE_COUNT = 1024u;
		for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
			vec2 Xi = hammersley(i, SAMPLE_COUNT);
			vec3 H  = importanceSampleGGX(Xi, N, roughness);
			vec3 L  = normalize(2.0 * dot(V, H) * H - V);

			float NdotL = max(L.z, 0.0);
			float NdotH = max(H.z, 0.0);
			float VdotH = max(dot(V, H), 0.0);

			if(NdotL > 0.0) {
				float G = geometrySmith(N, V, L, roughness);
				float G_Vis = (G * VdotH) / (NdotH * NdotV);
				float Fc = pow(1.0 - VdotH, 5.0);

				A += (1.0 - Fc) * G_Vis;
				B += Fc * G_Vis;
			}
		}
		A /= float(SAMPLE_COUNT);
		B /= float(SAMPLE_COUNT);
		return vec2(A, B);
	}

	void main() {
		vec2 integratedBRDF = integrateBRDF(texCoords.x, texCoords.y);
		fragColor = integratedBRDF;
	}
)glsl";

//--------------------

//--------------------

namespace MinSG {
using namespace Rendering;
using namespace Geometry;

static const Box unitCube({0.0f, 0.0f, 0.0f}, 1.0f);
static const Vec3 dirs[] = {
	{ 1.0f,  0.0f,  0.0f},
	{-1.0f,  0.0f,  0.0f},
	{ 0.0f, -1.0f,  0.0f},
	{ 0.0f,  1.0f,  0.0f},
	{ 0.0f,  0.0f,  1.0f},
	{ 0.0f,  0.0f, -1.0f}
};
static const Vec3 ups[] = {
	{0.0f, -1.0f,  0.0f},
	{0.0f, -1.0f,  0.0f},
	{0.0f,  0.0f, -1.0f},
	{0.0f,  0.0f,  1.0f},
	{0.0f, -1.0f,  0.0f},
	{0.0f, -1.0f,  0.0f}
};

//--------------------


//! [ctor]
IBLEnvironmentState::IBLEnvironmentState() : State(), environmentShader(Shader::createShader(environmentMapVertShader, environmentMapFragShader)) {
}

//--------------------

//! [dtor]
IBLEnvironmentState::~IBLEnvironmentState() = default;

//--------------------

IBLEnvironmentState * IBLEnvironmentState::clone() const {
	return new IBLEnvironmentState(*this);
}

//--------------------

void IBLEnvironmentState::loadEnvironmentMapFromHDR(const Util::FileName& filename) {
	hdrFile = filename;
	hdrEquirectangularMap = Serialization::loadTexture(filename);
}

//--------------------

void IBLEnvironmentState::setEnvironmentMap(const Util::Reference<Rendering::Texture>& texture) {
	hdrEquirectangularMap = nullptr;
	hdrFile = Util::FileName{};
	environmentMap = texture;
	irradianceMap = nullptr;
	prefilteredEnvMap = nullptr;
}

//--------------------

void IBLEnvironmentState::buildCubeMapFromEquirectangularMap(FrameContext& context) {
	auto& rc = context.getRenderingContext();
	
	Util::Reference<CameraNode> camera = new CameraNode;
	camera->setViewport({0,0,static_cast<int32_t>(resolution),static_cast<int32_t>(resolution)}, true);
	camera->setNearFar(0.1f, 2.0f);
	camera->applyHorizontalAngle(90.0f);

	environmentMap = TextureUtils::createHDRCubeTexture(resolution, false);
	//auto depthTexture = TextureUtils::createDepthTexture(resolution, resolution);
	Util::Reference<FBO> fbo = new FBO;
	//fbo->attachDepthTexture(rc, depthTexture.get());
	Util::Reference<Shader> shader = Shader::createShader(equiToCubeVertShader, equiToCubeFragShader);
	context.pushCamera();
	rc.pushAndSetShader(shader.get());
	rc.pushAndSetFBO(fbo.get());
	rc.pushAndSetTexture(0, hdrEquirectangularMap.get());
	rc.pushAndSetCullFace(CullFaceParameters::CULL_FRONT);
	rc.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::ALWAYS));
	for(uint32_t layer=0; layer<6; ++layer) {
		Matrix4x4 mat;
		mat.lookAt({0.0f, 0.0f, 0.0f}, dirs[layer], ups[layer]),
		fbo->attachColorTexture(rc, environmentMap.get(), 0, 0, layer);
		camera->setRelTransformation(mat);
		context.setCamera(camera.get());
		rc.clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
		drawBox(rc, unitCube);
	}
	rc.popDepthBuffer();
	rc.popCullFace();
	rc.popTexture(0);
	rc.popFBO();
	rc.popShader();
	context.popCamera();
	environmentMap->createMipmaps(rc);
	irradianceMap = nullptr;
	prefilteredEnvMap = nullptr;
	hdrEquirectangularMap = nullptr;
}

//--------------------

void IBLEnvironmentState::buildIrradianceMap(FrameContext& context) {
	auto& rc = context.getRenderingContext();
	
	Util::Reference<CameraNode> camera = new CameraNode;
	camera->setViewport({0,0,static_cast<int32_t>(irrResolution),static_cast<int32_t>(irrResolution)}, true);
	camera->setNearFar(0.1f, 2.0f);
	camera->applyHorizontalAngle(90.0f);

	irradianceMap = TextureUtils::createHDRCubeTexture(irrResolution, false);
	//auto depthTexture = TextureUtils::createDepthTexture(irrResolution, irrResolution);
	Util::Reference<FBO> fbo = new FBO;
	//fbo->attachDepthTexture(rc, depthTexture.get());
	Util::Reference<Shader> shader = Shader::createShader(environmentMapVertShader, irrConvFragShader);
	context.pushCamera();
	rc.pushAndSetShader(shader.get());
	rc.pushAndSetFBO(fbo.get());
	rc.pushAndSetTexture(0, environmentMap.get());
	rc.pushAndSetCullFace(CullFaceParameters::CULL_FRONT);
	rc.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::ALWAYS));
	for(uint32_t layer=0; layer<6; ++layer) {
		Matrix4x4 mat;
		mat.lookAt({0.0f, 0.0f, 0.0f}, dirs[layer], ups[layer]),
		fbo->attachColorTexture(rc, irradianceMap.get(), 0, 0, layer);
		camera->setRelTransformation(mat);
		context.setCamera(camera.get());
		rc.clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
		drawBox(rc, unitCube);
	}
	rc.popDepthBuffer();
	rc.popCullFace();
	rc.popTexture(0);
	rc.popFBO();
	rc.popShader();
	context.popCamera();
}

//--------------------

void IBLEnvironmentState::buildPrefilteredEnvMap(FrameContext& context) {
	auto& rc = context.getRenderingContext();
	uint32_t maxMipLevels = 5;
	
	Util::Reference<CameraNode> camera = new CameraNode;
	camera->setViewport({0,0,static_cast<int32_t>(prefilterResolution),static_cast<int32_t>(prefilterResolution)}, true);
	camera->setNearFar(0.1f, 2.0f);
	camera->applyHorizontalAngle(90.0f);

	prefilteredEnvMap = TextureUtils::createHDRCubeTexture(prefilterResolution, false);
	prefilteredEnvMap->createMipmaps(rc);
	//auto depthTexture = TextureUtils::createDepthTexture(prefilterResolution, prefilterResolution);
	//depthTexture->createMipmaps(rc);

	Util::Reference<FBO> fbo = new FBO;
	Util::Reference<Shader> shader = Shader::createShader(environmentMapVertShader, prefilterEnvFragShader);
	shader->attachShaderObject(ShaderObjectInfo::createFragment(commonShaderFunctions));
	context.pushCamera();
	rc.pushAndSetShader(shader.get());
	rc.pushAndSetFBO(fbo.get());
	rc.pushAndSetTexture(0, environmentMap.get());
	rc.pushAndSetCullFace(CullFaceParameters::CULL_FRONT);
	rc.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::ALWAYS));
	shader->setUniform(rc, Uniform("baseResolution", static_cast<float>(resolution)));
	for(uint32_t mip=0; mip < maxMipLevels; ++mip) {
		int32_t mipResolution = static_cast<int32_t>(prefilterResolution * std::pow(0.5, mip));
		float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
		shader->setUniform(rc, Uniform("roughness", roughness));
		camera->setViewport({0,0,mipResolution,mipResolution}, true);
		//fbo->attachDepthTexture(rc, depthTexture.get(), mip);
		for(uint32_t layer=0; layer<6; ++layer) {
			Matrix4x4 mat;
			mat.lookAt({0.0f, 0.0f, 0.0f}, dirs[layer], ups[layer]),
			fbo->attachColorTexture(rc, prefilteredEnvMap.get(), 0, mip, layer);
			camera->setRelTransformation(mat);
			context.setCamera(camera.get());
			rc.clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
			drawBox(rc, unitCube);
		}
	}
	rc.popDepthBuffer();
	rc.popCullFace();
	rc.popTexture(0);
	rc.popFBO();
	rc.popShader();
	context.popCamera();
}

//--------------------

void IBLEnvironmentState::buildBrdfLUT(FrameContext& context) {
	auto& rc = context.getRenderingContext();
	
	brdfLUT = TextureUtils::createColorTexture(TextureType::TEXTURE_2D, brdfLutResolution, brdfLutResolution, 1, Util::TypeConstant::FLOAT, 2, true, true);
	
	Util::Reference<FBO> fbo = new FBO;
	fbo->attachColorTexture(rc, brdfLUT.get(), 0);
	Util::Reference<Shader> shader = Shader::createShader(brdfLutVertShader, brdfLutFragShader);
	shader->attachShaderObject(ShaderObjectInfo::createFragment(commonShaderFunctions));
	rc.pushAndSetShader(shader.get());
	rc.pushAndSetFBO(fbo.get());
	rc.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::ALWAYS));
	rc.pushAndSetViewport({0,0,static_cast<int32_t>(brdfLutResolution),static_cast<int32_t>(brdfLutResolution)});
	rc.clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
	drawFullScreenRect(rc);
	rc.popDepthBuffer();
	rc.popFBO();
	rc.popShader();
}

//--------------------

//! ---|> [State]
State::stateResult_t IBLEnvironmentState::doEnableState(FrameContext & context, Node * /*node*/, const RenderParam & rp) {
	auto& rc = context.getRenderingContext();
	if(environmentMap.isNull() && !hdrFile.empty()) {
		buildCubeMapFromEquirectangularMap(context);
	}
	if(brdfLUT.isNull()) {
		buildBrdfLUT(context);
	}
	if(environmentMap.isNotNull() && irradianceMap.isNull()) {
		buildIrradianceMap(context);
	}
	if(environmentMap.isNotNull() && prefilteredEnvMap.isNull()) {
		buildPrefilteredEnvMap(context);
		environmentMap->downloadGLTexture(rc);
	}
	if(!environmentMap)
		return State::STATE_SKIPPED;

	if(drawEnvironmentMap && environmentMap.isNotNull()) {
		rc.pushAndSetShader(environmentShader.get());
		environmentShader->setUniform(rc, Uniform("lod", lod));
		rc.pushAndSetTexture(0, environmentMap.get());
		rc.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, false, Rendering::Comparison::LEQUAL));
		rc.pushAndSetCullFace(CullFaceParameters::CULL_FRONT);
		drawBox(rc, unitCube);
		rc.popCullFace();
		rc.popDepthBuffer();
		rc.popTexture(0);
		rc.popShader();
	}

	rc.setGlobalUniform(Uniform("sg_envEnabled", true));
	rc.setGlobalUniform(Uniform("sg_irradianceMap", static_cast<int32_t>(baseTextureUnit)));
	rc.setGlobalUniform(Uniform("sg_prefilteredEnvMap", static_cast<int32_t>(baseTextureUnit+1)));
	rc.setGlobalUniform(Uniform("sg_brdfLUT", static_cast<int32_t>(baseTextureUnit+2)));
	rc.pushAndSetTexture(baseTextureUnit, irradianceMap.get());
	rc.pushAndSetTexture(baseTextureUnit+1, prefilteredEnvMap.get());
	rc.pushAndSetTexture(baseTextureUnit+2, brdfLUT.get());

	return State::STATE_OK;
}

//--------------------

//! ---|> [State]
void IBLEnvironmentState::doDisableState(FrameContext & context, Node * /*node*/, const RenderParam & rp) {
	auto& rc = context.getRenderingContext();
	rc.setGlobalUniform(Uniform("sg_envEnabled", false));
	rc.popTexture(baseTextureUnit);
	rc.popTexture(baseTextureUnit+1);
	rc.popTexture(baseTextureUnit+2);
}

//--------------------

void IBLEnvironmentState::generateFromScene(FrameContext& context, Node* node, const RenderParam& rp) {
	auto& rc = context.getRenderingContext();
	deactivate();
	
	Util::Reference<CameraNode> camera = new CameraNode;
	camera->setViewport({0,0,static_cast<int32_t>(resolution),static_cast<int32_t>(resolution)}, true);
	camera->setNearFar(context.getCamera()->getNearPlane(), context.getCamera()->getFarPlane());
	camera->applyHorizontalAngle(90.0f);

	environmentMap = TextureUtils::createHDRCubeTexture(resolution, false);
	auto depthTexture = TextureUtils::createDepthTexture(resolution, resolution);
	Util::Reference<FBO> fbo = new FBO;
	fbo->attachDepthTexture(rc, depthTexture.get());
	
	context.pushCamera();
	rc.pushAndSetFBO(fbo.get());
	for(uint32_t layer=0; layer<6; ++layer) {
		Matrix4x4 mat;
		mat.lookAt({0.0f, 0.0f, 0.0f}, dirs[layer], ups[layer]),
		fbo->attachColorTexture(rc, environmentMap.get(), 0, 0, layer);
		camera->setRelTransformation(mat);
		context.setCamera(camera.get());
		rc.clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
		context.displayNode(node, rp);
	}
	rc.popFBO();
	context.popCamera();
	environmentMap->createMipmaps(rc);
	irradianceMap = nullptr;
	prefilteredEnvMap = nullptr;
	hdrEquirectangularMap = nullptr;
	hdrFile = Util::FileName{};
	activate();
}

//--------------------

}
