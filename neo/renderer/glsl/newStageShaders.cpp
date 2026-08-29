/*
 * GLSL replacements for the ARB vertex/fragment programs referenced by
 * materials through "program" / "vertexProgram" / "fragmentProgram"
 * ("new shader stages"). Ported from the GLES2 shaders of DIII4A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "glsl_shaders.h"

// heatHaze.vfp -- deforms _currentRender by a scrolling normal map

const char * const heatHazeShaderVP = R"(
#version 100
precision mediump float;

// In
attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelViewMatrix;    // transposed: row N is u_modelViewMatrix[N]
uniform highp mat4 u_projectionMatrix;   // transposed: row N is u_projectionMatrix[N]
uniform highp vec4 u_vertexParm0;        // texture scrolling
uniform highp vec4 u_vertexParm1;        // magnitude of the distortion

// Out
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;

void main(void)
{
  // texture 1 takes the texture coordinates and adds a scroll
  var_TexCoord1 = vec4(attr_TexCoord.xy, 0.0, 0.0) + u_vertexParm0;

  // texture 2 takes the deform magnitude and scales it by the projection distance
  vec4 vec = vec4(0.0, 1.0, 0.0, 1.0);
  vec.z = dot(attr_Vertex, u_modelViewMatrix[2]);

  // magicProjectionAdjust approximates what the original program did using the X axis
  const float magicProjectionAdjust = 0.43;
  float x = dot(vec, u_projectionMatrix[1]) * magicProjectionAdjust;
  float w = dot(vec, u_projectionMatrix[3]);

  // don't let the recip get near zero for polygons that cross the view plane
  w = max(w, 1.0);
  x /= w;

  // clamp the distance so the deformations don't get too wacky near the view
  x = min(x, 0.02);

  var_TexCoord2 = vec4(x) * u_vertexParm1;

  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
)";

const char * const heatHazeShaderFP = R"(
#version 100
precision mediump float;

// In
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;

// Uniforms
uniform sampler2D u_fragmentMap0;    // _currentRender
uniform sampler2D u_fragmentMap1;    // normal map used to deform _currentRender
uniform highp vec4 u_nonPowerOfTwo;  // 1.0 to _currentRender conversion
uniform highp vec4 u_windowCoords;   // fragment position to 0.0 - 1.0 conversion

void main(void)
{
  // load the filtered normal map and convert to -1 to 1 range
  vec4 bumpMap = (texture2D(u_fragmentMap1, var_TexCoord1.xy) * 2.0) - 1.0;
  vec2 localNormal = bumpMap.wy;

  // calculate the screen texcoord in the 0.0 to 1.0 range
  vec2 screenTexCoord = gl_FragCoord.xy * u_windowCoords.xy;
  screenTexCoord += (localNormal * var_TexCoord2.xy);
  screenTexCoord = clamp(screenTexCoord, vec2(0.0), vec2(1.0));
  screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;

  gl_FragColor = texture2D(u_fragmentMap0, screenTexCoord);
}
)";

// heatHazeWithMask.vfp -- as above, but a mask texture scales the distortion

const char * const heatHazeWithMaskShaderVP = R"(
#version 100
precision mediump float;

// In
attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelViewMatrix;    // transposed
uniform highp mat4 u_projectionMatrix;   // transposed
uniform highp vec4 u_vertexParm0;        // texture scrolling
uniform highp vec4 u_vertexParm1;        // magnitude of the distortion

// Out
varying highp vec4 var_TexCoord0;
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;

void main(void)
{
  // texture 0 takes the texture coordinates unmodified, for the mask
  var_TexCoord0 = vec4(attr_TexCoord.xy, 0.0, 0.0);

  // texture 1 takes the texture coordinates and adds a scroll
  var_TexCoord1 = vec4(attr_TexCoord.xy, 0.0, 0.0) + u_vertexParm0;

  // texture 2 takes the deform magnitude and scales it by the projection distance
  vec4 vec = vec4(0.0, 1.0, 0.0, 1.0);
  vec.z = dot(attr_Vertex, u_modelViewMatrix[2]);

  const float magicProjectionAdjust = 0.43;
  float x = dot(vec, u_projectionMatrix[1]) * magicProjectionAdjust;
  float w = dot(vec, u_projectionMatrix[3]);

  w = max(w, 1.0);
  x /= w;
  x = min(x, 0.02);

  var_TexCoord2 = vec4(x) * u_vertexParm1;

  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
)";

const char * const heatHazeWithMaskShaderFP = R"(
#version 100
precision mediump float;

// In
varying highp vec4 var_TexCoord0;
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;

// Uniforms
uniform sampler2D u_fragmentMap0;    // _currentRender
uniform sampler2D u_fragmentMap1;    // normal map
uniform sampler2D u_fragmentMap2;    // mask
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_windowCoords;

void main(void)
{
  // load the distortion map
  vec4 mask = texture2D(u_fragmentMap2, var_TexCoord0.xy);

  // kill the pixel if the distortion wound up being very small
  mask.xy -= 0.01;
  if (any(lessThan(mask, vec4(0.0)))) { discard; }

  // load the filtered normal map and convert to -1 to 1 range
  vec4 bumpMap = (texture2D(u_fragmentMap1, var_TexCoord1.xy) * 2.0) - 1.0;
  vec2 localNormal = bumpMap.wy;
  localNormal *= mask.xy;

  // calculate the screen texcoord in the 0.0 to 1.0 range
  vec2 screenTexCoord = gl_FragCoord.xy * u_windowCoords.xy;
  screenTexCoord += (localNormal * var_TexCoord2.xy);
  screenTexCoord = clamp(screenTexCoord, vec2(0.0), vec2(1.0));
  screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;

  gl_FragColor = texture2D(u_fragmentMap0, screenTexCoord);
}
)";

// heatHazeWithMaskAndVertex.vfp -- as above, with vertex colour fading (particles)

const char * const heatHazeWithMaskAndVertexShaderVP = R"(
#version 100
precision mediump float;

// In
attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;
attribute highp vec4 attr_Color;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelViewMatrix;    // transposed
uniform highp mat4 u_projectionMatrix;   // transposed
uniform highp vec4 u_vertexParm0;        // texture scrolling
uniform highp vec4 u_vertexParm1;        // magnitude of the distortion

// Out
varying highp vec4 var_TexCoord0;
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;
varying lowp vec4 var_Color;

void main(void)
{
  var_TexCoord0 = vec4(attr_TexCoord.xy, 0.0, 0.0);
  var_TexCoord1 = vec4(attr_TexCoord.xy, 0.0, 0.0) + u_vertexParm0;

  vec4 vec = vec4(0.0, 1.0, 0.0, 1.0);
  vec.z = dot(attr_Vertex, u_modelViewMatrix[2]);

  const float magicProjectionAdjust = 0.43;
  float x = dot(vec, u_projectionMatrix[1]) * magicProjectionAdjust;
  float w = dot(vec, u_projectionMatrix[3]);

  w = max(w, 1.0);
  x /= w;
  x = min(x, 0.02);

  var_TexCoord2 = vec4(x) * u_vertexParm1;
  var_Color = attr_Color / 255.0;

  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
)";

const char * const heatHazeWithMaskAndVertexShaderFP = R"(
#version 100
precision mediump float;

// In
varying highp vec4 var_TexCoord0;
varying highp vec4 var_TexCoord1;
varying highp vec4 var_TexCoord2;
varying lowp vec4 var_Color;

// Uniforms
uniform sampler2D u_fragmentMap0;    // _currentRender
uniform sampler2D u_fragmentMap1;    // normal map
uniform sampler2D u_fragmentMap2;    // mask
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_windowCoords;

void main(void)
{
  vec4 mask = texture2D(u_fragmentMap2, var_TexCoord0.xy);

  mask.xy *= var_Color.xy;
  mask.xy -= 0.01;
  if (any(lessThan(mask, vec4(0.0)))) { discard; }

  vec4 bumpMap = (texture2D(u_fragmentMap1, var_TexCoord1.xy) * 2.0) - 1.0;
  vec2 localNormal = bumpMap.wy;
  localNormal *= mask.xy;

  vec2 screenTexCoord = gl_FragCoord.xy * u_windowCoords.xy;
  screenTexCoord += (localNormal * var_TexCoord2.xy);
  screenTexCoord = clamp(screenTexCoord, vec2(0.0), vec2(1.0));
  screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;

  gl_FragColor = texture2D(u_fragmentMap0, screenTexCoord);
}
)";

// colorProcess.vfp -- blends _currentRender towards a target hue (berserk, etc.)

const char * const colorProcessShaderVP = R"(
#version 100
precision mediump float;

// In
attribute highp vec4 attr_Vertex;
attribute highp vec4 attr_TexCoord;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp vec4 u_vertexParm0;    // fraction towards the target hue
uniform highp vec4 u_vertexParm1;    // target hue

// Out
varying highp vec4 var_TexCoord;
varying lowp vec4 var_Color;

void main(void)
{
  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

  var_Color = u_vertexParm1;
  var_TexCoord.x = attr_TexCoord.x;
  var_TexCoord.y = 1.0 - attr_TexCoord.y;
  var_TexCoord.z = u_vertexParm0.x;
  var_TexCoord.w = 1.0;
}
)";

const char * const colorProcessShaderFP = R"(
#version 100
precision mediump float;

// In
varying highp vec4 var_TexCoord;
varying lowp vec4 var_Color;

// Uniforms
uniform sampler2D u_fragmentMap0;    // _currentRender
uniform highp vec4 u_nonPowerOfTwo;

void main(void)
{
  vec4 src = texture2D(u_fragmentMap0, var_TexCoord.xy * u_nonPowerOfTwo.xy);
  vec4 target = var_Color * dot(vec3(0.333, 0.333, 0.333), src.xyz);
  gl_FragColor = mix(src, target, var_TexCoord.z);
}
)";

// enviroSuit.vfp -- D3XP full screen warp

const char * const enviroSuitShaderVP = R"(
#version 100
precision mediump float;

// In
attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp vec4 u_vertexParm0;    // scroll (unused)
uniform highp vec4 u_vertexParm1;    // magnitude of the distortion

// Out
varying vec2 var_TexCoord0;
varying vec4 var_Color;

void main(void)
{
  var_TexCoord0 = attr_TexCoord.xy;
  var_Color = u_vertexParm1;

  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
)";

const char * const enviroSuitShaderFP = R"(
#version 100
precision mediump float;

// In
varying vec2 var_TexCoord0;
varying vec4 var_Color;

// Uniforms
uniform sampler2D u_fragmentMap0;    // _currentRender
uniform sampler2D u_fragmentMap1;    // normal map used to deform _currentRender
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_windowCoords;

void main(void)
{
  vec4 unadjustedTC = gl_FragCoord * u_windowCoords;

  // compute warp factor
  vec4 R0 = texture2D(u_fragmentMap1, unadjustedTC.xy);
  R0 = R0 * var_Color;
  vec4 warpFactor = 1.0 - R0;

  // compute _currentRender perturbed texcoords
  R0 = unadjustedTC;
  R0.xy = R0.xy - 0.5;
  R0.xy = R0.xy * warpFactor.xy;
  R0.xy = R0.xy + 0.5;

  // scale by the screen non-power-of-two-adjust
  R0 = R0 * u_nonPowerOfTwo;

  gl_FragColor = vec4(texture2D(u_fragmentMap0, R0.xy).xyz, 1.0);
}
)";

// environment.vfp -- per-pixel cubic reflection

const char * const environmentShaderVP = R"(
#version 100
precision mediump float;

// In
attribute highp vec4 attr_Vertex;
attribute lowp vec4 attr_Color;
attribute vec3 attr_Normal;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform lowp float u_colorModulate;    // 0 or 1/255
uniform lowp float u_colorAdd;         // 0 or 1
uniform vec4 u_viewOrigin;             // in local space

// Out
varying lowp vec4 var_Color;
varying vec3 var_normalLocal;
varying vec3 var_toEyeLocal;

void main(void)
{
  var_normalLocal = attr_Normal;
  var_toEyeLocal = vec3(u_viewOrigin - attr_Vertex);

  var_Color = attr_Color * u_colorModulate + u_colorAdd;

  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
)";

const char * const environmentShaderFP = R"(
#version 100
precision mediump float;

// In
varying lowp vec4 var_Color;
varying vec3 var_normalLocal;
varying vec3 var_toEyeLocal;

// Uniforms
uniform samplerCube u_fragmentCubeMap0;
uniform lowp vec4 u_glColor;

void main(void)
{
  vec3 normal = normalize(var_normalLocal);
  vec3 toEye = normalize(var_toEyeLocal);

  float dotEN = dot(toEye, normal);
  vec3 reflectionVector = vec3(2.0) * normal * vec3(dotEN) - toEye;

  gl_FragColor = textureCube(u_fragmentCubeMap0, reflectionVector) * u_glColor * var_Color;
}
)";

// bumpyEnvironment.vfp -- as above, with the reflection perturbed by a normal map

const char * const bumpyEnvironmentShaderVP = R"(
#version 100
precision mediump float;

// In
attribute highp vec4 attr_Vertex;
attribute lowp vec4 attr_Color;
attribute vec3 attr_Tangent;
attribute vec3 attr_Bitangent;
attribute vec3 attr_Normal;
attribute vec2 attr_TexCoord;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelMatrix;      // transposed
uniform lowp float u_colorModulate;    // 0 or 1/255
uniform lowp float u_colorAdd;         // 0 or 1
uniform vec4 u_viewOrigin;             // in local space

// Out
varying vec2 var_TexCoord;
varying lowp vec4 var_Color;
varying mat3 var_TangentToWorldMatrix;
varying highp vec3 var_toEyeWorld;     // must be highp in GLES

void main(void)
{
  var_TexCoord = attr_TexCoord;

  var_toEyeWorld = mat3(u_modelMatrix) * vec3(u_viewOrigin - attr_Vertex);

  mat3 matTangentToLocal = mat3(
    clamp(attr_Tangent, vec3(-1.0), vec3(1.0)),
    clamp(attr_Bitangent, vec3(-1.0), vec3(1.0)),
    clamp(attr_Normal, vec3(-1.0), vec3(1.0))
  );
  var_TangentToWorldMatrix = mat3(u_modelMatrix) * matTangentToLocal;

  var_Color = attr_Color * u_colorModulate + u_colorAdd;

  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
)";

const char * const bumpyEnvironmentShaderFP = R"(
#version 100
precision mediump float;

// In
varying vec2 var_TexCoord;
varying lowp vec4 var_Color;
varying mat3 var_TangentToWorldMatrix;
varying highp vec3 var_toEyeWorld;

// Uniforms
uniform samplerCube u_fragmentCubeMap0;    // environment cube map
uniform sampler2D u_fragmentMap1;          // normal map
uniform lowp vec4 u_glColor;

void main(void)
{
  // doom3's normal map is not rgb
  vec3 localNormal = texture2D(u_fragmentMap1, var_TexCoord).agb;
  localNormal = localNormal * vec3(2.0) - vec3(1.0);
  localNormal.z = sqrt(max(0.0, 1.0 - localNormal.x * localNormal.x - localNormal.y * localNormal.y));
  localNormal = normalize(localNormal);

  vec3 globalNormal = var_TangentToWorldMatrix * localNormal;
  vec3 globalEye = normalize(var_toEyeWorld);

  float dotEN = dot(globalEye, globalNormal);
  vec3 globalReflect = 2.0 * (dotEN * globalNormal) - globalEye;

  gl_FragColor = vec4(textureCube(u_fragmentCubeMap0, globalReflect).rgb, 1.0);
}
)";
