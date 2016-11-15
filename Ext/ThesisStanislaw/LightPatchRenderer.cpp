/*
	This file is part of the MinSG library.
	Copyright (C) 2016 Stanislaw Eppinger
	Copyright (C) 2016 Sascha Brandt

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#include "LightPatchRenderer.h"

#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Rendering/Texture/PixelFormatGL.h>
#include <Rendering/Texture/TextureType.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/BufferObject.h>
#include <Rendering/Helper.h>
#include <Rendering/RenderingContext/RenderingParameters.h>

#include <Geometry/BoxHelper.h>

#include <Util/Graphics/PixelFormat.h>
#include <Util/TypeConstant.h>


namespace MinSG{
namespace ThesisStanislaw{
using namespace Rendering;
    
LightPatchRenderer::LightPatchRenderer() :
  _lightPatchFBO(nullptr), _samplingWidth(256), _samplingHeight(256), _fboChanged(true),
  _lightPatchTBO(nullptr), _lightPatchShader(nullptr), _approxScene(nullptr), _lightCamera(new CameraNode), _normalTexture(nullptr)
{ 
  float minDistance = 0.1f;
  float maxDistance = 1000.f;
  _lightCamera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
  _lightCamera->setNearFar(minDistance, maxDistance);
}

void LightPatchRenderer::setShader(const std::string& lightPatchShaderFile, const std::string& polygonIdShaderFile){
  _lightPatchShader = Rendering::Shader::loadShader(Util::FileName(lightPatchShaderFile), Util::FileName(lightPatchShaderFile), Rendering::Shader::USE_UNIFORMS);
  _polygonIDWriterShader = Rendering::Shader::loadShader(Util::FileName(polygonIdShaderFile), Util::FileName(polygonIdShaderFile), Rendering::Shader::USE_UNIFORMS);
}

void LightPatchRenderer::initializeFBO(Rendering::RenderingContext& rc){

  _lightPatchFBO = new FBO;
  _depthTextureFBO = TextureUtils::createDepthTexture(_samplingWidth, _samplingHeight);
  _polygonIDTexture = TextureUtils::createDataTexture(TextureType::TEXTURE_2D, _samplingWidth, _samplingHeight, 1, Util::TypeConstant::UINT32, 1);
  
  
  _lightPatchFBO->attachColorTexture(rc, _polygonIDTexture.get(), 0);
  _lightPatchFBO->attachDepthTexture(rc, _depthTextureFBO.get());
  
  /////////////
  _normalTexture = Rendering::TextureUtils::createHDRTexture(_samplingWidth, _samplingHeight, false);
  _lightPatchFBO->attachColorTexture(rc, _normalTexture.get(), 1);
  /////////////
  
  if(!_lightPatchFBO->isComplete(rc)){
    WARN( _lightPatchFBO->getStatusMessage(rc) );
    return;
  }
}

void LightPatchRenderer::allocateLightPatchTBO(){
  auto numTriangles = countTriangles(_approxScene.get());
  _lightPatchTBO = TextureUtils::createDataTexture(TextureType::TEXTURE_BUFFER, numTriangles, 1, 1, Util::TypeConstant::UINT32, 1);
  _tboBindParameters.setTexture(_lightPatchTBO.get());
  _tboBindParameters.setLayer(0);
  _tboBindParameters.setLevel(0);
  _tboBindParameters.setMultiLayer(false);
  _tboBindParameters.setReadOperations(true);
  _tboBindParameters.setWriteOperations(true);
}

bool LightPatchRenderer::computeLightPatches(FrameContext & context, const RenderParam & rp){
  
  if(!_approxScene){
    WARN("No approximated Scene present in LightPatchRenderer!");
    return false;
  }
  
  auto& rc = context.getRenderingContext();
  rc.setImmediateMode(true);
  
  auto format = _lightPatchTBO->getFormat().pixelFormat;
  _lightPatchTBO->_uploadGLTexture(rc);
  _lightPatchTBO->getBufferObject()->clear(format.glInternalFormat, format.glLocalDataFormat, format.glLocalDataType, nullptr);
  //auto glID = _lightPatchTBO->getBufferObject()->getGLId();
  //glClearNamedBufferDataEXT(glID, GL_R32UI, GL_RED, GL_UNSIGNED_INT, 0);
  
  if(_fboChanged){ 
    initializeFBO(rc); 
    _fboChanged = false; 
  }
  
  rc.pushAndSetFBO(_lightPatchFBO.get());
  
  for(uint32_t i = 0; i < _spotLights.size(); i++){
    computeLightMatrix(_spotLights[i].get());
    
    // Write all visible polygonID's on the texture
    _lightPatchFBO->setDrawBuffers(2);
    
    // Clear the uint polygonID Texture
    rc.clearColor({0, 0, 0, 0});
    //GLuint clearColor[] = {0, 0, 0, 0};
    //glClearBufferuiv(GL_COLOR, 0, clearColor);

    rc.pushAndSetShader(_polygonIDWriterShader.get());
    context.pushAndSetCamera(_lightCamera.get());
    rc.clearDepth(1.0f); 
    _approxScene->display(context, rp);
    context.popCamera();
    rc.popShader();
    
    // Take every polygonID in the polygonIDTexture and the corresponding TBO entry to be lit by this light source.
    _lightPatchFBO->setDrawBuffers(0);
    rc.pushAndSetShader(_lightPatchShader.get());
    bindTBO(rc, true, true);
    _lightPatchShader->setUniform(rc, Rendering::Uniform("lightID", static_cast<int32_t>(1<<i))); // ID of light is its index in the vector
    //When removing this line, the normal texture in the photonsampler state is displayed correctly. Otherwise the screen is black.
    Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *_polygonIDTexture.get(), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
    rc.clearDepth(1.0f);
    unbindTBO(rc);
    rc.popShader();
  }
  
  rc.popFBO();
  
  rc.setImmediateMode(false);
  
  return true;
}

void LightPatchRenderer::setApproximatedScene(Node* root){
  _approxScene = root;
  allocateLightPatchTBO();
}

void LightPatchRenderer::setSamplingResolution(uint32_t width, uint32_t height){
  _samplingWidth  = width;
  _samplingHeight = height;
  _fboChanged = true;  
  _lightCamera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
}

void LightPatchRenderer::setSpotLights(std::vector<LightNode*> lights){
  _spotLights.clear();
  _spotLights.assign(lights.begin(), lights.end());
}

void LightPatchRenderer::computeLightMatrix(const MinSG::LightNode* light){  
  float cutoff = light->getCutoff();  
  _lightCamera->setRelTransformation(light->getRelTransformationMatrix());
  _lightCamera->setAngles(-cutoff, cutoff, -cutoff, cutoff);
}

void LightPatchRenderer::bindTBO(Rendering::RenderingContext& rc, bool read, bool write){
  _tboBindParameters.setReadOperations(read);
  _tboBindParameters.setWriteOperations(write);
  rc.pushAndSetBoundImage(0, _tboBindParameters);
}

void LightPatchRenderer::unbindTBO(Rendering::RenderingContext& rc){
  rc.popBoundImage(0);
}

}
}

#endif // MINSG_EXT_THESISSTANISLAW
