/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_THESISPETER

#ifndef MINSG_THESISPETER_LIGHTNODEMANAGER_H_
#define MINSG_THESISPETER_LIGHTNODEMANAGER_H_

namespace Rendering {
	class Mesh;
}

namespace MinSG {
namespace ThesisPeter {

class LightNodeManager {
//	PROVIDES_TYPE_NAME(ThesisPeter::LightNodeManager)
public:
	void addAttribute(Rendering::Mesh *mesh);
private:

};

}
}

#endif /* MINSG_THESISPETER_LIGHTNODEMANAGER_H_ */

#endif /* MINSG_EXT_THESISPETER */
