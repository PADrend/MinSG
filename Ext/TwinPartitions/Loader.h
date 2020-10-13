/*
	This file is part of the MinSG library extension TwinPartitions.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_TWIN_PARTITIONS

#ifndef LOADER_H_
#define LOADER_H_

namespace Util {
class FileName;
}

namespace MinSG {
class Node;

namespace TwinPartitions {

/**
 * Class to load a reduced object space and a reduced view space from a text file.
 *
 * @author Benjamin Eikel
 * @date 2010-09-22
 */
struct Loader {
	MINSGAPI static Node * importPartitions(const Util::FileName & fileName);
};

}
}

#endif /* LOADER_H_ */
#endif /* MINSG_EXT_TWIN_PARTITIONS */
