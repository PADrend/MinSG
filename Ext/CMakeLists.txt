#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

#set(MINSG_BUILD_EXTENSIONS ON)

set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

add_subdirectory(AdaptiveGlobalVisibilitySampling)
add_subdirectory(Behaviours)
add_subdirectory(BlueSurfels)
add_subdirectory(ColorCubes)
add_subdirectory(D3Fact)
add_subdirectory(Evaluator)
add_subdirectory(ImageCompare)
add_subdirectory(ImpostorFactory)
add_subdirectory(KeyFrameAnimation)
add_subdirectory(LoaderCOLLADA)
add_subdirectory(MixedExtVisibility)
add_subdirectory(MultiAlgoRendering)
add_subdirectory(Nodes)
add_subdirectory(OcclusionCulling)
add_subdirectory(OutOfCore)
add_subdirectory(ParticleSystem)
add_subdirectory(PipelineStatistics)
add_subdirectory(PathTracing)
add_subdirectory(Physics)
add_subdirectory(Profiling)
add_subdirectory(RayCasting)
add_subdirectory(RTree)
add_subdirectory(SamplingAnalysis)
add_subdirectory(SceneManagement)
add_subdirectory(SkeletalAnimation)
add_subdirectory(States)
add_subdirectory(SVS)
add_subdirectory(ThesisJonas)
add_subdirectory(ThesisPeter)
add_subdirectory(ThesisStanislaw)
add_subdirectory(Tools)
add_subdirectory(TreeBuilder)
add_subdirectory(TreeSync)
add_subdirectory(TriangleTrees)
add_subdirectory(Triangulation)
add_subdirectory(TwinPartitions)
add_subdirectory(ValuatedRegion)
add_subdirectory(VisibilityMerge)
add_subdirectory(VisibilitySubdivision)
add_subdirectory(VoxelWorld)
add_subdirectory(Waypoints)
