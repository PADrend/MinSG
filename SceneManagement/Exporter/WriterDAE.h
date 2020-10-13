/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef WRITERDAE_H
#define WRITERDAE_H

#include <Util/StringIdentifier.h>

#include <map>
#include <iosfwd>
#include <cstdint>
#include <string>

namespace Rendering {
	class Mesh;
}
namespace Util {
	class FileName;
}
namespace MinSG {
	class Node;

	namespace SceneManagement {
		/**
		 * Class to save a COLLAborative Design Activity (COLLADA) stored inside
		 * a Digital Asset Exchange (DAE) XML document.
		 *
		 * @author Benjamin Eikel
		 * @date 2009-08-08
		 * @see http://www.khronos.org/collada/
		 */
		class WriterDAE {
			public:
				/**
				 * Save the scene given by its root node to a document given by
				 * its file name.
				 *
				 * @param fileName File name of the file to save. If empty,
				 * output the data to standard output.
				 * @param scene Root node of the scene graph
				 * @return @c true on success, @c false otherwise
				 */
				MINSGAPI static bool
						saveFile(const Util::FileName & fileName, Node * scene);

			private:
				/**
				 * Check if there is already an identifier for the object in the
				 * given mapping and return that if it exists. Otherwise
				 * generate a new identifier and add it to the mapping.
				 *
				 * @param object Pointer to an object to check.
				 * @param mapping Mapping to store generated identifiers.
				 * @return Empty string if the object is invalid or an
				 * identifier for the object otherwise.
				 */
				template<typename _T>
				static std::string getOrGenerateId(_T * object, std::map<_T *,
						std::string> & mapping);
		};
	}
}
#endif // WRITERDAE_H
