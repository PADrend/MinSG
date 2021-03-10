/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#include "SurfacePoint.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Core/Transformations.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/TextureState.h"
#include "../../Core/States/GroupState.h"
#include "../TriangleTrees/TriangleTree.h"
#include "../TriangleTrees/TriangleAccessor.h"
#include "../TriangleTrees/ABTree.h"
#include "../TriangleTrees/ABTreeBuilder.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/TriangleAccessor.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>

#include <Geometry/Tools.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Line.h>
#include <Geometry/Triangle.h>
#include <Geometry/Convert.h>
#include <Geometry/Plane.h>

#include <Util/Graphics/PixelAccessor.h>

namespace Geometry {
typedef Triangle<_Vec3<float>> Triangle_f;
}

namespace MinSG {
namespace PathTracing {
  
using namespace Rendering;
using namespace TriangleTrees;
  
static const double PI_INV = 0.31830988618379067154;
static const float bias = 1e-4f;

static const auto idTriangleTree = NodeAttributeModifier::create("TriangleTree", NodeAttributeModifier::PRIVATE_ATTRIBUTE);
typedef Util::WrapperAttribute<std::unique_ptr<const TriangleTree>> TriangleTreeWrapper_t;

static bool hasTriangleTree(GeometryNode * node) {
  return node->isAttributeSet(idTriangleTree);
}

static const TriangleTree * getTriangleTree(Node * node) {
  auto attr = node->getAttribute<TriangleTreeWrapper_t>(idTriangleTree);	
  return attr->get().get();
}

static void storeTriangleTree(Node * node, const TriangleTree * tree) {
  node->setAttribute(idTriangleTree, new TriangleTreeWrapper_t(tree));
}

std::pair<uint32_t, Geometry::Triangle_f> getTriangleAtPoint(GeometryNode* geoNode, const Geometry::Vec3& pos) {
	uint32_t tIdx = std::numeric_limits<uint32_t>::max();
	Geometry::Triangle_f triangle({0,0,0},{0,0,0},{0,0,0});
	float dist = 1;	
	
	const TriangleTree* triangleTree;
	if(!hasTriangleTree(geoNode)) {
		TriangleTrees::ABTreeBuilder treeBuilder(32, 0.5f);
		triangleTree = treeBuilder.buildTriangleTree(geoNode->getMesh());
		storeTriangleTree(geoNode, triangleTree);
	} else {
		triangleTree = getTriangleTree(geoNode);
	}
		
	// TODO: check normals
	std::deque<const TriangleTree*> queue{triangleTree};
	while(!queue.empty()) {
		const TriangleTree* node = queue.front();		
		queue.pop_front();
		
		for(uint32_t i=0; i<node->getTriangleCount(); ++i) {
			auto ta = node->getTriangle(i);
			float d = ta.getTriangle().distanceSquared(pos);
			if(d < dist) {
				dist = d;
				tIdx = ta.getTriangleIndex();
				triangle = ta.getTriangle();
			}
		}
		if(!node->isLeaf()) {
			for(const auto* child : node->getChildren()) {
				if(child->getBound().getDistanceSquared(pos) < bias) {
					queue.push_back(child);
				}
			}
		}
	}
	return std::make_pair(tIdx, triangle);
}

Geometry::Vec3 sampleHemisphere(const Geometry::Vec2& sample) {
	// concentric sample disk
	Geometry::Vec2 u(0,0);
	{
		Geometry::Vec2 off = sample * 2.0f - Geometry::Vec2(1,1);
		if(!off.isZero()) {
			float theta, r;
			if(std::abs(off.x()) > std::abs(off.y())) {
				r = off.x();
				theta = static_cast<float>(M_PI/4 * (off.y() / off.x()));
			} else {
				r = off.y();
				theta = static_cast<float>(M_PI/2 - M_PI/4 * (off.x() / off.y()));
			}
			u.setValue(r * std::cos(theta), r * std::sin(theta));
		} 
	}
	float z = std::sqrt(std::max(0.0f, 1.0f - u.x() * u.x() - u.y() * u.y()));
	return {u.x(), u.y(), z};
}

BSDFSample SurfacePoint::getBSDF(const Geometry::Vec3& woWorld, const Geometry::Vec3& wiWorld) const {
	// Currently only diffuse lambertian reflection
	BSDFSample refl;
	refl.wi = wiWorld;	
	refl.f = albedo * static_cast<float>(PI_INV);
	Geometry::Vec3 wo = worldToLocal(woWorld);	
	Geometry::Vec3 wi = worldToLocal(wiWorld);	
	refl.pdf = (wo.z()*wi.z()) > 0 ? static_cast<float>(std::abs(wi.z()) * PI_INV) : 0.0f;
	return refl;
}

BSDFSample SurfacePoint::sampleBSDF(const Geometry::Vec3 &woWorld, const Geometry::Vec2& sample) const {	
	Geometry::Vec3 wo = worldToLocal(woWorld);	
	Geometry::Vec3 wi = sampleHemisphere(sample);
	if(wo.z() < 0) wi.z(-wi.z()); 
	return getBSDF(woWorld, localToWorld(wi));
}

SurfacePoint getSurfaceAt(GeometryNode* node, const Geometry::Vec3& pos) {
	SurfacePoint surface;
	
	auto mesh = node->getMesh();
	
	surface.pos = pos;
	Geometry::Vec3 localPos = Transformations::worldPosToLocalPos(*node, surface.pos);
	uint32_t tIndex;
	Geometry::Triangle_f triangle({0,0,0},{0,0,0},{0,0,0});
	
	std::tie(tIndex, triangle) = getTriangleAtPoint(node, localPos);
	
	if(tIndex > mesh->getPrimitiveCount()) {
		WARN("PathTracer: Could not find intersecting triangle.");
		return surface;
	}
	auto bc = triangle.calcBarycentricCoordinates(localPos);
	surface.normal = Transformations::localDirToWorldDir(*node, triangle.calcNormal());
	surface.tangent = triangle.getEdgeAB().getNormalized();
	surface.bitangent = surface.normal.cross(surface.tangent);
	auto& vertexData = mesh->openVertexData();
	
	auto tAcc = MeshUtils::TriangleAccessor::create(mesh);
	uint32_t t0,t1,t2;
	std::tie(t0,t1,t2) = tAcc->getIndices(tIndex);	
	
	auto states = node->getStates();
	MaterialState* mat = nullptr;
	TextureState* tex = nullptr;
	// TODO: normal texture
	for(uint32_t i=0; i<states.size(); ++i) {
		auto gs = dynamic_cast<GroupState*>(states[i]);
		auto ms = dynamic_cast<MaterialState*>(states[i]);
		auto ts = dynamic_cast<TextureState*>(states[i]);
		if(gs && gs->isActive()) {
			for(auto s : gs->getStates())
				states.push_back(s.get());
		} else if(ms && ms->isActive()) {
			mat = ms;
		} else if(ts && ts->isActive() && ts->getTextureUnit() == 0) {
			tex = ts;
		}
	}
	
	// sample texture
	if(mesh->getVertexDescription().hasAttribute(VertexAttributeIds::TEXCOORD0)) {
		auto tAcc = TexCoordAttributeAccessor::create(vertexData, VertexAttributeIds::TEXCOORD0);
		auto tc0 = tAcc->getCoordinate(t0);
		auto tc1 = tAcc->getCoordinate(t1);
		auto tc2 = tAcc->getCoordinate(t2);
		surface.texcoord = tc0 * bc.x() + tc1 * bc.y()  + tc2 * bc.z();
	}
	
	if(mat) {
		surface.albedo = mat->getParameters().getDiffuse();
		surface.emission = mat->getParameters().getEmission();
	} else if(mesh->getVertexDescription().hasAttribute(VertexAttributeIds::COLOR)) {
		auto cAcc = ColorAttributeAccessor::create(vertexData, VertexAttributeIds::COLOR);
		Util::Color4f col0 = cAcc->getColor4f(t0);
		Util::Color4f col1 = cAcc->getColor4f(t1);
		Util::Color4f col2 = cAcc->getColor4f(t2);		
		surface.albedo = col0 * bc.x() + col1 * bc.y()  + col2 * bc.z();
	} else {
		surface.albedo = Util::Color4f(1,1,1,1);
	}
	
	if(tex && tex->getTexture()->getLocalBitmap()) {
		auto pAcc = Util::PixelAccessor::create(tex->getTexture()->getLocalBitmap());
		// TODO: filtering
		surface.albedo *= pAcc->readColor4f(
			static_cast<int32_t>(surface.texcoord.x() * pAcc->getWidth()) % pAcc->getWidth(), 
			static_cast<int32_t>(surface.texcoord.y() * pAcc->getHeight()) % pAcc->getHeight());
	}	
	
	return surface;
}

}
}

#endif /* MINSG_EXT_PATHTRACING */