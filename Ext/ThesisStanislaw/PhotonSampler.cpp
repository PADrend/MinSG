#ifdef MINSG_EXT_THESISSTANISLAW

#include "PhotonSampler.h"

#include "../../Core/FrameContext.h"

#include "../../../Rendering/Shader/Uniform.h"

#include "SamplingPatterns/PoissonGenerator.h"


namespace MinSG{
namespace ThesisStanislaw{
  
const std::string PhotonSampler::_shaderPath = "plugins/Effects/resources/PP_Effects/";
  
PhotonSampler::PhotonSampler() :
  State(),
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

State::stateResult_t PhotonSampler::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  auto& rc = context.getRenderingContext();
  
  if(_fboChanged){
    if(!initializeFBO(rc)){
      WARN("Could not initialize FBO for PhotonSampler!");
      return State::stateResult_t::STATE_SKIPPED;
    }
    _fboChanged = false;
  }
  
  rc.pushAndSetFBO(_fbo.get());
  _fbo->setDrawBuffers(2);
  rc.pushAndSetShader(_shader.get());
  
  if(_camera != nullptr){
    context.pushAndSetCamera(_camera);
    _shader->setUniform(rc, Rendering::Uniform("sg_useMaterials", false));
    rc.clearDepth(1.0f);
    rc.clearColor(Util::Color4f(0.f, 0.f, 0.f));

    _approxScene->display(context, rp);
  }
  
  context.popCamera();
  
  rc.popShader();
  rc.popFBO();
  
//  rc.pushAndSetShader(nullptr);
//  auto width = _camera->getWidth();
//  auto height = _camera->getHeight();
//  Rendering::TextureUtils::drawTextureToScreen(rc, Geometry::Rect_i(0, 0, width, height), *(_normalTexture.get()), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
//  rc.popShader();
//  return State::stateResult_t::STATE_SKIP_RENDERING;
  
  return State::stateResult_t::STATE_OK;
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
  
  auto imageSize = static_cast<size_t>(std::ceil(std::sqrt(_photonNumber)));
  std::vector<std::tuple<float, float, size_t>> fPoints;
  for (size_t idx = 0; idx < _samplePoints.size(); idx++) {
    fPoints.push_back(std::make_tuple(_samplePoints[idx].x(), _samplePoints[idx].y(), idx));
  }
  
  computeSamplingImage(fPoints, imageSize);
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

float PhotonSampler::distance(std::pair<float, float> p1, std::pair<float, float> p2) {
  return static_cast<float>(std::sqrt((p1.first - p2.first) * (p1.first - p2.first) + (p1.second - p2.second) * (p1.second - p2.second)));
}

std::vector<size_t> PhotonSampler::getPointsInQuad(std::vector<std::tuple<float, float, size_t>>& Points, float x1, float y1, float x2, float y2) {
  using std::get;
  std::vector<size_t> ret;

  for (size_t i = 0; i < Points.size(); i++) {
    auto point = Points[i];
    if (get<0>(point) >= x1  && get<0>(point) <= x2 && get<1>(point) >= y1 && get<1>(point) <= y2) {
      ret.push_back(i);
    }
  }

  return ret;
}

std::pair<size_t, size_t> PhotonSampler::checkNeighbours(std::vector<std::tuple<float, float, size_t>>& Points, float x1, float y1, float x2, float y2, float offset, size_t x, size_t y, size_t& idxMovingPoint, size_t& idxStayingPoint) {
  using std::get;
  using std::make_pair;

  float dist = 2.f;
  auto points = getPointsInQuad(Points, x1, y1, x2, y2);
  auto retX = x;
  auto retY = y;

  auto dp1 = distance(make_pair(x1 + (x2-x1)/2.f, y1 + (y2-y1)/2.f), make_pair(get<0>(Points[points[0]]), get<1>(Points[points[0]])) );
  auto dp2 = distance(make_pair(x1 + (x2 - x1) / 2.f, y1 + (y2 - y1) / 2.f), make_pair(get<0>(Points[points[1]]), get<1>(Points[points[1]])));

  auto movingPoint = make_pair(get<0>(Points[points[0]]), get<1>(Points[points[0]]));
  idxMovingPoint = points[0];
  idxStayingPoint = points[1];
  if (dp1 > dp2) {
    movingPoint = make_pair(get<0>(Points[points[1]]), get<1>(Points[points[1]]));
    idxMovingPoint = points[1];
    idxStayingPoint = points[0];
  }

  /*
  x o o
  o o o
  o o o
  */
  if (x1 - offset >= 0.f && y2 + offset <= 1.f) 
  {
    auto pINq = getPointsInQuad(Points, x1 - offset, y1 + offset, x2 - offset, y2 + offset);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair( x1 - offset + (x2 - x1) / 2.f, y1 + offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x - 1;
        retY = y + 1;
      }
    }
  }

