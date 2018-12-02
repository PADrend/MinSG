#
# This file is part of the MinSG library extension SVS.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	BudgetRenderer.cpp
	GeometryNodeCollector.cpp
	Helper.cpp
	PreprocessingContext.cpp
	Renderer.cpp
	SamplePoint.cpp
	SphereVisualizationRenderer.cpp
	Statistics.cpp
	VisibilitySphere.cpp
)

minsg_add_extension(MINSG_EXT_SVS "Defines if the MinSG extension Spherical Visibility Sampling (SVS) is built." ${MINSG_RECOMMENDED_EXT})
minsg_add_extension(MINSG_EXT_SVS_PROFILING "Defines if profiling information is generated for the MinSG extension Spherical Visibility Sampling (SVS).")
minsg_add_dependencies(MINSG_EXT_SVS MINSG_EXT_EVALUATORS MINSG_EXT_TRIANGULATION MINSG_EXT_VISIBILITY_SUBDIVISION)
minsg_add_dependencies(MINSG_EXT_SVS_PROFILING MINSG_EXT_SVS)
