#ifdef MINSG_EXT_THESISSTANISLAW

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


namespace MinSG{
namespace ThesisStanislaw{
  
const std::string LightPatchRenderer::_shaderPath = "ThesisStanislaw/shader/";
  
LightPatchRenderer::LightPatchRenderer() : State(),
  _lightPatchFBO(nullptr), _samplingWidth(256), _samplingHeight(256), _fboChanged(true),
  _lightPatchTBO(nullptr), _lightPatchShader(nullptr), _approxScene(nullptr)
{
  _lightPatchShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "lightPatchEstimation.vs"), Util::FileName(_shaderPath + "lightPatchEstimation.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
  
  _tboBindParameters.setLayer(0);
  _tboBindParameters.setLevel(0);
  _tboBindParameters.setMultiLayer(false);
  _tboBindParameters.setReadOperations(true);
  _tboBindParameters.setWriteOperations(true);
  
  //// Debug output ////
  std::vector<Rendering::Uniform> uniforms;
  _lightPatchShader->getActiveUniforms(uniforms);
  
  for(auto& uniform : uniforms){
    std::cout << uniform.toString() << std::endl << std::endl;
  }
  //////////////////////
}

void LightPatchRenderer::initializeFBO(Rendering::RenderingContext& rc){
  _lightPatchFBO = new Rendering::FBO;
  _depthTextureFBO = Rendering::TextureUtils::createDepthTexture(_samplingWidth, _samplingHeight);
  
  rc.pushAndSetFBO(_lightPatchFBO.get());
  
  _lightPatchFBO->attachDepthTexture(rc, _depthTextureFBO.get());
  
  if(!_lightPatchFBO->isComplete(rc)){
    WARN( _lightPatchFBO->getStatusMessage(rc) );
    rc.popFBO();
    return;
  }
  
  rc.popFBO();
  std::cout << "init FBO" << std::endl;
}

void LightPatchRenderer::allocateLightPatchTBO(){

  auto NONE = Util::PixelFormat::NONE;
  
  auto pf = Util::PixelFormat(Util::TypeConstant::UINT32, 0, NONE, NONE, NONE);
  auto glpf = Rendering::TextureUtils::pixelFormatToGLPixelFormat(pf);
  
  _tboFormat.sizeY = _tboFormat.numLayers = 1;
  _tboFormat.sizeX =  countTriangles(_approxScene);
  _tboFormat.linearMinFilter = _tboFormat.linearMagFilter = false;
  _tboFormat.pixelFormat = glpf;
  
  _lightPatchTBO = new Rendering::Texture(_tboFormat);
  _tboBindParameters.setTexture(_lightPatchTBO.get());
  std::cout << "Allocated TBO" << std::endl;
}

State::stateResult_t LightPatchRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  if(!_approxScene) return State::stateResult_t::STATE_SKIPPED;
  
  auto& rc = context.getRenderingContext();
  
  if(_fboChanged){ 
    initializeFBO(rc); 
    _fboChanged = false; 
  }
  
  rc.pushAndSetFBO(_lightPatchFBO.get());
  rc.pushAndSetShader(_lightPatchShader.get());
  rc.pushAndSetBoundImage(0, _tboBindParameters);
  
  for(uint32_t i = 0; i < _spotLights.size(); i++){
    auto cameraNode = computeLightMatrix(_spotLights[i]);
   
    if(cameraNode.get() != nullptr){
      context.pushAndSetCamera(cameraNode.get());
      rc.clearDepth(1.0f);
      _lightPatchShader->setUniform(rc, Rendering::Uniform("lightID", static_cast<int32_t>(1<<i))); // ID of light is its index in the vector
      _approxScene->display(context, rp);
      context.popCamera();
    }
  }
  
  rc.popShader();
  rc.popBoundImage(0);
  rc.popFBO();
  
  return State::stateResult_t::STATE_OK;
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

Rendering::ImageBindParameters& LightPatchRenderer::getTBOBindParameters(){
  return _tboBindParameters;
}

Util::Reference<CameraNode> LightPatchRenderer::computeLightMatrix(const MinSG::LightNode* light){
  Util::Reference<CameraNode> camera = new CameraNode;

  float minDistance = 0.1f;
  float maxDistance = 1000.f;
  
  float halfCutoff = light->getCutoff()/2.f;
  
  camera->setWorldOrigin(light->getWorldOrigin());
  camera->setWorldTransformation(light->getWorldTransformationSRT());

  camera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
  camera->setNearFar(minDistance, maxDistance);
  camera->setAngles(halfCutoff, halfCutoff, halfCutoff, halfCutoff);
  
  //camera->setRelTransformation(light->getRelTransformationMatrix());
  //camera->setRelOrigin(light->getRelOrigin());
  //camera->setRelScaling(light->getRelScaling());
  
  return camera;
}

}
}

#endif // MINSG_EXT_THESISSTANISLAW
