#version 130

/*
	This file is part of the MinSG library.
	Copyright (C) 2011-2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

out vec4 fragColor;

uniform sampler2D A;
uniform int sg_scissorRect[4];
uniform bool sg_scissorEnabled;
uniform int borderSize = 2;
uniform vec4 borderColor = vec4(0,0,0,1);

void main(void){

	ivec2 pos = ivec2(gl_FragCoord.xy);
	
	if(	sg_scissorEnabled && (
		sg_scissorRect[0] > pos.x-borderSize || 
		sg_scissorRect[1] > pos.y-borderSize || 
		sg_scissorRect[0] + sg_scissorRect[2] <= pos.x+borderSize || 
		sg_scissorRect[1] + sg_scissorRect[3] <= pos.y+borderSize
	)){
		fragColor = borderColor;
	}
	else
		fragColor = texelFetch(A, pos, 0);
}
