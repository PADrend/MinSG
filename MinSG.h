/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_H_INCLUDED
#define MINSG_H_INCLUDED

const char * const MINSG_VERSION = "MinSG 0.2.0";

/**
 * @mainpage MinSG: Minimalist Scene Graph
 * @section internal_deps Internal dependencies
 * - Package: Model
 * - Package: Util
 * - Package: Geometry
 *
 * @section external_deps External dependencies
 * - OpenGL
 * - OpenAL for sound support (optional)
 *
 * @section directories Directory structure
 * @subsection directory_core Core
 * This directory contains the main classes for @link MinSG::Node nodes@endlink and @link MinSG::State states@endlink of the scene graph.
 * Furthermore there are classes for @link MinSG::AbstractCameraNode cameras@endlink and @link MinSG::LightNode lights@endlink and a @link MinSG::FrameContext rendering context@endlink.
 * This directory should not be polluted with poppycock or experimental stuff.
 * Furthermore there should not be any dependencies to other directories (especially not to @ref directory_ext).
 * @subsection directory_ext Ext
 * Poppycock and experimental stuff should go here.
 * Furthermore classes that are used seldom belong here.
 * This is a good place to try out new things and let them mature.
 * For example code that is developed during theses or code that needs external libraries should be put here.
 * If the code is tried and tested it may be moved to @ref directory_core.
 * There are subdirectories that contain related classes (for example classes from a thesis or for one special purpose).
 * All experimental code should be surrounded by a guard (see the following code example; do this in header and source files).
 * @code
 *
 * #ifdef MINSG_EXT_MYEXTENSIONNAME
 * ... your code ...
 * #endif // MINSG_EXT_MYEXTENSIONNAME
 * @endcode
 * @subsection directory_scenemanagement SceneManagement
 * Classes for the management of scenes, for example loading and saving of scenes from and to the file system.
 * Global registration for @link MinSG::SceneManagement::SceneManager::registerNode Nodes@endlink and @link MinSG::SceneManagement::SceneManager::registerState States@endlink.
 * The only dependency should be to @ref directory_core as possible.
 * @subsection directory_helper Helper
 * Things that ease your work with MinSG.
 * Most important are the @link MinSG::NodeVisitor standard visitors@endlink to traverse the scene graph.
 */

#endif // MINSG_H_INCLUDED
