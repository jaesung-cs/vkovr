struct Light
{
  // Light is turned off if position = (0, 0, 0, 0)
  vec4 position;
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
};

struct Material
{
  vec4 diffuse;
  vec4 specular;
  float shininess;
};

vec3 compute_light_color(Light light, Material material, vec3 P, vec3 N, vec3 V)
{
  // Light is off
  if (dot(light.position, light.position) == 0.f)
    return vec3(0.f);

  float atten;
  vec3 L;
  if (light.position.w == 1.f) // Positional light
  {
    float d = length(light.position.xyz - P);
    atten = 1.f / (0.1f * d * d + 1.f);
    L = normalize(light.position.xyz - P);
  }
  else // Directional light
  {
    atten = 1.f;
    L = normalize(light.position.xyz);
  }
  vec3 R = reflect(-L, N);
  vec3 H = normalize(L + V);

  float diffuse_strength = max(dot(L, N), 0.f);
  float specular_strength = pow(max(dot(H, N), 0.f), material.shininess);

  vec3 color = atten * (
    light.ambient.rgb * material.diffuse.rgb
    + diffuse_strength * light.diffuse.rgb * material.diffuse.rgb
    + specular_strength * light.specular.rgb * material.specular.rgb);

  return color;
}
