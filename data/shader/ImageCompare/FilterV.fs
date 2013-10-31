#version 130

/*
	This file is part of the MinSG library.
	Copyright (C) 2011 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

out vec4 fragColor;

uniform sampler2D A;
uniform int filterSize;
uniform float filterValues[16];
uniform int[4] sg_viewport;

void main(void){

	ivec2 pos = ivec2(gl_FragCoord.xy);

	vec4 color = texelFetch(A, pos, 0) * filterValues[0];
	
	for(int y = 1; y <= 15; y++){
		if(y>filterSize)
			break;
		ivec2 src = pos + ivec2(0,y);
		if(all(greaterThanEqual(src, ivec2(sg_viewport[0],sg_viewport[1]))) && all(lessThan(src, ivec2(sg_viewport[0]+sg_viewport[2],sg_viewport[1]+sg_viewport[3])))){
				color += texelFetch(A, src, 0) * filterValues[y];
		}
		src = pos - ivec2(0,y);
		if(all(greaterThanEqual(src, ivec2(sg_viewport[0],sg_viewport[1]))) && all(lessThan(src, ivec2(sg_viewport[0]+sg_viewport[2],sg_viewport[1]+sg_viewport[3])))){
				color += texelFetch(A, src, 0) * filterValues[y];
		}
	}
	
	fragColor = color;
}
