#pragma once
namespace scu::voxel {
// GLSL 1.20 / ES 1.00 share this small, original textured-volume shader.
// It has one texture lookup and bounded directional/hemisphere lighting.
inline constexpr const char *volumeVertex = R"GLSL(
attribute vec3 aPosition;
attribute vec3 aNormal;
attribute vec2 aTexcoord;
uniform mat4 uCamera;
varying SCU_VARYING vec3 vNormal;
varying SCU_VARYING vec2 vTexcoord;
void main(){
 gl_Position=uCamera*vec4(aPosition,1.0);
 vNormal=aNormal;vTexcoord=aTexcoord;
}
)GLSL";
inline constexpr const char *volumeFragment = R"GLSL(
uniform sampler2D uImage;
uniform float uLight;
varying SCU_VARYING vec3 vNormal;
varying SCU_VARYING vec2 vTexcoord;
void main(){
#ifdef SCU_BIG_ENDIAN
 vec4 pixel=texture2D(uImage,vTexcoord).gbar;
#else
 vec4 pixel=texture2D(uImage,vTexcoord).bgra;
#endif
 if(pixel.a<0.002)discard;
 vec3 n=normalize(vNormal);
 float diffuse=max(dot(n,normalize(vec3(-0.35,-0.7,0.85))),0.0);
 float shade=mix(1.0,0.48+0.52*diffuse,clamp(uLight,0.0,1.0));
 vec3 color=pixel.rgb*shade;
 // Premultiplied output preserves translucent spawn previews and cutout edges.
 gl_FragColor=vec4(color*pixel.a,pixel.a);
}
)GLSL";
} // namespace scu::voxel
