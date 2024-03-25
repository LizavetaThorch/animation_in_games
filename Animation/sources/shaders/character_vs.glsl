
struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec2 UV;
};

uniform mat4 Transform;
uniform mat4 ViewProjection;


layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 UV;
layout(location = 3) in vec4 BoneWeights;
layout(location = 4) in uvec4 BoneIndex;

out VsOutput vsOutput;

void main()
{

  vec3 VertexPosition = (Transform * vec4(Position, 1)).xyz;
  vsOutput.EyespaceNormal = (Transform * vec4(Normal, 0)).xyz;

  gl_Position = ViewProjection * vec4(VertexPosition, 1);
  vsOutput.WorldPosition = VertexPosition;

  vsOutput.UV = UV;

}