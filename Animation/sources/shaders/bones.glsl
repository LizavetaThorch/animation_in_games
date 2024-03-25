#shader bones

layout(std140, binding = 0) uniform GlobalRenderData
{
    mat4 ViewProjection;
    vec3 CameraPosition;
    vec3 LightDirection;
    vec3 AmbientLight;
    vec3 SunLight;
};

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
};

struct Instance
{
    mat4 BoneTransform;
    vec4 Color;
};
layout(std430, binding = 1) readonly buffer InstanceData
{
    Instance instances[];
};


#vertex_shader

layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;

out VsOutput vsOutput;

void main()
{
  instanceID = gl_InstanceID;
  vec4 worldPos = instances[instanceID].BoneTransform * vec4(Position, 1);
  vsOutput.WorldPosition = worldPos.xyz;
  gl_Position = ViewProjection * worldPos;
  mat3 ModelNorm = mat3(instances[instanceID].BoneTransform);
  ModelNorm[0] = normalize(ModelNorm[0]);
  ModelNorm[1] = normalize(ModelNorm[1]);
  ModelNorm[2] = normalize(ModelNorm[2]);
  vsOutput.EyespaceNormal = normalize(ModelNorm * Normal);

}

#pixel_shader

in VsOutput vsOutput;
out vec4 FragColor;

vec3 LightedColor(
  vec3 color,
  float shininess,
  float metallness,
  vec3 world_position,
  vec3 world_normal,
  vec3 light_dir,
  vec3 camera_pos)
{
  vec3 W = normalize(camera_pos - world_position);
  vec3 E = reflect(light_dir, world_normal);
  float df = max(0.0, dot(world_normal, -light_dir));
  float sf = max(0.0, dot(E, W));
  sf = pow(sf, shininess);
  return color * (AmbientLight + df * SunLight) + vec3(1,1,1) * sf * metallness;
}
void main()
{
  float shininess = 40;
  float metallness = 0;
  vec3 color = LightedColor(instances[instanceID].Color.rgb, shininess, metallness,
    vsOutput.WorldPosition, vsOutput.EyespaceNormal, LightDirection, CameraPosition);
  FragColor = vec4(color, 1.0);
}