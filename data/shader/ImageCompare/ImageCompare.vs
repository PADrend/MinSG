#version 130

/*
	This file is part of the MinSG library.
	Copyright (C) 2011 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

in vec3 sg_Position;

uniform mat4 sg_modelViewProjectionMatrix;

void main(void){
	gl_Position = sg_modelViewProjectionMatrix * vec4(sg_Position,1);
}
