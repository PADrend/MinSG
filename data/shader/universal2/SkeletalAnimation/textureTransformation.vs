/*
	This file is part of the MinSG library.
	Copyright (C) 2012-2013 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

uniform sampler1D sg_texture7;
uniform bool sg_textureEnabled[8];

void doJointTransformations(inout vec4 newPosition_hms, inout vec4 newNormal_hms, in int weightCount, in float weights[16], in float weightsIndex[16])
{
    if(!sg_textureEnabled[7])
        return;
    
    // bringing mesh into bind pose
    int jointSize = int(texelFetch1D(sg_texture7, 0, 0).x) * 4;
    
    mat4 invWorldMatrix = mat4(texelFetch1D(sg_texture7, 1, 0),
                               texelFetch1D(sg_texture7, 2, 0),
                               texelFetch1D(sg_texture7, 3, 0),
                               texelFetch1D(sg_texture7, 4, 0));
    
    vec4 position =  vec4(newPosition_hms.xyz, 1.0);
    vec4 normal =  vec4(newNormal_hms.xyz, 1.0);
    if(weightCount < 1)
    {
        newPosition_hms = position;
        newNormal_hms = normal;
        return;
    }
    
    newPosition_hms = vec4(0.0);
    newNormal_hms = vec4(0.0);
    int index;
    mat4 skin;
    for(int i=0; i<weightCount; i++) 
    {
        index = int(weightsIndex[i]) * 4;
        mat4 jointMat = mat4(texelFetch1D(sg_texture7, 9+jointSize+index, 0),
                             texelFetch1D(sg_texture7, 9+jointSize+index+1, 0),
                             texelFetch1D(sg_texture7, 9+jointSize+index+2, 0),
                             texelFetch1D(sg_texture7, 9+jointSize+index+3, 0));
        
        mat4 jointInvMat = mat4(texelFetch1D(sg_texture7, 9+index, 0),
                                texelFetch1D(sg_texture7, 9+index+1, 0),
                                texelFetch1D(sg_texture7, 9+index+2, 0),
                                texelFetch1D(sg_texture7, 9+index+3, 0));
        
        skin = jointInvMat * (jointMat * invWorldMatrix);
        newPosition_hms += weights[i] * position * skin;
        newNormal_hms += weights[i] * normal * skin;
    }
}
