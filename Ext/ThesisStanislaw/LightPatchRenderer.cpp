#ifdef MINSG_EXT_THESISSTANISLAW

#include "LightPatchRenderer.h"

#include "../../Core/FrameContext.h"
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
  
LightPatchRenderer::LightPatchRenderer() : 
  NodeRendererState(FrameContext::DEFAULT_CHANNEL),
  _lightPatchFBO(nullptr), _samplingWidth(256), _samplingHeight(256), _fboChanged(true),
  _lightPatchTBO(nullptr), _lightPatchShader(nullptr), _approxScene(nullptr)
{
  _lightPatchShader = Rendering::Shader::loadShader(Util::FileName(_shaderPath + "lightPatchEstimation.vs"), Util::FileName(_shaderPath + "lightPatchEstimation.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
  
  _tboBindParameters.setLayer(0);
  _tboBindParameters.setLevel(0);
  _tboBindParameters.setMultiLayer(false);
  _tboBindParameters.setReadOperations(true);
  _tboBindParameters.setWriteOperations(true);
  
  std::vector<Rendering::Uniform> uniforms;
  _lightPatchShader->getActiveUniforms(uniforms);
  
  for(auto& uniform : uniforms){
    std::cout << uniform.toString() << std::endl << std::endl;
  }
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
}

void LightPatchRenderer::allocateLightPatchTBO(){
  if(!_approxScene) return;
  
  auto NONE = Util::PixelFormat::NONE;
  
  auto pf = Util::PixelFormat(Util::TypeConstant::UINT32, 0, NONE, NONE, NONE);
  auto glpf = Rendering::TextureUtils::pixelFormatToGLPixelFormat(pf);
  
  _tboFormat.sizeY = _tboFormat.numLayers = 1;
  _tboFormat.sizeX =  countTriangles(_approxScene);
  _tboFormat.linearMinFilter = _tboFormat.linearMagFilter = false;
  _tboFormat.pixelFormat = glpf;
  
  _lightPatchTBO = new Rendering::Texture(_tboFormat);
  _tboBindParameters.setTexture(_lightPatchTBO.get());
}

NodeRendererResult LightPatchRenderer::displayNode(FrameContext & context, Node * node, const RenderParam & rp){
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
   }
   
   context.popCamera();
  }
  
  rc.popShader();
  rc.popBoundImage(0);
  rc.popFBO();
  
  return NodeRendererResult::PASS_ON;
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
  const Geometry::Vec3f camPos = light->getWorldOrigin();
  const Geometry::Box & box = _approxScene->getWorldBB();
  const Geometry::Vec3f boxCenter = box.getCenter();

  // ##### Fit bounding box into frustum #####
  if(box.contains(camPos)) {
    return nullptr;
  }

  const Geometry::Vec3f camDir = (boxCenter - camPos).getNormalized();

  // Determine the normal of a box side to which the viewing direction is "most" orthogonal.
  Geometry::Vec3 orthoNormal = Geometry::Helper::getNormal(static_cast<Geometry::side_t>(0));
  float minAbsCosAngle = std::abs(orthoNormal.dot(camDir));
  for (uint_fast8_t s = 1; s < 6; ++s) {
    const Geometry::side_t side = static_cast<Geometry::side_t>(s);

    const Geometry::Vec3 & normal = Geometry::Helper::getNormal(side);
    const float absCosAngle = std::abs(normal.dot(camDir));

    if(absCosAngle < minAbsCosAngle) {
      minAbsCosAngle = absCosAngle;
      orthoNormal = normal;
    }
  }

  // Use "best" normal vector for up vector calculation.
  const Geometry::Vec3f camRight = camDir.cross(orthoNormal).normalize();
  const Geometry::Vec3f camUp = camRight.cross(camDir).normalize();

  Util::Reference<CameraNode> camera = new CameraNode;
  camera->setRelTransformation(Geometry::SRT(camPos, -camDir, camUp));

  // Calculate minimum and maximum distance of all bounding box corners to camera.
  const Geometry::Plane camPlane(camPos, camDir);
  float minDistance = std::numeric_limits<float>::max();
  float maxDistance = 0.0f;
  // Calculate maximum angles in four directions between camera direction and all bounding box corners.
  float leftAngle = 0.0f;
  float rightAngle = 0.0f;
  float topAngle = 0.0f;
  float bottomAngle = 0.0f;
  const Geometry::Matrix4x4f cameraMatrix = camera->getWorldTransformationMatrix().inverse();
  for (uint_fast8_t c = 0; c < 8; ++c) {
    const Geometry::corner_t corner = static_cast<Geometry::corner_t>(c);
    const Geometry::Vec3f cornerPos = box.getCorner(corner);

    const float distance = camPlane.planeTest(cornerPos);
    minDistance = std::min(minDistance, distance);
    maxDistance = std::max(maxDistance, distance);

    const Geometry::Vec3f camSpacePoint = cameraMatrix.transformPosition(cornerPos);
    const float horizontalAngle = Geometry::Convert::radToDeg(std::atan(camSpacePoint.getX() / -camSpacePoint.getZ()));
    const float verticalAngle = Geometry::Convert::radToDeg(std::atan(-camSpacePoint.getY() / -camSpacePoint.getZ()));
    if(camSpacePoint.getX() < 0.0f) {
      leftAngle = std::min(leftAngle, horizontalAngle);
    } else {
      rightAngle = std::max(rightAngle, horizontalAngle);
    }
    if(camSpacePoint.getY() < 0.0f) {
      topAngle = std::max(bottomAngle, verticalAngle);
    } else {
      bottomAngle = std::min(topAngle, verticalAngle);
    }
  }

  // Make sure that the near plane is not behind the camera.
  minDistance = std::max(0.001f, minDistance);

  camera->setViewport(Geometry::Rect_i(0, 0, _samplingWidth, _samplingHeight));
  camera->setNearFar(minDistance, maxDistance);
  camera->setAngles(leftAngle, rightAngle, bottomAngle, topAngle);
  
  return camera;
}

}
}

#endif // MINSG_EXT_THESISSTANISLAW
