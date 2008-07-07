#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect textureIn;

vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{
  vec2 l;
  vec4 r;
  vec2 fullpel_pos = -sign(gl_TexCoord[1]);
  float halfpel;
  float intpel;

  l = vec2(texture2DRect(textureIn, gl_TexCoord[0].st+fullpel_pos*2).r,
           texture2DRect(textureIn, gl_TexCoord[0].st+fullpel_pos).r);
  
  r = vec4(texture2DRect(textureIn, gl_TexCoord[0].st).r,
           texture2DRect(textureIn, gl_TexCoord[0].st-fullpel_pos).r,
           texture2DRect(textureIn, gl_TexCoord[0].st-fullpel_pos*2).r,
           texture2DRect(textureIn, gl_TexCoord[0].st-fullpel_pos*3).r);

  //Texturein contains the previous frame
  halfpel = floor(( dot(l*coeffs_l, vec2(1.0, 1.0)) + dot(r*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 16)/32);

  //FIXME: is it +?
  intpel = texture2DRect(textureIn, gl_TexCoord[0].st+ceil(gl_TexCoord[1].st-vec2(0.5, 0.5))).r;

  gl_FragColor = floor( (intpel + halfpel + 1)/2)/255;
}
