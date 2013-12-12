#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	AlgoSelector.cpp
	LP.cpp
	MultiAlgoGroupNode.cpp
	Region.cpp
	RegionEvaluator.cpp
	SampleContext.cpp
	SampleRegion.cpp
	SampleStorage.cpp
	SurfelRenderer.cpp
)

minsg_add_extension(MINSG_EXT_MULTIALGORENDERING "Defines if the MinSG extension Multiple Algorithm Rendering is built.")
minsg_add_dependencies(MINSG_EXT_MULTIALGORENDERING MINSG_HAVE_LIB_LPSOLVE MINSG_EXT_BLUE_SURFELS MINSG_EXT_COLORCUBES MINSG_EXT_SVS)