  /*
  o x o
  o o o
  o o o
  */
  if (y2 + offset <= 1.f)
  {
    auto pINq = getPointsInQuad(Points, x1, y1 + offset, x2, y2 + offset);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair(x1 + (x2 - x1) / 2.f, y1 + offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x;
        retY = y + 1;
      }
    }
  }

  /*
  o o x
  o o o
  o o o
  */
  if (x2 + offset <= 1.f && y2 + offset <= 1.f)
  {
    auto pINq = getPointsInQuad(Points, x1 + offset, y1 + offset, x2 + offset, y2 + offset);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair(x1 + offset + (x2 - x1) / 2.f, y1 + offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x + 1;
        retY = y + 1;
      }
    }
  }

  /*
  o o o
  x o o
  o o o
  */
  if (x1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1 - offset, y1, x2 - offset, y2);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair(x1 - offset + (x2 - x1) / 2.f, y1 + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x - 1;
        retY = y;
      }
    }
  }

  /*
  o o o
  o o x
  o o o
  */
  if (x2 + offset <= 1.f)
  {
    auto pINq = getPointsInQuad(Points, x1 + offset, y1, x2 + offset, y2);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair(x1 + offset + (x2 - x1) / 2.f, y1 + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x + 1;
        retY = y;
      }
    }
  }

  /*
  o o o
  o o o
  x o o
  */
  if (x1 - offset >= 0.f && y1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1 - offset, y1 - offset, x2 - offset, y2 - offset);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair(x1 - offset + (x2 - x1) / 2.f, y1 - offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x - 1;
        retY = y - 1;
      }
    }
  }

  /*
  o o o
  o o o
  o x o
  */
  if (y1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1, y1 - offset, x2, y2 - offset);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair(x1 + (x2 - x1) / 2.f, y1 - offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x;
        retY = y - 1;
      }
    }
  }

  /*
  o o o
  o o o
  o o x
  */
  if (x2 + offset <= 1.f && y1 - offset >= 0.f)
  {
    auto pINq = getPointsInQuad(Points, x1 + offset, y1 - offset, x2 + offset, y2 - offset);
    if (pINq.size() == 0) {
      auto thisDist = distance(make_pair(x1 + offset + (x2 - x1) / 2.f, y1 - offset + (y2 - y1) / 2.f), movingPoint);
      if (thisDist < dist) {
        dist = thisDist;
        retX = x + 1;
        retY = y - 1;
      }
    }
  }

  return make_pair(retX, retY);

}

std::vector<int> PhotonSampler::computeSamplingImage(std::vector<std::tuple<float, float, size_t>> Points, size_t size) {
  using std::get;

  std::vector<int> image; 
  image.resize(size * size);
  image.reserve(size * size);

  float offset = 1.0f / static_cast<float>(size);
  float x1, y1, x2, y2;

  x1 = y1 = 0.f;
  x2 = y2 = offset;

  for (size_t y = 0; y < size - 1; y++){
    for (size_t x = 0; x < size- 1; x++) {
      auto pINq = getPointsInQuad(Points, x1, y1, x2, y2);

      if (pINq.size() == 2) {
        auto idxMovingPoint = pINq[0];
        auto idxStayingPoint = pINq[1];
        auto newPos = checkNeighbours(Points, x1, y1, x2, y2, offset, x, y, idxMovingPoint, idxStayingPoint);

        if (newPos.first == x && newPos.second == y) {
          std::cout << "FOUL " << pINq.size() << std::endl;
        }
        else {
          image[x + y * size] = get<2>(Points[idxStayingPoint]);
          image[newPos.first + newPos.second * size] = get<2>(Points[idxMovingPoint]);
          if (idxMovingPoint > idxStayingPoint) {
            Points.erase(Points.begin() + idxMovingPoint);
            Points.erase(Points.begin() + idxStayingPoint);
          }
          else {
            Points.erase(Points.begin() + idxStayingPoint);
            Points.erase(Points.begin() + idxMovingPoint);
          }
        }

      } else if(pINq.size() == 1) {
        image[x + y * size] = get<2>(Points[pINq[0]]); 
        Points.erase(Points.begin() + pINq[0]);
      }
      else if (pINq.size() > 2) {
        std::cout << "More than two Points in one Cell! " << pINq.size() << std::endl;
      }
      else {
        image[x + y * size] = -1;
      }

      x1 += offset;
      x2 += offset;
    }

    y1 += offset;
    y2 += offset;
    x1 = 0;
    x2 = offset;
  }

  return image;

}

}
}


#endif // MINSG_EXT_THESISSTANISLAW
