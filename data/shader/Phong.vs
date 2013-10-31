#version 150

/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
 Important: Do not rename or remove this file without changing the search for
            this file in MinSG::DataDirectory::getPath().
*/

uniform mat4 sg_projectionMatrix;
uniform mat4 sg_modelViewMatrix;

in vec3 sg_Position;
in vec4 sg_Normal;
in vec4 sg_Color;
in vec2 sg_TexCoord0;

out vec3 position;
out vec3 normal;
out vec4 color;
out vec2 texCoord0;

void main() {
	normal = normalize(sg_Normal.xyz);
	texCoord0 = sg_TexCoord0;
	color = sg_Color;
	vec4 eyePosition = sg_modelViewMatrix * vec4(sg_Position, 1.0);
	position = eyePosition.xyz / eyePosition.w;
	gl_Position = sg_projectionMatrix * eyePosition;
}
