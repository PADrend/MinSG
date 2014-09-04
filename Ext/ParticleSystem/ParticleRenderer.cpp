/*
	This file is part of the MinSG library extension ParticleSystem.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2010 Jan Krems
	Copyright (C) 2010-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PARTICLE

#include "ParticleRenderer.h"

#include "ParticleSystemNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/References.h>
#include <string.h>

namespace MinSG {

void ParticleBillboardRenderer::operator()(ParticleSystemNode * psys, FrameContext & context, const RenderParam & rp) {
	if(rp.getFlag(NO_GEOMETRY)) {
		return;
	}

	const auto & worldToCamera = context.getRenderingContext().getMatrix_worldToCamera();
	const auto cameraToWorld = worldToCamera.inverse();

	const auto halfRight = cameraToWorld.transformDirection(context.getWorldRightVector() * 0.5f);
	const auto halfUp = cameraToWorld.transformDirection(context.getWorldUpVector() * 0.5f);

	// 2. just update position for each particle and render
	// render particles
	const uint32_t count = psys->getParticleCount();

	Rendering::VertexDescription vertexDesc;
	const Rendering::VertexAttribute & posAttrib = vertexDesc.appendPosition3D();
	const Rendering::VertexAttribute & colorAttrib = vertexDesc.appendColorRGBAByte();
	const Rendering::VertexAttribute & texCoordAttrib = vertexDesc.appendTexCoord();
	// The usage of a cache for the mesh has been tested. Reusing a preallocated mesh is not faster.
	Util::Reference<Rendering::Mesh> mesh = new Rendering::Mesh(vertexDesc, 4 * count, 6 * count);
	mesh->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy());
	Rendering::MeshIndexData & indexData = mesh->openIndexData();
	Rendering::MeshVertexData & vertexData = mesh->openVertexData();
	Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor = Rendering::PositionAttributeAccessor::create(vertexData, posAttrib.getNameId());
	Util::Reference<Rendering::ColorAttributeAccessor> colorAccessor = Rendering::ColorAttributeAccessor::create(vertexData, colorAttrib.getNameId());
	Util::Reference<Rendering::TexCoordAttributeAccessor> texCoordAccessor = Rendering::TexCoordAttributeAccessor::create(vertexData, texCoordAttrib.getNameId());
	uint32_t * indices = indexData.data();

	uint_fast32_t index = 0;
	for(const auto & p : psys->getParticles()) {
		const Geometry::Vec3f upOffset = halfUp * p.size.getHeight();
		const Geometry::Vec3f rightOffset = halfRight * p.size.getWidth();

		colorAccessor->setColor(index + 0, p.color);
		texCoordAccessor->setCoordinate(index + 0, Geometry::Vec2f(0.0f, 0.0f));
		positionAccessor->setPosition(index + 0, p.position + upOffset - rightOffset);

		colorAccessor->setColor(index + 1, p.color);
		texCoordAccessor->setCoordinate(index + 1, Geometry::Vec2f(0.0f, 1.0f));
		positionAccessor->setPosition(index + 1, p.position - upOffset - rightOffset);

		colorAccessor->setColor(index + 2, p.color);
		texCoordAccessor->setCoordinate(index + 2, Geometry::Vec2f(1.0f, 1.0f));
		positionAccessor->setPosition(index + 2, p.position - upOffset + rightOffset);

		colorAccessor->setColor(index + 3, p.color);
		texCoordAccessor->setCoordinate(index + 3, Geometry::Vec2f(1.0f, 0.0f));
		positionAccessor->setPosition(index + 3, p.position + upOffset + rightOffset);

		*indices++ = index + 0;
		*indices++ = index + 1;
		*indices++ = index + 3;

		*indices++ = index + 1;
		*indices++ = index + 2;
		*indices++ = index + 3;

		index += 4;
	}

	indexData.markAsChanged();
	indexData.updateIndexRange();
	vertexData.markAsChanged();
	vertexData.updateBoundingBox();
	context.displayMesh(mesh.get());
}

void ParticlePointRenderer::operator()(ParticleSystemNode* psys, FrameContext & context, const RenderParam & rp /* = 0 */) {
	if ( (rp.getFlag(NO_GEOMETRY)) )
		return;

	// render particles
	std::vector<Particle> & particles = psys->getParticles();
	uint32_t count = psys->getParticleCount();

	Rendering::VertexDescription vertexDesc;
	const Rendering::VertexAttribute & posAttrib = vertexDesc.appendPosition3D();
	const Rendering::VertexAttribute & colorAttrib = vertexDesc.appendColorRGBAByte();
	// The usage of a cache for the mesh has been tested. Reusing a preallocated mesh is not faster.
	Util::Reference<Rendering::Mesh> mesh = new Rendering::Mesh(vertexDesc, count, count);
	mesh->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy());
	mesh->setDrawMode(Rendering::Mesh::DRAW_POINTS);
//	mesh->setUseIndexData(false);
	Rendering::MeshIndexData & indexData = mesh->openIndexData();
	Rendering::MeshVertexData & vertexData = mesh->openVertexData();
	Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor = Rendering::PositionAttributeAccessor::create(vertexData, posAttrib.getNameId());
	Util::Reference<Rendering::ColorAttributeAccessor> colorAccessor = Rendering::ColorAttributeAccessor::create(vertexData, colorAttrib.getNameId());
	uint32_t * indices = indexData.data();

	for(uint_fast32_t index = 0; index < count; ++index) {
		const Particle & p = particles[index];

		colorAccessor->setColor(index, p.color);
		positionAccessor->setPosition(index, p.position);

		*indices++ = index;
	}

	indexData.markAsChanged();
	indexData.updateIndexRange();
	vertexData.markAsChanged();
	vertexData.updateBoundingBox();
	context.displayMesh(mesh.get());
}

}

#endif
