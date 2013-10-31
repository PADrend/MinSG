/*
	This file is part of the MinSG library.
	Copyright (C) 2011-2013 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

uniform int debugJointId;
varying vec4 debugColor;
varying float debug;

attribute vec4 sg_Weights1;
attribute vec4 sg_WeightsIndex1;
attribute vec4 sg_Weights2;
attribute vec4 sg_WeightsIndex2;
attribute vec4 sg_Weights3;
attribute vec4 sg_WeightsIndex3;
attribute vec4 sg_Weights4;
attribute vec4 sg_WeightsIndex4;

attribute float sg_WeightsCount;

void doJointTransformations(inout vec4 newPosition_hms, inout vec4 newNormal_hms, in int weightCount, in float weights[16], in float weightsIndex[16]);

void calcDebugColor(in float weights[16], in float weightsIndex[16])
{
    debugColor = vec4(0.0, 0.0, 0.0, 1.0);
    debug = 1.0;
    
    int weightCount = int(sg_WeightsCount);
    int index;
    
    for(int i=0; i<weightCount; ++i)
    {
        index = int(weightsIndex[i]);
        if(index == debugJointId)
            debugColor += vec4(weights[i], 0.0, 0.0, 0.0);
    }
    debugColor.b = 1.0 - debugColor.r;
}

void sg_provideVertexEffect(inout vec3 position_ms, inout vec3 normal_ms)
{
    if(sg_WeightsCount < 0.01)
        return;
    
    vec4 newPosition_hms = vec4(position_ms, 1.0);
    vec4 newNormal_hms = vec4(normal_ms, 1.0);
    int weightCount = int(sg_WeightsCount);
    
    float weights[16];
    float weightsIndex[16];
    
    for(int i=0; i<weightCount; i++)
    {
        if(i<4)
        {
            weights[i] = sg_Weights1[i];
            weightsIndex[i] = sg_WeightsIndex1[i];
        }else if(i<8)
        {
            weights[i] = sg_Weights2[i-4];
            weightsIndex[i] = sg_WeightsIndex2[i-4];
        }else if(i<12)
        {
            weights[i] = sg_Weights3[i-8];
            weightsIndex[i] = sg_WeightsIndex3[i-8];
        }else if(i<16)
        {
            weights[i] = sg_Weights4[i-12];
            weightsIndex[i] = sg_WeightsIndex4[i-12];
        }
    }
    
    doJointTransformations(newPosition_hms, newNormal_hms, weightCount, weights, weightsIndex);
    position_ms = newPosition_hms.xyz;
    normal_ms = newNormal_hms.xyz;
    
    if(debugJointId > -1)
        calcDebugColor(weights, weightsIndex);
    else
        debug = 0.0;
}
