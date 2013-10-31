/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2011 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2008 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <MinSG/MinSG.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Helper.h>
#include <Util/UI/EventContext.h>
#include <Util/UI/UI.h>
#include <Util/StringUtils.h>
#include <Util/Util.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

extern int test_automatic();
extern int test_cost_evaluator(Util::UI::Window *);
extern int test_large_scene(Util::UI::Window *, Util::UI::EventContext &);
extern int test_load_scene(Util::UI::Window *, Util::UI::EventContext &);
extern int test_node_memory();
extern int test_OutOfCore();
extern int test_simple1(Util::UI::Window *, Util::UI::EventContext &);
extern int test_spherical_sampling();
extern int test_spherical_sampling_serialization();
extern int test_statistics();
extern int test_valuated_region_node();
extern int test_visibility_vector();

static Util::UI::Window * init() {
	std::cout << "Init video system ... ";
	Util::UI::Window::Properties properties;
	properties.positioned = true;
	properties.posX = 100;
	properties.posY = 100;
	properties.clientAreaWidth = 1024;
	properties.clientAreaHeight = 768;
	properties.title = "MinSG Test";
	properties.compatibilityProfile = true;
	Util::UI::Window * window = Util::UI::createWindow(properties);
	Rendering::enableGLErrorChecking();
	Rendering::RenderingContext::initGLState();
	std::cout << "done.\n";
	return window;
}

int main(int argc, char * argv[]) {
	Util::init();
	
	// Check command line parameters
	uint32_t testNum = 0;
	for (int i = 0; i < argc; ++i) {
		std::string arg(argv[i]);
		if (arg.compare(0, 7, "--test=") == 0) {
			testNum = Util::StringUtils::toNumber<uint32_t>(arg.substr(7));
		}
	}

	std::cout << MINSG_VERSION << "\n";
	std::cout << "==============\n";

	if (testNum == 0) {
		std::cout << "Select test:\n";
		std::cout << " 1 ... Simple 1 (just some objects and movement handler)\n";
		std::cout << " 2 ... SceneLoader (load a MinSG scene)\n";
		std::cout << " 3 ... (currently unused)\n";
		std::cout << " 4 ... Start Clustering\n";
		std::cout << " 5 ... Automatic test (command line only, no GUI)\n";
		std::cout << " 6 ... Test for MinSG::OutOfCore\n";
		std::cout << " 7 ... Test with large scene\n";
		std::cout << " 8 ... Test memory consumption\n";
		std::cout << " 9 ... Test CostEvaluator (and implicitly VisibilityTester)\n";
		std::cout << "10 ... Test SphericalSampling\n";
		std::cout << "11 ... Test serialization of SphericalSampling object\n";
		std::cout << "12 ... Test ValuatedRegionNode\n";
		std::cout << "13 ... Test VisibilityVector\n";
		std::cout << "14 ... Test Statistics\n";

		std::cout << "Select test: ";
		std::cin >> testNum;
	} else {
		std::cout << "Selected test: " << testNum << "\n";
	}

	std::unique_ptr<Util::UI::Window> window;
	Util::UI::EventContext eventContext;
	switch(testNum) {
		case 1:
			window.reset(init());
			eventContext.getEventQueue().registerEventGenerator(std::bind(&Util::UI::Window::fetchEvents, window.get()));
			return test_simple1(window.get(), eventContext);
		case 2:
			window.reset(init());
			eventContext.getEventQueue().registerEventGenerator(std::bind(&Util::UI::Window::fetchEvents, window.get()));
			return test_load_scene(window.get(), eventContext);
		case 4:
			window.reset(init());
			// startPreprocessing();
			return EXIT_SUCCESS;
		case 5:
			return test_automatic();
		case 6:
			return test_OutOfCore();
		case 7:
			window.reset(init());
			eventContext.getEventQueue().registerEventGenerator(std::bind(&Util::UI::Window::fetchEvents, window.get()));
			return test_large_scene(window.get(), eventContext);
		case 8:
			window.reset(init());
			return test_node_memory();
		case 9:
			window.reset(init());
			return test_cost_evaluator(window.get());
		case 10:
			window.reset(init());
			return test_spherical_sampling();
		case 11:
			return test_spherical_sampling_serialization();
		case 12:
			return test_valuated_region_node();
		case 13:
			return test_visibility_vector();
		case 14:
			return test_statistics();
		default:
			std::cout << "FAILURE: Invalid test selected!\n";
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
