/*
	This file is part of the MinSG library.
	Copyright (C) 2012-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <Geometry/Sphere.h>
#include <Geometry/Vec3.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Ext/SVS/SamplePoint.h>
#include <MinSG/Ext/SVS/SamplingSphere.h>
#include <MinSG/Ext/VisibilitySubdivision/VisibilityVector.h>
#include <MinSG/SceneManagement/SceneManager.h>
#include <Rendering/MeshUtils/PlatonicSolids.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Util/References.h>
#include <Util/Timer.h>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

// Prevent warning
int test_spherical_sampling_serialization();

int test_spherical_sampling_serialization() {
#ifdef MINSG_EXT_SVS
	std::cout << "Test serialization of SVS objects ... ";
	Util::Timer timer;
	timer.reset();

	using namespace MinSG::SVS;
	MinSG::SceneManagement::SceneManager sceneManager;

	const uint32_t count = 10;
	std::array<Util::Reference<MinSG::GeometryNode>, count> nodes;
	for(uint_fast32_t i = 0; i < count; ++i) {
		nodes[i] = new MinSG::GeometryNode;
		sceneManager.registerNode(std::string("Node") + Util::StringUtils::toString(i), nodes[i].get());
	}
	MinSG::VisibilitySubdivision::VisibilityVector vv;
	for(uint_fast32_t i = 0; i < count; ++i) {
		vv.setNode(nodes[i].get(), i);
	}

	std::vector<SamplePoint> samples;
	{
		using namespace Rendering::MeshUtils::PlatonicSolids;
		Util::Reference<Rendering::Mesh> mesh = createEdgeSubdivisionSphere(createIcosahedron(), 4);
		auto accessor = Rendering::PositionAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
		for(std::size_t i = 0; accessor->checkRange(i); ++i) {
			samples.emplace_back(accessor->getPosition(i));
		}
	}
	{
		std::stringstream stream;
		stream.precision(std::numeric_limits<long double>::digits10);
		// Serialize
		for(const auto & sample : samples) {
			stream << sample.getPosition() << ' ';
			sample.getValue().serialize(stream, sceneManager);
			stream << ' ';
		}
		// Unserialize
		std::vector<SamplePoint> newSamples;
		for(std::size_t s = 0; s < samples.size(); ++s) {
			Geometry::Vec3f pos;
			stream >> pos;
			SamplePoint sample(pos);
			sample.setValue(MinSG::VisibilitySubdivision::VisibilityVector::unserialize(stream, sceneManager));
			newSamples.push_back(sample);
		}
		for(std::size_t s = 0; s < samples.size(); ++s) {
			const SamplePoint & oldSample = samples[s];
			const SamplePoint & newSample = newSamples[s];
			if(oldSample.getPosition().distance(newSample.getPosition()) > std::numeric_limits<float>::epsilon()) {
				std::cout << "Serialization/unserialization failed." << std::endl;
				return EXIT_FAILURE;
			}
			const auto & oldVV = oldSample.getValue();
			const auto & newVV = newSample.getValue();
			for(uint_fast32_t n = 0; n < count; ++n) {
				if(oldVV.getBenefits(nodes[n].get()) != newVV.getBenefits(nodes[n].get())) {
					std::cout << "Serialization/unserialization failed." << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
	}
	SamplingSphere samplingSphere(Geometry::Sphere_f(Geometry::Vec3f(1.0f, 2.0f, 3.0f), 17.0f), samples);
	{
		std::stringstream stream;
		stream.precision(std::numeric_limits<long double>::digits10);
		// Serialize
		stream << samplingSphere.getSphere() << ' ' << samples.size();
		for(const auto & sample : samples) {
			stream << ' ' << sample.getPosition() << ' ';
			sample.getValue().serialize(stream, sceneManager);
			stream << ' ';
		}
		
		// Unserialize
		Geometry::Sphere_f sphere;
		stream >> sphere;
		std::size_t sampleCount;
		stream >> sampleCount;
		std::vector<SamplePoint> newSamples;
		for(std::size_t s = 0; s < sampleCount; ++s) {
			Geometry::Vec3f pos;
			stream >> pos;
			SamplePoint sample(pos);
			sample.setValue(MinSG::VisibilitySubdivision::VisibilityVector::unserialize(stream, sceneManager));
			newSamples.push_back(sample);
		}
		
		SamplingSphere newSamplingSphere(sphere, newSamples);
		if(!(samplingSphere.getSphere() == newSamplingSphere.getSphere())) {
			std::cout << "Serialization/unserialization failed." << std::endl;
			return EXIT_FAILURE;
		}
		for(std::size_t s = 0; s < samples.size(); ++s) {
			const SamplePoint & oldSample = samplingSphere.getSamples()[s];
			const SamplePoint & newSample = newSamplingSphere.getSamples()[s];
			if(oldSample.getPosition().distance(newSample.getPosition()) > std::numeric_limits<float>::epsilon()) {
				std::cout << "Serialization/unserialization failed." << std::endl;
				return EXIT_FAILURE;
			}
			const auto & oldVV = oldSample.getValue();
			const auto & newVV = newSample.getValue();
			for(uint_fast32_t n = 0; n < count; ++n) {
				if(oldVV.getBenefits(nodes[n].get()) != newVV.getBenefits(nodes[n].get())) {
					std::cout << "Serialization/unserialization failed." << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
	}

	timer.stop();
	std::cout << "done (duration: " << timer.getSeconds() << " s).\n";
#endif /* MINSG_EXT_SVS */
	return EXIT_SUCCESS;
}
