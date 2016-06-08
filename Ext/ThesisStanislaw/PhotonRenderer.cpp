#ifdef MINSG_EXT_THESISSTANISLAW

#include "PhotonRenderer.h"
#include "PhotonSampler.h"
#include "LightPatchRenderer.h"

#include "../../Core/FrameContext.h"
#include "../../Core/States/LightingState.h"

#include "../../../Rendering/Shader/Uniform.h"

#include "../../../Geometry/Matrix4x4.h"


namespace MinSG{
namespace ThesisStanislaw{
  
const std::string PhotonRenderer::_shaderPath = "ThesisStanislaw/ShaderScenes/shader/";
  
PhotonRenderer::PhotonRenderer() :
  State(),
  _fbo(nullptr), _indirectLightTexture(nullptr), _depthTexture(nullptr), _fboChanged(true),_samplingWidth(64), _samplingHeight(64), 
  _indirectLightShader(nullptr), _accumulationShader(nullptr), _approxScene(nullptr), _photonSampler(nullptr), _lightPatchRenderer(nullptr), 
  _photonCamera(nullptr)
{
  _indirectLightShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "indirectLightGathering.vs"), Util::FileName(_shaderPath + "indirectLightGathering.fs"), Rendering::Shader::USE_UNIFORMS);
  _accumulationShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "indirectLightAccumulation.vs"), Util::FileName(_shaderPath + "indirectLightAccumulation.fs"), Rendering::Shader::USE_UNIFORMS);
  _photonCamera = computePhotonCamera();
}

bool PhotonRenderer::initializeFBO(Rendering::RenderingContext& rc){
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

State::stateResult_t PhotonRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  if(!_photonSampler) return State::stateResult_t::STATE_SKIPPED;
  
  auto& rc = context.getRenderingContext();
  rc.setImmediateMode(true);
  rc.applyChanges();
  
  if(_fboChanged){
    if(!initializeFBO(rc)){
      WARN("Could not initialize FBO for PhotonRenderer!");
      return State::stateResult_t::STATE_SKIPPED;
    }
    _fboChanged = false;
  }
  
  rc.pushAndSetFBO(_fbo.get());
  rc.clearDepth(1.0f);
  
  rc.applyChanges();
  
  for(int32_t i = 0; i < 1/*_photonSampler->getPhotonNumber()*/; i++){
    
    _fbo->setDrawBuffers(2);
    rc.pushAndSetShader(_indirectLightShader.get());
    _lightPatchRenderer->bindTBO(rc, true, false);
    _photonSampler->bindPhotonBuffer(1);
    context.pushAndSetCamera(_photonCamera.get());
    _indirectLightShader->setUniform(rc, Rendering::Uniform("photonID", i));
    
    rc.clearDepth(1.0f);
    rc.clearColor(Util::Color4f(0.f, 0.f, 0.f, 0.f));
    _approxScene->display(context, rp);
    
    context.popCamera();
    _photonSampler->unbindPhotonBuffer(1);
    _lightPatchRenderer->unbindTBO(rc);
    rc.popShader();
    
    
    // Accumulate all pixel values in one photon 
    _fbo->setDrawBuffers(0);
    rc.pushAndSetShader(_accumulationShader.get());
    _accumulationShader->setUniform(rc, Rendering::Uniform("photonID", i));
    _photonSampler->bindPhotonBuffer(1);
    rc.clearDepth(1.0f);
    Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *_indirectLightTexture.get(), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
    _photonSampler->unbindPhotonBuffer(1);
    rc.popShader();
  }
  
  rc.popFBO();
  
  rc.setImmediateMode(false);
  
  rc.pushAndSetShader(nullptr);
  Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *(_indirectLightTexture.get()), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
  rc.popShader();
  return State::stateResult_t::STATE_SKIP_RENDERING;
  
//  return State::stateResult_t::STATE_OK;
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

void PhotonRenderer::setSpotLights(std::vector<LightNode*> lights){
  _spotLights = lights;
}


Util::Reference<CameraNode> PhotonRenderer::computePhotonCamera(){
  Util::Reference<CameraNode> camera = new CameraNode;

  float minDistance = 0.01f;
  float maxDistance = 500.f;
  
//  auto srt = Geometry::_SRT<float>();
//  srt.translate(pos);
//  srt.setRotation(normal * -1.f, Geometry::Vec3f(0.f, 1.f, 0.f));
  
//  camera->setRelTransformation(srt);

  camera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
  camera->setNearFar(minDistance, maxDistance);
  camera->setAngles(-70, 70, -50, 50);
  
//  std::cout << "Result" << std::endl;
//  for(int i = 0; i < 4; i++){
//    for(int j = 0; j < 4; j++){
//      std::cout << mat[i * 4 + j] << " ";  
//    }
//    std::cout << std::endl;
//  }
//  std::cout << std::endl;

  return camera;
}

PhotonRenderer * PhotonRenderer::clone() const {
  return new PhotonRenderer(*this);
}

PhotonRenderer::~PhotonRenderer(){}

}
}


#endif // MINSG_EXT_THESISSTANISLAW
