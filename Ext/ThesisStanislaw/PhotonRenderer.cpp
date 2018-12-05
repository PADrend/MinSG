/*
	This file is part of the MinSG library.
	Copyright (C) 2016 Stanislaw Eppinger
	Copyright (C) 2016 Sascha Brandt

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#include "PhotonRenderer.h"
#include "PhotonSampler.h"
#include "LightPatchRenderer.h"

#include "../../Core/FrameContext.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/Transformations.h"

#include <Rendering/Shader/Uniform.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Texture/TextureType.h>

#include <Geometry/Matrix4x4.h>


namespace MinSG{
namespace ThesisStanislaw{
  
PhotonRenderer::PhotonRenderer() :
  _fbo(nullptr), _indirectLightTexture(nullptr), _depthTexture(nullptr), _fboChanged(true),_samplingWidth(64), _samplingHeight(64), 
  _indirectLightShader(nullptr), _accumulationShader(nullptr), _approxScene(nullptr), _photonSampler(nullptr), _lightPatchRenderer(nullptr), 
  _photonCamera(nullptr)
{
  _photonCamera = computePhotonCamera();
}

void PhotonRenderer::setShader(const std::string& gatheringShaderFile, const std::string& accumShaderFile) {  
  _indirectLightShader = Rendering::Shader::loadShader(Util::FileName(gatheringShaderFile), Util::FileName(gatheringShaderFile), Rendering::Shader::USE_UNIFORMS);
  _accumulationShader = Rendering::Shader::loadShader(Util::FileName(accumShaderFile), Util::FileName(accumShaderFile), Rendering::Shader::USE_UNIFORMS);
}

bool PhotonRenderer::initializeFBO(Rendering::RenderingContext& rc) {
  _fbo = new Rendering::FBO;
  
  _depthTexture = Rendering::TextureUtils::createDepthTexture(_samplingWidth, _samplingHeight);
  _indirectLightTexture = Rendering::TextureUtils::createHDRTexture(_samplingWidth, _samplingHeight, true);
  _normalTex = Rendering::TextureUtils::createHDRTexture(_samplingWidth, _samplingHeight, true);
  
  _fbo->attachDepthTexture(rc, _depthTexture.get());
  _fbo->attachColorTexture(rc, _indirectLightTexture.get(), 0);
  _fbo->attachColorTexture(rc, _normalTex.get(), 1);
  
  if(!_fbo->isComplete(rc)){
    WARN( _fbo->getStatusMessage(rc) );
    rc.popFBO();
    return false;
  }
  
  return true;
}

bool PhotonRenderer::gatherLight(FrameContext & context, const RenderParam & rp){

  if(!_photonSampler){
    WARN("No PhotonSampler present in PhotonRenderer!");
    return false;
  }
  
  if(!_approxScene){
    WARN("No approximated Scene present in PhotonRenderer!");
    return false;
  }  
  
  auto& rc = context.getRenderingContext();
  rc.setImmediateMode(true);
  rc.applyChanges();
  
  if(_fboChanged){
    if(!initializeFBO(rc)){
      WARN("Could not initialize FBO for PhotonRenderer!");
      return false;
    }
    _fboChanged = false;
  }
  
  RenderParam newRp;
  
  rc.pushAndSetFBO(_fbo.get());
  rc.clearDepth(1.0f);
  
  rc.applyChanges();
  
  for(int32_t i = 0; i < _photonSampler->getPhotonNumber(); i++){
    _fbo->setDrawBuffers(rc, 2);
    rc.pushAndSetShader(_indirectLightShader.get());
    _lightPatchRenderer->bindTBO(rc, true, false);
    _photonSampler->bindPhotonBuffer(1);
    context.pushAndSetCamera(_photonCamera.get());
    _indirectLightShader->setUniform(rc, Rendering::Uniform("photonID", i));
    
    rc.clearDepth(1.0f);
    rc.clearColor(Util::Color4f(0.f, 0.f, 0.f, 0.f));
    _approxScene->display(context, newRp);
    
    context.popCamera();
    _photonSampler->unbindPhotonBuffer(1);
    _lightPatchRenderer->unbindTBO(rc);
    rc.popShader();    
    
    // Accumulate all pixel values in one photon 
    _fbo->setDrawBuffers(rc, 0);
    rc.pushAndSetShader(_accumulationShader.get());
    _accumulationShader->setUniform(rc, Rendering::Uniform("photonID", i));
    _accumulationShader->setUniform(rc, Rendering::Uniform("samplingWidth", static_cast<int>(_samplingWidth)));
    _accumulationShader->setUniform(rc, Rendering::Uniform("samplingHeight", static_cast<int>(_samplingHeight)));
    _photonSampler->bindPhotonBuffer(1);
    rc.clearDepth(1.0f);
    Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *_indirectLightTexture.get(), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
    _photonSampler->unbindPhotonBuffer(1);
    rc.popShader();
  }
  
  rc.popFBO();
  
  rc.setImmediateMode(false);

  return true;
}

void PhotonRenderer::setPhotonSampler(PhotonSampler* sampler){
  _photonSampler = sampler;
}

void PhotonRenderer::setSamplingResolution(uint32_t width, uint32_t height){
  _samplingWidth = width;
  _samplingHeight = height;
  _fboChanged = true;
  _photonCamera = computePhotonCamera();
}

void PhotonRenderer::setApproximatedScene(Node* root){
  _approxScene = root;
}

void PhotonRenderer::setLightPatchRenderer(LightPatchRenderer* renderer){
  _lightPatchRenderer = renderer;
}

Util::Reference<CameraNode> PhotonRenderer::computePhotonCamera(){
  Util::Reference<CameraNode> camera = new CameraNode;

  float minDistance = 0.01f;
  float maxDistance = 500.f;

  camera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
  camera->setNearFar(minDistance, maxDistance);
  camera->setAngles(-70, 70, -50, 50);
  
  return camera;
}

}
}


#endif // MINSG_EXT_THESISSTANISLAW
