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
  _indirectLightShader(nullptr), _accumulationShader(nullptr), _approxScene(nullptr), _photonSampler(nullptr), _lightPatchRenderer(nullptr), _photonBufferGLId(0)
{
  _indirectLightShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "indirectLightGathering.vs"), Util::FileName(_shaderPath + "indirectLightGathering.fs"), Rendering::Shader::USE_UNIFORMS);
  _accumulationShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "indirectLightAccumulation.vs"), Util::FileName(_shaderPath + "indirectLightAccumulation.fs"), Rendering::Shader::USE_UNIFORMS);
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
  glBufferData(GL_SHADER_STORAGE_BUFFER, numPhotons * sizeof(float) * 4 * 3, NULL, GL_STATIC_READ);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

State::stateResult_t PhotonRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  if(!_photonSampler || _photonBufferGLId == 0) return State::stateResult_t::STATE_SKIPPED;
  
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
  
  // Clear Buffer
  float clearVal = 0.f;
  glClearNamedBufferData(_photonBufferGLId, GL_R32F, GL_RED, GL_FLOAT, &clearVal);
  
  rc.pushAndSetFBO(_fbo.get());
  rc.clearDepth(1.0f);
  
  rc.applyChanges();
  
  const auto& samplePoints = _photonSampler->getSamplePoints();
  
  for(uint32_t i = 0; i < _photonSampler->getPhotonNumber(); i++){
    
    // Compute indirect light at every pixel
    auto samplePoint = samplePoints[i];
    auto pos = _photonSampler->getPosAt(rc, samplePoint);
    auto normal = _photonSampler->getNormalAt(rc, samplePoint);
//    auto pos = _photonSampler->getPosAt(rc, Geometry::Vec2f(0.5f, 0.5f));
//    auto normal = _photonSampler->getNormalAt(rc, Geometry::Vec2f(0.5f, 0.5f));
//    pos = Geometry::Vec3f(12.5f, 14.89f, -31.0577f);
//    normal = Geometry::Vec3f(0.f , 0.f, -1.f);
    if (normal.isZero()) continue;
    auto photonCamera = computePhotonCamera(pos, normal);
    
    _fbo->setDrawBuffers(2);
    rc.pushAndSetShader(_indirectLightShader.get());
    _lightPatchRenderer->bindTBO(rc, true, false);
    context.pushAndSetCamera(photonCamera.get());
    rc.clearDepth(1.0f);
    rc.clearColor(Util::Color4f(0.f, 0.f, 0.f, 0.f));
//    auto _near = 0.01f;
//    auto _far = 500.f;
//    auto degToRad = [](float deg){const double halfC = M_PI / 180; return static_cast<float>(static_cast<double>(deg) * halfC);};
//    auto persp = Geometry::Matrix4x4::perspectiveProjection(_near * std::tan(degToRad(-70.f)), _near * std::tan(degToRad(70.f)), _near * std::tan(degToRad(-50.f)), _near * std::tan(degToRad(50.f)), _near, _far);
//    _indirectLightShader->setUniform(rc, Rendering::Uniform("perspective", persp));
    _approxScene->display(context, rp);
    
    context.popCamera();
    _lightPatchRenderer->unbindTBO(rc);
    rc.popShader();
    
    
    // Accumulate all pixel values in one photon 
    _fbo->setDrawBuffers(0);
    rc.pushAndSetShader(_accumulationShader.get());
    
    _accumulationShader->setUniform(rc, Rendering::Uniform("photonID", static_cast<int32_t>(i)));
    _accumulationShader->setUniform(rc, Rendering::Uniform("photonPos_ws", pos));
    _accumulationShader->setUniform(rc, Rendering::Uniform("photonNormal_ws", normal));
    bindPhotonBuffer(1);
    rc.clearDepth(1.0f);
    Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *_indirectLightTexture.get(), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
    unbindPhotonBuffer(1);
    
    rc.popShader();
  }
  
  rc.popFBO();
  
  rc.setImmediateMode(false);
  
//  glBindBuffer(GL_SHADER_STORAGE_BUFFER, _photonBufferGLId);
//  GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//  float* ptr = reinterpret_cast<float*>(p);
//  std::cout << *(ptr) <<" "<< *((ptr)++) <<" "<< *((ptr)+2) <<" "<< *((ptr)+3) << std::endl;
//  std::cout << *(ptr+4) <<" "<< *((ptr)+5) <<" "<< *((ptr)+6) <<" "<< *((ptr)+7) << std::endl << std::endl;;
//  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  
//  rc.pushAndSetShader(nullptr);
//  Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *(_indirectLightTexture.get()), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
//  rc.popShader();
//  return State::stateResult_t::STATE_SKIP_RENDERING;
  
  return State::stateResult_t::STATE_OK;
}

void PhotonRenderer::bindPhotonBuffer(unsigned int location){
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, _photonBufferGLId);
}

void PhotonRenderer::unbindPhotonBuffer(unsigned int location){
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, 0);
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
  
  auto srt = Geometry::_SRT<float>();
  srt.translate(pos);
  srt.setRotation(normal * -1.f, Geometry::Vec3f(0.f, 1.f, 0.f));
  
  camera->setRelTransformation(srt);

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
