/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SG_GEOMETRYNODE_H
#define SG_GEOMETRYNODE_H

#include "Node.h"

namespace Rendering {
//class VBOWrapper;
class Mesh;
}

namespace MinSG {


/**
 *  Node containing geometry.
 *
 *   [GeometryNode] ---|> [Node]
 */
class GeometryNode : public Node {
		PROVIDES_TYPE_NAME(GeometryNode)

	public:
		GeometryNode();
		GeometryNode(const Util::Reference<Rendering::Mesh> & _mesh);
		virtual ~GeometryNode();

		void setMesh(const Util::Reference<Rendering::Mesh> & newMesh);
		Rendering::Mesh * getMesh()const                {   return mesh.get();  }
		bool hasMesh()const                				{   return mesh.isNotNull();  }

		uint32_t getTriangleCount()const;
		uint32_t getVertexCount()const;

		/// ---|> [Node]
		void doDisplay(FrameContext & context, const RenderParam & rp) override;

		/**
		 * Get the amount of memory that is required to store this node. The
		 * returned value does not include the size of the mesh.
		 * 
		 * @return Amount of memory in bytes
		 */
		size_t getMemoryUsage() const override;

	protected:
		// ----
		Util::Reference<Rendering::Mesh> mesh;
		explicit GeometryNode(const GeometryNode & source);
	private:
		/// ---|> [Node]
		const Geometry::Box& doGetBB() const override;


		/// ---|> [Node]
		GeometryNode * doClone()const override			{	return new GeometryNode(*this);	}

};

}

#endif // SG_GEOMETRYNODE_H
