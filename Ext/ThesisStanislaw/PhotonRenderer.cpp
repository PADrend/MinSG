#ifdef MINSG_EXT_THESISSTANISLAW

#define LIB_GL
#define LIB_GLEW

#include "../../../Rendering/GLHeader.h"

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
  _shader(nullptr), _approxScene(nullptr), _photonSampler(nullptr), _lightPatchRenderer(nullptr), _photonBufferGLId(0)
{
  _shader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "photonGathering.vs"), Util::FileName(_shaderPath + "photonGathering.fs"), Rendering::Shader::USE_UNIFORMS);
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

void PhotonRenderer::initializePhotonBuffer(){
  uint32_t numPhotons = _photonSampler->getPhotonNumber();
  
  if(_photonBufferGLId != 0){
    glDeleteBuffers(1, &_photonBufferGLId);
  }

  glGenBuffers(1, &_photonBufferGLId);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, _photonBufferGLId);
  glBufferData(GL_SHADER_STORAGE_BUFFER, numPhotons * sizeof(float) * 4, NULL, GL_STATIC_READ);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

State::stateResult_t PhotonRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  if(!_photonSampler || _photonBufferGLId == 0) return State::stateResult_t::STATE_SKIPPED;
  
  // Clear Buffer
  glClearNamedBufferData(_photonBufferGLId, GL_RGBA32F, GL_RGBA, GL_FLOAT, 0);
  
  auto& rc = context.getRenderingContext();
  
  if(_fboChanged){
    if(!initializeFBO(rc)){
      WARN("Could not initialize FBO for PhotonRenderer!");
      return State::stateResult_t::STATE_SKIPPED;
    }
    _fboChanged = false;
  }
  
  rc.pushAndSetFBO(_fbo.get());
  rc.clearDepth(1.0f);
  _fbo->setDrawBuffers(2);
  rc.pushAndSetShader(_shader.get());
  
  _lightPatchRenderer->bindTBO(rc, true, false);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _photonBufferGLId);
  
  std::vector<LightingState*> lstates;
  
  for(uint32_t i = 0; i < _spotLights.size(); i++){
    auto state = new LightingState(_spotLights[i]);
    lstates.push_back(state);
    _approxScene->addState(state);
  }
  
  const auto& samplePoints = _photonSampler->getSamplePoints();
  
  for(uint32_t i = 0; i < _photonSampler->getPhotonNumber(); i++){
    
    auto samplePoint = samplePoints[i];
    auto pos = _photonSampler->getPosAt(rc, samplePoint);
    auto normal = _photonSampler->getNormalAt(rc, samplePoint);
//    auto pos = _photonSampler->getPosAt(rc, Geometry::Vec2f(0.5f, 0.5f));
//    auto normal = _photonSampler->getNormalAt(rc, Geometry::Vec2f(0.5f, 0.5f));
//    pos = Geometry::Vec3f(12.5f, 14.89f, -31.0577f);
//    normal = Geometry::Vec3f(0.f , 0.f, -1.f);
    auto photonCamera = computePhotonCamera(pos, normal);
    
    context.pushAndSetCamera(photonCamera.get());
    
    //_shader->setUniform(rc, Rendering::Uniform("photonID", static_cast<int32_t>(i)));
    _approxScene->display(context, rp);
    rc.clearDepth(1.0f);
    
    context.popCamera();
  }
  
  for(uint32_t i = 0; i < _spotLights.size(); i++){
    auto state = lstates[i];
    _approxScene->removeState(state);
    //delete state; //Yes, this causes memory leaks, but uncommentig this line causes a crash.
  }
  
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
  _lightPatchRenderer->unbindTBO(rc);
  rc.popShader();
  rc.popFBO();
  
//  rc.pushAndSetShader(nullptr);
//  Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *(_indirectLightTexture.get()), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
//  rc.popShader();
//  return State::stateResult_t::STATE_SKIP_RENDERING;
  
  return State::stateResult_t::STATE_OK;
}

void PhotonRenderer::setPhotonSampler(PhotonSampler* sampler){
  _photonSampler = sampler;
  initializePhotonBuffer();
}

void PhotonRenderer::setSamplingResolution(uint32_t width, uint32_t height){
  _samplingWidth = width;
  _samplingHeight = height;
  _fboChanged = true;
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


Util::Reference<CameraNode> PhotonRenderer::computePhotonCamera(Geometry::Vec3f pos, Geometry::Vec3f normal){
  Util::Reference<CameraNode> camera = new CameraNode;

  float minDistance = 0.01f;
  float maxDistance = 500.f;
  
  Geometry::Matrix4x4f m;
  m.rotateToDirection(normal * -1.0f ).translate(pos);
  
  camera->setRelTransformation(m);

  camera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
  camera->setNearFar(minDistance, maxDistance);
  camera->setAngles(-70, 70, -50, 50);

  return camera;
}

PhotonRenderer * PhotonRenderer::clone() const {
  return new PhotonRenderer(*this);
}

PhotonRenderer::~PhotonRenderer(){}

}
}


#endif // MINSG_EXT_THESISSTANISLAW
