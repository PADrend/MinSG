/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef LODRENDERER_H
#define LODRENDERER_H

#include "../../Core/States/NodeRendererState.h"

namespace MinSG{
	
//! @ingroup states
class LODRenderer : public NodeRendererState{
	PROVIDES_TYPE_NAME(LODRenderer)
private:
	uint32_t minComplexity;
	uint32_t maxComplexity;
	float relComplexity;

public:
    MINSGAPI LODRenderer();
	
	MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;
	
	MINSGAPI void generateLODsRecursiv(Node * node);
	
	uint32_t getMinComplexity() const { return minComplexity;}
	
	uint32_t getMaxComplexity() const { return maxComplexity;}
	
	float getRelComplexity() const { return relComplexity;}
	
	void setMinComplexity(size_t c){ minComplexity = c;}
	
	void setMaxComplexity(size_t c){ maxComplexity = c;}
	
	void setRelComplexity(float c){ relComplexity = c;}

	LODRenderer* clone() const override { return new LODRenderer(*this); };
};

}

#endif // LODRENDERER_H
