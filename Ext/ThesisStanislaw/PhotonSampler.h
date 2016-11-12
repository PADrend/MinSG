/*
	This file is part of the MinSG library.
	Copyright (C) 20016 Stanislaw Eppinger

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_THESISSTANISLAW

#ifndef MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H
#define MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H

#include <tuple>

#include <Util/ReferenceCounter.h>
#include <Util/Timer.h>

#include "Statistics.h"

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/States/State.h"

#include "../../../Rendering/RenderingContext/RenderingContext.h"
#include "../../../Rendering/Shader/Shader.h"
#include "../../../Rendering/Texture/Texture.h"
#include "../../../Rendering/Texture/TextureUtils.h"
#include "../../../Rendering/Texture/TextureType.h"
#include "../../../Rendering/FBO.h"

#include "../../../Rendering/Mesh/Mesh.h"

namespace MinSG{
namespace ThesisStanislaw{
  
class PhotonSampler : public State {
  PROVIDES_TYPE_NAME(PhotonSampler)
public:
  enum class Sampling : uint8_t {
    POISSON,
    UNIFORM
  };
  
private:
  static const std::string             _shaderPath;
  
  Util::Reference<Rendering::FBO>      _fbo, _photonMatrixFBO;
  Util::Reference<Rendering::Texture>  _depthTexturePhotonMatrix, _photonMatrixTexture;
  Util::Reference<Rendering::Texture>  _depthTexture, _posTexture, _normalTexture;
  bool                                 _fboChanged, _needResampling;
  
  Util::Reference<Rendering::Texture>  _samplingTexture;
  uint32_t                             _samplingTextureSize;
  
  CameraNode*                          _camera;
  
  Util::Reference<Rendering::Shader>   _mrtShader;
  Util::Reference<Rendering::Shader>   _photonMatrixShader;
  
  Node*                                _approxScene;
  
  std::vector<Geometry::Vec2f>         _samplePoints;
  uint32_t                             _photonNumber;
  Sampling                             _samplingStrategy;
  Util::Reference<Rendering::Mesh>     _samplingMesh;
//  unsigned int                         _photonBufferGLId;

#ifdef MINSG_THESISSTANISLAW_GATHER_STATISTICS
  //Framestatistics
  Util::Timer _timer;
#endif // MINSG_THESISSTANISLAW_GATHER_STATISTICS

  void allocateSamplingTexture(std::vector<int>& samplingImage);
  bool initializeFBO(Rendering::RenderingContext& rc);
  void initializeSamplePointMesh();
  void computePhotonMatrices(Rendering::RenderingContext& rc, FrameContext & context);
  
  float distance(std::pair<float, float> p1, std::pair<float, float> p2);
  std::vector<size_t> getPointsInQuad(const std::vector<std::tuple<float, float, size_t>>& Points, float x1, float y1, float x2, float y2);
  std::pair<size_t, size_t> checkNeighbours(const std::vector<std::tuple<float, float, size_t>>& Points, const std::vector<std::vector<bool>>& occupiedCells, float x1, float y1, float x2, float y2, float offset, size_t x, size_t y, size_t& idxMovingPoint, size_t& idxStayingPoint);
  std::vector<int> computeSamplingImage(std::vector<std::tuple<float, float, size_t>> Points, size_t size);
  
  
public:
  State::stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

  PhotonSampler();
  unsigned int                         _photonBufferGLId;
  ~PhotonSampler();

  PhotonSampler * clone() const override;
  
  void setApproximatedScene(Node* root);
  void setPhotonNumber(uint32_t number);
  void setSamplingStrategy(uint8_t type);
  void setCamera(CameraNode* camera);
  void resample(Rendering::RenderingContext& rc);
  
  uint32_t getPhotonNumber();
  
  Util::Reference<Rendering::Texture> getPosTexture();
  Util::Reference<Rendering::Texture> getNormalTexture();
  uint32_t getTextureWidth();
  uint32_t getTextureHeight();
  const std::vector<Geometry::Vec2f>& getSamplePoints();
  
  void bindSamplingTexture(Rendering::RenderingContext& rc, unsigned int location);
  void unbindSamplingTexture(Rendering::RenderingContext& rc, unsigned int location);
  int getSamplingTextureSize();
  
  void bindPhotonBuffer(unsigned int location);
  void unbindPhotonBuffer(unsigned int location);
  void clearPhotonBuffer();

  Util::Reference<Rendering::Texture> getSamplingTexture() const { return _samplingTexture; }
  Util::Reference<Rendering::Texture> getMatrixTexture() const { return _photonMatrixTexture; }
  
  void outputPhotonBuffer(std::string location);
};

}
}

#endif // MINSG_EXT_THESISSTANISLAW_PHOTONSAMPLER_H
#endif // MINSG_EXT_THESISSTANISLAW
