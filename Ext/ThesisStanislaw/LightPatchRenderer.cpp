#ifdef MINSG_EXT_THESISSTANISLAW

#define LIB_GL
#define LIB_GLEW

#include "LightPatchRenderer.h"

#include "../../Core/FrameContext.h"
#include "../../Core/States/State.h"
#include "../../Helper/StdNodeVisitors.h"

#include "../../../Rendering/Texture/TextureType.h"
#include "../../../Rendering/Texture/TextureUtils.h"
#include "../../../Rendering/Shader/Uniform.h"

#include "../../../Geometry/BoxHelper.h"

#include "../../../Util/Graphics/PixelFormat.h"
#include "../../../Util/TypeConstant.h"
#include "../../../Rendering/GLHeader.h"
#include "../../../Rendering/BufferObject.h"

#include <Rendering/Helper.h>

namespace MinSG{
namespace ThesisStanislaw{
  
const std::string LightPatchRenderer::_shaderPath = "ThesisStanislaw/ShaderScenes/shader/";
  
LightPatchRenderer::LightPatchRenderer() : State(),
  _lightPatchFBO(nullptr), _samplingWidth(256), _samplingHeight(256), _fboChanged(true),
  _lightPatchTBO(nullptr), _lightPatchShader(nullptr), _approxScene(nullptr), _camera(nullptr)
{
  _lightPatchShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "lightPatchEstimation.vs"), Util::FileName(_shaderPath + "lightPatchEstimation.fs"), Rendering::Shader::USE_UNIFORMS);
  _polygonIDWriterShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "polygonIDWriter.vs"), Util::FileName(_shaderPath + "polygonIDWriter.fs"), Rendering::Shader::USE_UNIFORMS);
  
  _tboBindParameters.setLayer(0);
  _tboBindParameters.setLevel(0);
  _tboBindParameters.setMultiLayer(false);
  _tboBindParameters.setReadOperations(true);
  _tboBindParameters.setWriteOperations(true);
}

void LightPatchRenderer::initializeFBO(Rendering::RenderingContext& rc){
  using namespace Rendering;

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
  using namespace Rendering;
  auto numTriangles = countTriangles(_approxScene);
  _lightPatchTBO = TextureUtils::createDataTexture(TextureType::TEXTURE_BUFFER, numTriangles, 1, 1, Util::TypeConstant::UINT32, 1);
  _tboBindParameters.setTexture(_lightPatchTBO.get());
}

State::stateResult_t LightPatchRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
#ifdef MINSG_THESISSTANISLAW_GATHER_STATISTICS
  Rendering::RenderingContext::finish();
  _timer.reset();
#endif // MINSG_THESISSTANISLAW_GATHER_STATISTICS
  
  if(!_approxScene){
    WARN("No approximated Scene present in LightPatchRenderer!");
    return State::stateResult_t::STATE_SKIPPED;
  }
  
  auto& rc = context.getRenderingContext();
  rc.setImmediateMode(true);
  rc.applyChanges();
  
  _lightPatchTBO->_uploadGLTexture(rc);
  auto glID = _lightPatchTBO->getBufferObject()->getGLId();
  glClearNamedBufferDataEXT(glID, GL_R32UI, GL_RED, GL_UNSIGNED_INT, 0);
  GET_GL_ERROR();
  
  if(_fboChanged){ 
    initializeFBO(rc); 
    _fboChanged = false; 
  }
  
  rc.pushAndSetFBO(_lightPatchFBO.get());
  
  for(uint32_t i = 0; i < _spotLights.size(); i++){
    auto cameraNode = computeLightMatrix(_spotLights[i]);
    if(cameraNode.get() != nullptr){
      // Write all visible polygonID's on the texture
      _lightPatchFBO->setDrawBuffers(2);

      rc.pushAndSetShader(_polygonIDWriterShader.get());
      context.pushAndSetCamera(cameraNode.get());
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

      // Clear the uint polygonID Texture
      _lightPatchFBO->setDrawBuffers(2);
      GLuint clearColor[] = {0, 0, 0, 0};
      glClearBufferuiv(GL_COLOR, 0, clearColor);
      rc.applyChanges();
    }
  }
  
  rc.popFBO();
  
  rc.setImmediateMode(false);
  GET_GL_ERROR();
  
//  rc.pushAndSetShader(nullptr);
//  Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight), *(_normalTexture.get()), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
//  rc.popShader();
//  return State::stateResult_t::STATE_SKIP_RENDERING;

#ifdef MINSG_THESISSTANISLAW_GATHER_STATISTICS
  Rendering::RenderingContext::finish();
  _timer.stop();
  auto& stats = context.getStatistics();
  Statistics::instance(stats).addLightPatchTime(stats, _timer.getMilliseconds());
#endif // MINSG_THESISSTANISLAW_GATHER_STATISTICS

  return State::stateResult_t::STATE_OK;
}

void LightPatchRenderer::doDisableState(FrameContext & context,Node *, const RenderParam & rp){
}

LightPatchRenderer * LightPatchRenderer::clone() const {
  return new LightPatchRenderer(*this);
}

LightPatchRenderer::~LightPatchRenderer(){}

void LightPatchRenderer::setApproximatedScene(Node* root){
  _approxScene = root;
  allocateLightPatchTBO();
}

void LightPatchRenderer::setSamplingResolution(uint32_t width, uint32_t height){
  _samplingWidth  = width;
  _samplingHeight = height;
  _fboChanged = true;
}

void LightPatchRenderer::setSpotLights(std::vector<LightNode*> lights){
  _spotLights = lights;
}

Util::Reference<CameraNode> LightPatchRenderer::computeLightMatrix(const MinSG::LightNode* light){
  Util::Reference<CameraNode> camera = new CameraNode;

  float minDistance = 0.1f;
  float maxDistance = 1000.f;
  
  float cutoff = light->getCutoff();
  
  camera->setRelTransformation(light->getRelTransformationMatrix());

  camera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
  camera->setNearFar(minDistance, maxDistance);
  camera->setAngles(-cutoff, cutoff, -cutoff, cutoff);

  return camera;
}

void LightPatchRenderer::bindTBO(Rendering::RenderingContext& rc, bool read, bool write){
  _tboBindParameters.setReadOperations(read);
  _tboBindParameters.setWriteOperations(write);
  rc.pushAndSetBoundImage(0, _tboBindParameters);
}

void LightPatchRenderer::unbindTBO(Rendering::RenderingContext& rc){
  rc.popBoundImage(0);
}

void LightPatchRenderer::setCamera(CameraNode* camera){
  _camera = camera;
}

}
}

#endif // MINSG_EXT_THESISSTANISLAW
