/*
	This file is part of the MinSG library.
	Copyright (C) 2011-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Ext/VisibilitySubdivision/VisibilityVector.h>
#include <MinSG/SceneManagement/SceneManager.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/Timer.h>
#include <bitset>
#include <cstdlib>
#include <iostream>
#include <sstream>

#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
int testDifferentBenefits() {
	using MinSG::VisibilitySubdivision::VisibilityVector;

	Util::Reference<MinSG::GeometryNode> nodes[6];
	for(auto & nodeRef : nodes) {
		nodeRef = new MinSG::GeometryNode;
	}

	// (1, 1, 0, 2, 0, 0)
	VisibilityVector vvA;
	// Insert in "random" order
	vvA.setNode(nodes[3].get(), 2);
	vvA.setNode(nodes[1].get(), 1);
	vvA.setNode(nodes[0].get(), 1);

	// (0, 2, 2, 1, 2, 0)
	VisibilityVector vvB;
	// Insert in "random" order
	vvB.setNode(nodes[3].get(), 1);
	vvB.setNode(nodes[1].get(), 2);
	vvB.setNode(nodes[4].get(), 2);
	vvB.setNode(nodes[2].get(), 2);

	{
		// Make sure that the minimum is the intersection of the two sets.
		const VisibilityVector vvC = VisibilityVector::makeMin(vvA, vvB);
		// (0, 1, 0, 1, 0, 0)
		bool success = true;
		success &= vvC.getBenefits(nodes[0].get()) == 0;
		success &= vvC.getBenefits(nodes[1].get()) == 1;
		success &= vvC.getBenefits(nodes[2].get()) == 0;
		success &= vvC.getBenefits(nodes[3].get()) == 1;
		success &= vvC.getBenefits(nodes[4].get()) == 0;
		success &= vvC.getBenefits(nodes[5].get()) == 0;
		if(!success) {
			std::cout << "makeMin failed: " << vvC.toString() << std::endl;
			return EXIT_FAILURE;
		}
	}
	{
		// Make sure that the maximum is the union of the two sets.
		const VisibilityVector vvC = VisibilityVector::makeMax(vvA, vvB);
		// (1, 2, 2, 2, 2, 0)
		bool success = true;
		success &= vvC.getBenefits(nodes[0].get()) == 1;
		success &= vvC.getBenefits(nodes[1].get()) == 2;
		success &= vvC.getBenefits(nodes[2].get()) == 2;
		success &= vvC.getBenefits(nodes[3].get()) == 2;
		success &= vvC.getBenefits(nodes[4].get()) == 2;
		success &= vvC.getBenefits(nodes[5].get()) == 0;
		if(!success) {
			std::cout << "makeMax failed: " << vvC.toString() << std::endl;
			return EXIT_FAILURE;
		}
	}
	{
		// Make sure that the difference is the set difference of the two sets.
		const VisibilityVector vvC = VisibilityVector::makeDifference(vvA, vvB);
		bool success = true;
		success &= vvC.getBenefits(nodes[0].get()) == 1;
		success &= vvC.getBenefits(nodes[1].get()) == 0;
		success &= vvC.getBenefits(nodes[2].get()) == 0;
		success &= vvC.getBenefits(nodes[3].get()) == 0;
		success &= vvC.getBenefits(nodes[4].get()) == 0;
		success &= vvC.getBenefits(nodes[5].get()) == 0;
		if(!success) {
			std::cout << "makeDifference failed: " << vvC.toString() << std::endl;
			return EXIT_FAILURE;
		}
	}
	{
		// Make sure that the difference is the set difference of the two sets.
		const VisibilityVector vvC = VisibilityVector::makeDifference(vvB, vvA);
		bool success = true;
		success &= vvC.getBenefits(nodes[0].get()) == 0;
		success &= vvC.getBenefits(nodes[1].get()) == 0;
		success &= vvC.getBenefits(nodes[2].get()) == 2;
		success &= vvC.getBenefits(nodes[3].get()) == 0;
		success &= vvC.getBenefits(nodes[4].get()) == 2;
		success &= vvC.getBenefits(nodes[5].get()) == 0;
		if(!success) {
			std::cout << "makeDifference failed: " << vvC.toString() << std::endl;
			return EXIT_FAILURE;
		}
	}
	{
		// Make sure that the symmetric difference is the exclusive or of the two sets.
		const VisibilityVector vvC = VisibilityVector::makeSymmetricDifference(vvA, vvB);
		bool success = true;
		success &= vvC.getBenefits(nodes[0].get()) == 1;
		success &= vvC.getBenefits(nodes[1].get()) == 0;
		success &= vvC.getBenefits(nodes[2].get()) == 2;
		success &= vvC.getBenefits(nodes[3].get()) == 0;
		success &= vvC.getBenefits(nodes[4].get()) == 2;
		success &= vvC.getBenefits(nodes[5].get()) == 0;
		if(!success) {
			std::cout << "makeSymmetricDifference failed: " << vvC.toString() << std::endl;
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
#endif /* MINSG_EXT_VISIBILITY_SUBDIVISION */

