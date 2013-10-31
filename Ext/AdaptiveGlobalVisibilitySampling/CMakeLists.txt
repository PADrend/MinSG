#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	AdaptiveGlobalVisibilitySampling.cpp
	MutationCandidates.cpp
	SampleDistributions.cpp
	ViewCells.cpp
)

minsg_add_extension(MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING "Defines if the MinSG extension AdaptiveGlobalVisibilitySampling is built.")
minsg_add_extension(MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING "Defines if profiling information is generated for the MinSG extension AdaptiveGlobalVisibilitySampling.")
minsg_add_dependencies(MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING MINSG_EXT_RAYCASTING MINSG_EXT_TRIANGLETREES MINSG_EXT_VISIBILITY_SUBDIVISION)
minsg_add_dependencies(MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING_PROFILING MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING)
