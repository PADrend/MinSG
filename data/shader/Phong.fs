#version 150

/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

in vec3 position;
in vec3 normal;
in vec4 color;
in vec2 texCoord0;

uniform bool sg_useMaterials;
struct sg_MaterialParameters {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};
uniform sg_MaterialParameters sg_Material;

uniform int sg_lightCount;
struct sg_LightSourceParameters {
	int type; // has to be DIRECTIONAL, POINT or SPOT
	
	vec3 position; // position of the light
	vec3 direction; // direction of the light, has to be normalized
	
	// light colors for all lights
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	
	// attenuations for point & spot lights
	float constant;
	float linear;
	float quadratic;
	
	// spot light parameters
	float exponent;
	float cosCutoff;
};
uniform sg_LightSourceParameters sg_LightSource[8];

uniform bool sg_textureEnabled[8];
uniform sampler2D sg_texture0;

out vec4 FragColor;

void pointLight(in int lightNum, in vec3 pixelNormal, inout vec4 ambient, inout vec4 diffuse, inout vec4 specular) {
	vec3 lightDir = sg_LightSource[lightNum].position - position;
	float distance = length(lightDir);
	lightDir = normalize(lightDir);
	float attenuation = 1.0 / (	sg_LightSource[lightNum].constant +
								sg_LightSource[lightNum].linear * distance +
								sg_LightSource[lightNum].quadratic * distance * distance);
	vec3 viewDir = normalize(-position);
	vec3 halfVector = normalize(lightDir + viewDir);
	float norDotL = max(0.0, dot(pixelNormal, lightDir));
	float norDotH = max(0.0, dot(pixelNormal, halfVector));
	float powerFactor;
	if (norDotL == 0.0) {
		powerFactor = 0.0;
	} else {
		if(sg_useMaterials) {
			powerFactor = pow(norDotH, sg_Material.shininess);
		} else {
			powerFactor = pow(norDotH, 32.0);
		}
	}
	ambient  += sg_LightSource[lightNum].ambient * attenuation;
	diffuse  += sg_LightSource[lightNum].diffuse * norDotL * attenuation;
	specular += sg_LightSource[lightNum].specular * powerFactor * attenuation;
}

void main() {
	vec3 pixelNormal = normalize(normal);

	vec4 ambient = vec4(0.0);
	vec4 diffuse = vec4(0.0);
	vec4 specular = vec4(0.0);

	for(int i = 0; i < sg_lightCount; i++) {
		pointLight(i, pixelNormal, ambient, diffuse, specular);
	}

	ambient.a = 1.0;
	diffuse.a = 1.0;
	specular.a = 1.0;

	vec4 textureColor = vec4(1.0);
    if(sg_textureEnabled[0]) {
    	textureColor = texture(sg_texture0, texCoord0);
    }
    if(sg_useMaterials) {
    	FragColor = textureColor * ambient * sg_Material.ambient + diffuse * sg_Material.diffuse + specular * sg_Material.specular;
    } else {
    	FragColor = textureColor * (ambient + diffuse + specular) * color;
    }
}
