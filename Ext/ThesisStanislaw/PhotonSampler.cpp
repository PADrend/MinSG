#ifdef MINSG_EXT_THESISSTANISLAW

#include "PhotonSampler.h"

#include "../../Core/FrameContext.h"

#include "SamplingPatterns/PoissonGenerator.h"


namespace MinSG{
namespace ThesisStanislaw{
  
const std::string PhotonSampler::_shaderPath = "plugins/Effects/resources/PP_Effects";
  
PhotonSampler::PhotonSampler() :
  NodeRendererState(FrameContext::DEFAULT_CHANNEL),
  _fbo(nullptr), _depthTexture(nullptr), _posTexture(nullptr), _normalTexture(nullptr),
  _fboChanged(true), _camera(nullptr), _shader(nullptr), _approxScene(nullptr), _photonNumber(100)
{
  _shader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "PackGeometry.vs"), Util::FileName(_shaderPath + "PackGeometry.fs"), Rendering::Shader::USE_UNIFORMS);
  resample();
}


bool PhotonSampler::initializeFBO(Rendering::RenderingContext& rc){
  if(!_camera) return false;
  
  _fbo = new Rendering::FBO;
  
  auto width = _camera->getWidth();
  auto height = _camera->getHeight();
  
  _depthTexture = Rendering::TextureUtils::createDepthTexture(width, height);
  _posTexture = Rendering::TextureUtils::createHDRTexture(width, height, false);
  _normalTexture = Rendering::TextureUtils::createHDRTexture(width, height, false);
  
  rc.pushAndSetFBO(_fbo.get());
  
  _fbo->attachDepthTexture(rc, _depthTexture.get());
  _fbo->attachColorTexture(rc, _posTexture.get(), 0);
  _fbo->attachColorTexture(rc, _normalTexture.get(), 1);
  
  if(!_fbo->isComplete(rc)){
    WARN( _fbo->getStatusMessage(rc) );
    rc.popFBO();
    return false;
  }
  
  rc.popFBO();
  
  return true;
}

NodeRendererResult PhotonSampler::displayNode(FrameContext & context, Node * node, const RenderParam & rp){
  auto& rc = context.getRenderingContext();
  
  if(_fboChanged){
    if(!initializeFBO(rc)){
      WARN("Could not initialize FBO for PhotonSampler!");
      return NodeRendererResult::PASS_ON;
    }
    _fboChanged = false;
  }
  
  rc.pushAndSetFBO(_fbo.get());
  _fbo->setDrawBuffers(2);
  rc.pushAndSetShader(_shader.get());
  
  if(_camera != nullptr){
    context.pushAndSetCamera(_camera);
    rc.clearDepth(1.0f);

    _approxScene->display(context, rp);
  }

  context.popCamera();
  
  rc.popShader();
  rc.popFBO();

  return NodeRendererResult::PASS_ON;
}

void PhotonSampler::setApproximatedScene(Node* root){
  _approxScene = root;
}

void PhotonSampler::setPhotonNumber(uint32_t number){
  _photonNumber = number;
  resample();
}

void PhotonSampler::setSamplingStrategy(uint8_t type){
  _samplingStrategy = static_cast<Sampling>(type);
  resample();
}

void PhotonSampler::resample(){
  switch(_samplingStrategy){
    case Sampling::POISSON:
    default:{
      Sampler::PoissonGenerator::DefaultPRNG PRNG;
      auto points = Sampler::PoissonGenerator::GeneratePoissonPoints( _photonNumber, PRNG );
      _samplePoints.clear();
      for(auto& point : points){
        _samplePoints.push_back(Geometry::Vec2f(point.x, point.y));
      }
    }
  }
}

void PhotonSampler::setCamera(CameraNode* camera){
  _camera = camera;
  _fboChanged = true;
}

Util::Reference<Rendering::Texture> PhotonSampler::getPosTexture(){
  return _posTexture;
}

Util::Reference<Rendering::Texture> PhotonSampler::getNormalTexture(){
  return _normalTexture;
}

PhotonSampler * PhotonSampler::clone() const {
  return new PhotonSampler(*this);
}

PhotonSampler::~PhotonSampler(){}


}
}


#endif // MINSG_EXT_THESISSTANISLAW