int test_visibility_vector() {
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
	std::cout << "Test VisibilityVector ... ";
	Util::Timer timer;
	timer.reset();

	if(testDifferentBenefits() != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	using MinSG::VisibilitySubdivision::VisibilityVector;
	MinSG::SceneManagement::SceneManager sceneManager;
	
	const uint32_t count = 10;
	
	Util::Reference<MinSG::GeometryNode> nodes[count];
	for(uint_fast32_t i = 0; i < count; ++i) {
		nodes[i] = new MinSG::GeometryNode;
		sceneManager.registerNode(std::string("Node") + Util::StringUtils::toString(i), nodes[i].get());
	}
	for(uint_fast32_t i = 0; i < (1 << count); ++i) {
		const std::bitset<count> setA(i);
		VisibilityVector vvA;
		VisibilityVector::benefits_t sumBenefitsA = 0;
		for(uint_fast32_t n = 0; n < count; ++n) {
			if(setA[n]) {
				vvA.setNode(nodes[n].get(), n + 1);
				sumBenefitsA += (n + 1);
			}
		}
		// Test if everything was saved correctly.
		for(uint_fast32_t n = 0; n < count; ++n) {
			const bool contained = (vvA.getBenefits(nodes[n].get()) > 0);
			if(contained != setA[n]) {
				std::cout << "Creation of first VisibilityVector failed: " << vvA.toString() << " != " << setA << std::endl;
				return EXIT_FAILURE;
			}
		}
		if(vvA.getTotalBenefits() != sumBenefitsA) {
			std::cout << "Wrong benefits." << std::endl;
			return EXIT_FAILURE;
		}
		{
			// Test serialization and unserialization;
			std::stringstream stream;
			vvA.serialize(stream, sceneManager);
			VisibilityVector vvC = VisibilityVector::unserialize(stream, sceneManager);
			for(uint_fast32_t n = 0; n < count; ++n) {
				if(vvA.getBenefits(nodes[n].get()) != vvC.getBenefits(nodes[n].get())) {
					std::cout << "Serialization/unserialization failed." << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
		for(uint_fast32_t j = 0; j < (1 << count); ++j) {
			const std::bitset<count> setB(j);
			VisibilityVector vvB;
			VisibilityVector::benefits_t sumBenefitsB = 0;
			for(uint_fast32_t n = 0; n < count; ++n) {
				if(setB[n]) {
					vvB.setNode(nodes[n].get(), count - n);
					sumBenefitsB += count - n;
				}
			}
			// Test if everything was saved correctly.
			for(uint_fast32_t n = 0; n < count; ++n) {
				const bool contained = (vvB.getBenefits(nodes[n].get()) > 0);
				if(contained != setB[n]) {
					std::cout << "Creation of second VisibilityVector failed: " << vvB.toString() << " != " << setB << std::endl;
					return EXIT_FAILURE;
				}
			}
			if(vvB.getTotalBenefits() != sumBenefitsB) {
				std::cout << "Wrong benefits." << std::endl;
				return EXIT_FAILURE;
			}
			{
				// Make sure that the minimum is the intersection of the two sets.
				const VisibilityVector vvC = VisibilityVector::makeMin(vvA, vvB);
				const std::bitset<count> setC = (setA & setB);
				for(uint_fast32_t n = 0; n < count; ++n) {
					const bool contained = (vvC.getBenefits(nodes[n].get()) > 0);
					if(contained != setC[n]) {
						std::cout << "makeMin failed: " << vvC.toString() << " != " << setC << std::endl;
						return EXIT_FAILURE;
					}
				}
			}
			{
				// Make sure that the maximum is the union of the two sets.
				const VisibilityVector vvC = VisibilityVector::makeMax(vvA, vvB);
				const std::bitset<count> setC = (setA | setB);
				for(uint_fast32_t n = 0; n < count; ++n) {
					const bool contained = (vvC.getBenefits(nodes[n].get()) > 0);
					if(contained != setC[n]) {
						std::cout << "makeMax failed: " << vvC.toString() << " != " << setC << std::endl;
						return EXIT_FAILURE;
					}
				}
			}
			{
				// Make sure that the difference is the set difference of the two sets.
				const VisibilityVector vvC = VisibilityVector::makeDifference(vvA, vvB);
				const std::bitset<count> setC = (setA & ~setB);
				for(uint_fast32_t n = 0; n < count; ++n) {
					const bool contained = (vvC.getBenefits(nodes[n].get()) > 0);
					if(contained != setC[n]) {
						std::cout << "makeDifference failed: " << vvC.toString() << " != " << setC << std::endl;
						return EXIT_FAILURE;
					}
				}
			}
			{
				// Make sure that the symmetric difference is the exclusive or of the two sets.
				const VisibilityVector vvC = VisibilityVector::makeSymmetricDifference(vvA, vvB);
				const std::bitset<count> setC = (setA ^ setB);
				for(uint_fast32_t n = 0; n < count; ++n) {
					const bool contained = (vvC.getBenefits(nodes[n].get()) > 0);
					if(contained != setC[n]) {
						std::cout << "makeSymmetricDifference failed: " << vvC.toString() << " != " << setC << std::endl;
						return EXIT_FAILURE;
					}
				}
			}
			{
				// Make sure that diff works as expected.
				VisibilityVector::costs_t costsDiff;
				VisibilityVector::benefits_t benefitsDiff;
				std::size_t sameCount;
				VisibilityVector::diff(vvA, vvB, costsDiff, benefitsDiff, sameCount);
				const std::bitset<count> setIntersection = (setA & setB);
				if(sameCount != setIntersection.count()) {
					std::cout << "diff failed: sameCount is wrong." << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
	}
	
	const float oneThird = 1.0f / 3.0f;
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 1; n <= count; ++n) {
			vvA.setNode(nodes[n - 1].get(), n);
			vvB.setNode(nodes[n - 1].get(), 2 * n);
			vvC.setNode(nodes[n - 1].get(), 4 * n);
		}
		{
			// Artifical case: Weight all three vectors with zero
			const VisibilityVector vvW = VisibilityVector::makeWeightedThree(0.0f, vvA, 0.0f, vvB, 0.0f, vvC);
			if(vvW.getVisibleNodeCount() > 0) {
			std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << vvW.toString() << " is not empty." << std::endl;
				return EXIT_FAILURE;
			}
		}
		{
			// Weight the first vector with zero
			const VisibilityVector vvW = VisibilityVector::makeWeightedThree(0.0f, vvA, 0.5f, vvB, 0.5f, vvC);
			for(uint_fast32_t n = 1; n <= count; ++n) {
				const VisibilityVector::benefits_t expected = 3 * n;
				const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n - 1].get());
				if(expected != actual) {
					std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
		{
			// Weight the second vector with zero
			const VisibilityVector vvW = VisibilityVector::makeWeightedThree(0.5f, vvA, 0.0f, vvB, 0.5f, vvC);
			for(uint_fast32_t n = 1; n <= count; ++n) {
				const VisibilityVector::benefits_t expected = 2.5f * n;
				const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n - 1].get());
				if(expected != actual) {
					std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
		{
			// Weight the third vector with zero
			const VisibilityVector vvW = VisibilityVector::makeWeightedThree(0.5f, vvA, 0.5f, vvB, 0.0f, vvC);
			for(uint_fast32_t n = 1; n <= count; ++n) {
				const VisibilityVector::benefits_t expected = 1.5f * n;
				const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n - 1].get());
				if(expected != actual) {
					std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
		{
			// Weight only one vector with one
			const VisibilityVector vvWA = VisibilityVector::makeWeightedThree(1.0f, vvA, 0.0f, vvB, 0.0f, vvC);
			const VisibilityVector vvWB = VisibilityVector::makeWeightedThree(0.0f, vvA, 1.0f, vvB, 0.0f, vvC);
			const VisibilityVector vvWC = VisibilityVector::makeWeightedThree(0.0f, vvA, 0.0f, vvB, 1.0f, vvC);
			for(uint_fast32_t n = 1; n <= count; ++n) {
				const VisibilityVector::benefits_t expectedA = vvA.getBenefits(nodes[n - 1].get());
				const VisibilityVector::benefits_t actualA = vvWA.getBenefits(nodes[n - 1].get());
				if(expectedA != actualA) {
					std::cout << "makeWeightedThree failed for vvWA (line " << __LINE__ << "): " << expectedA << " != " << actualA << std::endl;
					return EXIT_FAILURE;
				}
				const VisibilityVector::benefits_t expectedB = vvB.getBenefits(nodes[n - 1].get());
				const VisibilityVector::benefits_t actualB = vvWB.getBenefits(nodes[n - 1].get());
				if(expectedB != actualB) {
					std::cout << "makeWeightedThree failed for vvWB (line " << __LINE__ << "): " << expectedB << " != " << actualB << std::endl;
					return EXIT_FAILURE;
				}
				const VisibilityVector::benefits_t expectedC = vvC.getBenefits(nodes[n - 1].get());
				const VisibilityVector::benefits_t actualC = vvWC.getBenefits(nodes[n - 1].get());
				if(expectedC != actualC) {
					std::cout << "makeWeightedThree failed for vvWC (line " << __LINE__ << "): " << expectedC << " != " << actualC << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
		{
			// Equally weight the three vectors
			const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
			for(uint_fast32_t n = 1; n <= count; ++n) {
				const VisibilityVector::benefits_t expected = (7 * n) / 3;
				const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n - 1].get());
				if(expected != actual) {
					std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
		{
			const VisibilityVector vvW = VisibilityVector::makeWeightedThree(0.25f, vvA, 0.5f, vvB, 0.25f, vvC);
			for(uint_fast32_t n = 1; n <= count; ++n) {
				const VisibilityVector::benefits_t expected = (9 * n) / 4;
				const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n - 1].get());
				if(expected != actual) {
					std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
	}
	
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 1; n <= count; ++n) {
			if(n % 2 == 0) {
				vvA.setNode(nodes[n - 1].get(), 3 * n);
			}
			if(n % 3 == 0) {
				vvB.setNode(nodes[n - 1].get(), 6 * n);
			}
			if(n % 5 == 0) {
				vvC.setNode(nodes[n - 1].get(), 9 * n);
			}
		}
		{
			const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
			for(uint_fast32_t n = 1; n <= count; ++n) {
				const VisibilityVector::benefits_t expected = (n % 2 == 0 ? n : 0) + (n % 3 == 0 ? 2 * n : 0) + (n % 5 == 0 ? 3 * n : 0);
				const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n - 1].get());
				if(expected != actual) {
					std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
	}
	
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 0; n < count; ++n) {
			vvA.setNode(nodes[n].get(), 1 + n);
			vvB.setNode(nodes[n].get(), 1 + 2 * n);
			// vvC is empty.
		}
		const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
		for(uint_fast32_t n = 0; n < count; ++n) {
			const VisibilityVector::benefits_t expected = n;
			const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n].get());
			if(expected != actual) {
				std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
				return EXIT_FAILURE;
			}
		}
	}
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 0; n < count; ++n) {
			vvA.setNode(nodes[n].get(), 2 * n);
			// vvB is empty.
			vvC.setNode(nodes[n].get(), n);
		}
		const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
		for(uint_fast32_t n = 0; n < count; ++n) {
			const VisibilityVector::benefits_t expected = n;
			const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n].get());
			if(expected != actual) {
				std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
				return EXIT_FAILURE;
			}
		}
	}
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 0; n < count; ++n) {
			// vvA is empty.
			vvB.setNode(nodes[n].get(), 4 * n);
			vvC.setNode(nodes[n].get(), 2 * n);
		}
		const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
		for(uint_fast32_t n = 0; n < count; ++n) {
			const VisibilityVector::benefits_t expected = 2 * n;
			const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n].get());
			if(expected != actual) {
				std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
				return EXIT_FAILURE;
			}
		}
	}
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 0; n < count; ++n) {
			vvA.setNode(nodes[n].get(), 3 * n);
			// vvB is empty.
			// vvC is empty.
		}
		const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
		for(uint_fast32_t n = 0; n < count; ++n) {
			const VisibilityVector::benefits_t expected = n;
			const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n].get());
			if(expected != actual) {
				std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
				return EXIT_FAILURE;
			}
		}
	}
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 0; n < count; ++n) {
			// vvA is empty.
			vvB.setNode(nodes[n].get(), 6 * n);
			// vvC is empty.
		}
		const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
		for(uint_fast32_t n = 0; n < count; ++n) {
			const VisibilityVector::benefits_t expected = 2 * n;
			const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n].get());
			if(expected != actual) {
				std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
				return EXIT_FAILURE;
			}
		}
	}
	{
		VisibilityVector vvA;
		VisibilityVector vvB;
		VisibilityVector vvC;
		for(uint_fast32_t n = 0; n < count; ++n) {
			// vvA is empty.
			// vvB is empty.
			vvC.setNode(nodes[n].get(), 9 * n);
		}
		const VisibilityVector vvW = VisibilityVector::makeWeightedThree(oneThird, vvA, oneThird, vvB, oneThird, vvC);
		for(uint_fast32_t n = 0; n < count; ++n) {
			const VisibilityVector::benefits_t expected = 3 * n;
			const VisibilityVector::benefits_t actual = vvW.getBenefits(nodes[n].get());
			if(expected != actual) {
				std::cout << "makeWeightedThree failed (line " << __LINE__ << "): " << expected << " != " << actual << std::endl;
				return EXIT_FAILURE;
			}
		}
	}
	
	timer.stop();
	std::cout << "done (duration: " << timer.getSeconds() << " s).\n";
#endif /* MINSG_EXT_VISIBILITY_SUBDIVISION */
	return EXIT_SUCCESS;
}
