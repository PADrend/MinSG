/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2013 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef SHAPE_H_
#define SHAPE_H_

#include <string>
#include <vector>
#include <Geometry/Vec3.h>

namespace Rendering {
class Mesh;
}  // namespace Rendering

namespace D3Fact {

class Shape {
public:
	static Rendering::Mesh* createShape(const std::string& shape, const std::vector<Geometry::Vec3>& points);
	static void updateShape(const std::string& shape, Rendering::Mesh* mesh, const std::vector<Geometry::Vec3>& points);
};

} /* namespace D3Fact */
#endif /* SHAPE_H_ */
#endif /* MINSG_EXT_D3FACT */
