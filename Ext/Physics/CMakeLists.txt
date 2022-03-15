#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
# Copyright (C) 2019 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	Bullet/BtPhysicObject.cpp
	Bullet/BtPhysicWorld.cpp
	Bullet/Helper.cpp
	Bullet/MotionState.cpp
	Bullet/BtConstraintObject.cpp
	PhysicWorld.cpp
)

minsg_add_extension(MINSG_EXT_PHYSICS "physic experiments" ${MINSG_RECOMMENDED_EXT})
#minsg_add_dependencies(MINSG_EXT_PHYSICS MINSG_HAVE_LIB_BULLET)

if(MINSG_EXT_PHYSICS)	
	# Dependency to Bullet
	FetchContent_Declare(
		bullet
		GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
		GIT_TAG 3.09
		GIT_SHALLOW ON
	)	
	FetchContent_GetProperties(bullet)
	if(NOT bullet_POPULATED)
		message(STATUS "Fetching bullet...")
		FetchContent_Populate(bullet)
		# hide options in cmake-gui
		set(CATCH_USE_VALGRIND OFF CACHE INTERNAL "" FORCE)
		set(USE_DOUBLE_PRECISION OFF CACHE INTERNAL "" FORCE)
		set(USE_GRAPHICAL_BENCHMARK OFF CACHE INTERNAL "" FORCE)
		set(USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD ON CACHE INTERNAL "" FORCE)
		set(BULLET2_MULTITHREADING ON CACHE INTERNAL "" FORCE)
		set(USE_MSVC_INCREMENTAL_LINKING OFF CACHE INTERNAL "" FORCE)
		set(USE_CUSTOM_VECTOR_MATH OFF CACHE INTERNAL "" FORCE)
		set(INTERNAL_CREATE_DISTRIBUTABLE_MSVC_PROJECTFILES OFF CACHE INTERNAL "" FORCE)
		set(INTERNAL_CREATE_MSVC_RELATIVE_PATH_PROJECTFILES OFF CACHE INTERNAL "" FORCE)
		set(INTERNAL_ADD_POSTFIX_EXECUTABLE_NAMES OFF CACHE INTERNAL "" FORCE)
		set(BUILD_CPU_DEMOS OFF CACHE INTERNAL "" FORCE)
		set(INTERNAL_UPDATE_SERIALIZATION_STRUCTURES OFF CACHE INTERNAL "" FORCE)
		set(USE_GLUT OFF CACHE INTERNAL "" FORCE)
		set(BUILD_BULLET3 ON CACHE INTERNAL "" FORCE)
		set(BUILD_PYBULLET OFF CACHE INTERNAL "" FORCE)
		set(BUILD_ENET OFF CACHE INTERNAL "" FORCE)
		set(BUILD_CLSOCKET OFF CACHE INTERNAL "" FORCE)
		set(BUILD_EGL OFF CACHE INTERNAL "" FORCE)
		set(BUILD_OPENGL3_DEMOS OFF CACHE INTERNAL "" FORCE)
		set(BUILD_BULLET2_DEMOS OFF CACHE INTERNAL "" FORCE)
		set(BUILD_EXTRAS OFF CACHE INTERNAL "" FORCE)
		set(INSTALL_LIBS OFF CACHE INTERNAL "" FORCE)
		set(BUILD_UNIT_TESTS OFF CACHE INTERNAL "" FORCE)
		set(INSTALL_CMAKE_FILES OFF CACHE INTERNAL "" FORCE)
		
		if(MSVC)
			set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE INTERNAL "" FORCE)
		else()
			set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE INTERNAL "" FORCE)
		endif()

		set(BUILD_SHARED_LIBS OFF)
		add_subdirectory(${bullet_SOURCE_DIR} ${bullet_BINARY_DIR} EXCLUDE_FROM_ALL)
	endif()
	minsg_link_libraries(
		Bullet2FileLoader Bullet3Dynamics Bullet3Collision Bullet3Geometry
		BulletInverseDynamics BulletSoftBody BulletDynamics BulletCollision LinearMath Bullet3Common
	)
	minsg_include_directories(${bullet_SOURCE_DIR}/src)
	append_property(MINSG_COMPILE_DEFINITIONS MINSG_HAVE_LIB_BULLET)
  set_target_properties(
		Bullet2FileLoader Bullet3Dynamics Bullet3Collision Bullet3Geometry
		BulletInverseDynamics BulletSoftBody BulletDynamics BulletCollision LinearMath Bullet3Common
		PROPERTIES POSITION_INDEPENDENT_CODE ON
	)
endif()