#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 frag_position;
layout (location = 1) in vec3 frag_normal;
layout (location = 2) in vec2 frag_tex_coord;

layout (std140, binding = 0) uniform Camera
{
  mat4 projection;
  mat4 view;
  vec3 eye;
} camera;

#include "light.glsl"

const int MAX_NUM_LIGHTS = 8;
layout (std140, binding = 1) uniform LightUbo
{
  Light lights[MAX_NUM_LIGHTS];
} lights;

layout (binding = 2) uniform sampler2D tex;

layout (location = 0) out vec4 out_color;

void main()
{
  // Directional light
  vec3 N = normalize(frag_normal);
  vec3 V = normalize(camera.eye - frag_position);

  Material material;
  material.diffuse.rgb = texture(tex, frag_tex_coord).rgb;
  material.specular.rgb = vec3(0.1f, 0.1f, 0.1f);
  material.shininess = 1.f;

  vec3 total_color = vec3(0.f, 0.f, 0.f);
  for (int i = 0; i < MAX_NUM_LIGHTS; i++)
  {
    vec3 light_color = compute_light_color(lights.lights[i], material, frag_position, N, V);
    total_color += light_color;
  }

  out_color = vec4(total_color, 1.f);
}
