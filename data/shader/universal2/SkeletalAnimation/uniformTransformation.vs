/*
	This file is part of the MinSG library.
	Copyright (C) 2012-2013 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#define MAXJOINTS 8
uniform mat4 joints[MAXJOINTS];
uniform mat4 jointInv[MAXJOINTS]; 
uniform mat4 invWorldMatrix;

void doJointTransformations(inout vec4 newPosition_hms, inout vec4 newNormal_hms, in int weightCount,
                            in float weights[16], in float weightsIndex[16])
{        
    // bringing mesh into bind pose    
    vec4 position = vec4(newPosition_hms.xyz, 1.0);
    vec4 normal = vec4(newNormal_hms.xyz, 1.0);
    
    newPosition_hms = vec4(0.0); 
    newNormal_hms = vec4(0.0);
    int index;
    mat4 skin;
    for(int i=0; i<weightCount; i++) 
    {
        index = int(weightsIndex[i]);
        skin = invWorldMatrix * joints[index] * jointInv[index];
        newPosition_hms += weights[i] * skin * position;
        newNormal_hms += weights[i] * skin * normal; 
    }
}
