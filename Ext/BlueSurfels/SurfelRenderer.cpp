/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelRenderer.h"
#include "SurfelAnalysis.h"
#include "Strategies/AbstractSurfelStrategy.h"

#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"

#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>

#include <Util/Macros.h>
#include <Util/IO/FileName.h>

#include <functional>
#include <typeinfo>
#include <iostream>

namespace MinSG {
namespace BlueSurfels {
using namespace Rendering;
using namespace Geometry;

static const Uniform::UniformName UNIFORM_SURFEL_RADIUS("sg_surfelRadius");
static const Uniform::UniformName UNIFORM_SURFEL_PACKING("sg_surfelPacking");

struct SurfelDrawCommand {
  SurfelDrawCommand(const Matrix4x4& t, const Mesh* m, uint32_t c, float s, float r, float a) : transform(t), mesh(m), count(c), size(s), radius(r), packing(a) {}
  const Matrix4x4 transform;
  const Mesh* mesh;
  const uint32_t count;
  const float size;
  const float radius;
  const float packing;
};

struct SurfelRenderer::Data {
  std::vector<Util::Reference<AbstractSurfelStrategy>> strategies;
  std::vector<SurfelDrawCommand> commands;
  bool skipRendering = false;
};

static bool compareStrategies(const Util::Reference<AbstractSurfelStrategy>& s1, const Util::Reference<AbstractSurfelStrategy>& s2) {
  return s1->getPriority() > s2->getPriority();
}

SurfelRenderer::SurfelRenderer() : NodeRendererState(FrameContext::DEFAULT_CHANNEL), data(new Data) { }

SurfelRenderer::~SurfelRenderer() = default;

/// ---|> [State]
SurfelRenderer * SurfelRenderer::clone() const { throw std::runtime_error("SurfelRenderer: cloning is not allowed!"); };

SurfelRenderer::stateResult_t SurfelRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
  data->commands.clear();
  data->skipRendering = false;
  for(auto& strategy : data->strategies) {
    if(strategy->isEnabled())
      data->skipRendering |= strategy->prepare(context, node);
  }
  
  if(data->skipRendering) {
    drawSurfels(context);
    return State::STATE_SKIP_RENDERING;
  }
  
	return NodeRendererState::doEnableState(context, node, rp);
}

void SurfelRenderer::doDisableState(FrameContext& context, Node* node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);
  drawSurfels(context);
}

NodeRendererResult SurfelRenderer::displayNode(FrameContext& context, Node* node, const RenderParam & /*rp*/) {
	if(!node->isActive())
		return NodeRendererResult::NODE_HANDLED;
	
	Rendering::Mesh* mesh = getSurfels(node);
	if(!mesh)
		return NodeRendererResult::PASS_ON;
		
	uint32_t maxPrefix = mesh->isUsingIndexData() ? mesh->getIndexCount() : mesh->getVertexCount();
	float packing = getSurfelPacking(node, mesh);
	float relPixelSize = computeRelPixelSize(context.getCamera(), node);
  auto surfelToCamera = context.getRenderingContext().getMatrix_worldToCamera() * node->getWorldTransformationMatrix();

  SurfelObject surfel{mesh, maxPrefix, packing, surfelToCamera, relPixelSize, maxPrefix, 1};

  bool breakTraversal = false;
  for(auto& strategy : data->strategies) {
    if(strategy->isEnabled())
      breakTraversal |= strategy->update(context, node, surfel);
  }
  surfel.prefix = std::min(surfel.prefix, surfel.maxPrefix);
  surfel.pointSize = std::max(surfel.pointSize, 1.0f);
	float radius = sizeToRadius(surfel.pointSize, surfel.relPixelSize);
	//float radius = std::sqrt(surfel.packing/static_cast<float>(surfel.prefix));
  
  if(surfel.prefix > 0) {
    // TODO: put matrices directly into matrix buffer
    data->commands.emplace_back(surfel.surfelToCamera, surfel.mesh, surfel.prefix, surfel.pointSize, radius, surfel.packing);
  }

  return breakTraversal ? NodeRendererResult::NODE_HANDLED : NodeRendererResult::PASS_ON;
}

void SurfelRenderer::addSurfelStrategy(AbstractSurfelStrategy* strategy) {
  data->strategies.emplace_back(strategy);
  std::sort(data->strategies.begin(), data->strategies.end(), &compareStrategies);
}

void SurfelRenderer::removeSurfelStrategy(AbstractSurfelStrategy* strategy) {
  data->strategies.erase(std::remove(data->strategies.begin(), data->strategies.end(), strategy));
  std::sort(data->strategies.begin(), data->strategies.end(), &compareStrategies);
}

void SurfelRenderer::clearSurfelStrategies() {
  data->strategies.clear();
}

std::vector<AbstractSurfelStrategy*> SurfelRenderer::getSurfelStrategies() const {
  std::vector<AbstractSurfelStrategy*> out;
  for(auto s : data->strategies)
    out.emplace_back(s.get());
  return out;
}


void SurfelRenderer::drawSurfels(FrameContext & context) {
  bool skipRendering = false;  
  for(auto& strategy : data->strategies) {
    if(strategy->isEnabled())
      skipRendering |= strategy->beforeRendering(context);
  }
  
  if(!skipRendering) {
    auto& rc = context.getRenderingContext();
    rc.pushPointParameters();
    rc.pushMatrix_modelToCamera();
    // TODO: use mutli-draw-indirect
    for(auto& cmd : data->commands) {
      rc.setGlobalUniform({UNIFORM_SURFEL_RADIUS, cmd.radius});
      rc.setGlobalUniform({UNIFORM_SURFEL_PACKING, cmd.packing});
  		rc.setPointParameters(Rendering::PointParameters(cmd.size));
      rc.setMatrix_modelToCamera(cmd.transform);
  		context.displayMesh(const_cast<Mesh*>(cmd.mesh), 0, cmd.count);
    }
    rc.popMatrix_modelToCamera();
    rc.popPointParameters();
  }
  
  for(auto it = data->strategies.rbegin(); it != data->strategies.rend(); ++it) {
    if((*it)->isEnabled())
      (*it)->afterRendering(context);
  }
}

}
}

#endif // MINSG_EXT_BLUE_SURFELS