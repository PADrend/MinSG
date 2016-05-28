#ifdef MINSG_EXT_THESISSTANISLAW

#define LIB_GL
#define LIB_GLEW

#include "../../../Rendering/GLHeader.h"

#include "PhotonRenderer.h"
#include "PhotonSampler.h"
#include "LightPatchRenderer.h"

#include "../../Core/FrameContext.h"

#include "../../../Rendering/Shader/Uniform.h"

#include "SamplingPatterns/PoissonGenerator.h"


namespace MinSG{
namespace ThesisStanislaw{
  
const std::string PhotonRenderer::_shaderPath = "ThesisStanislaw/shader/";
  
PhotonRenderer::PhotonRenderer() :
  State(),
  _fbo(nullptr), _depthTexture(nullptr), _fboChanged(true),_samplingWidth(64), _samplingHeight(64), _shader(nullptr), _approxScene(nullptr),
  _photonSampler(nullptr), _lightPatchRenderer(nullptr), _photonBufferGLId(0)
{
  _shader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "photonGathering.vs"), Util::FileName(_shaderPath + "photonGathering.fs"), Rendering::Shader::USE_UNIFORMS);
}


bool PhotonRenderer::initializeFBO(Rendering::RenderingContext& rc){
  
  _fbo = new Rendering::FBO;
  
  _depthTexture = Rendering::TextureUtils::createDepthTexture(_samplingWidth, _samplingHeight);
  
  rc.pushAndSetFBO(_fbo.get());
  
  _fbo->attachDepthTexture(rc, _depthTexture.get());
  
  if(!_fbo->isComplete(rc)){
    WARN( _fbo->getStatusMessage(rc) );
    rc.popFBO();
    return false;
  }
  
  rc.popFBO();
  
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
  rc.pushAndSetShader(_shader.get());
  
  _lightPatchRenderer->bindTBO(rc, true, false);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _photonBufferGLId);
  
  const auto& samplePoints = _photonSampler->getSamplePoints();
  
  for(uint32_t i = 0; i < _photonSampler->getPhotonNumber(); i++){
    rc.clearDepth(1.0f);
    
    auto samplePoint = samplePoints[i];
    auto pos = _photonSampler->getPosAt(samplePoint);
    auto normal = _photonSampler->getNormalAt(samplePoint);
    auto photonCamera = computePhotonCamera(pos, normal);
    
    context.pushAndSetCamera(photonCamera.get());
    
    _shader->setUniform(rc, Rendering::Uniform("photonID", static_cast<int32_t>(i)));
    _approxScene->display(context, rp);
    
    context.popCamera();
  }
  
  rc.popShader();
  rc.popFBO();
  
  return State::stateResult_t::STATE_OK;
}

void PhotonRenderer::setPhotonSampler(PhotonSampler* sampler){
  _photonSampler = sampler;
  initializePhotonBuffer();
}

void PhotonRenderer::setSamplingResolution(uint32_t width, uint32_t height){
  _samplingWidth = width;
  _samplingHeight = height;
}

void PhotonRenderer::setApproximatedScene(Node* root){
  _approxScene = root;
}

void PhotonRenderer::setLightPatchRenderer(LightPatchRenderer* renderer){
  _lightPatchRenderer = renderer;
}


Util::Reference<CameraNode> PhotonRenderer::computePhotonCamera(Geometry::Vec3f pos, Geometry::Vec3f normal){
  Util::Reference<CameraNode> camera = new CameraNode;

  float minDistance = 0.01f;
  float maxDistance = 500.f;
  
  // TODO: Compute RelTransformation with "pos" and "normal"
  // camera->setRelTransformation();

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
