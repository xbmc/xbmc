#extension GL_ARB_texture_rectangle : enable

uniform sampler3D dpb;

//Coefficients of the 6 tap filter
vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{
  vec2 l;
  vec4 r;

  l = vec2(float(texture2DRect(dpb, gl_TexCoord[0].st+gl_TexCoord[1].st*2.0).r),
           float(texture2DRect(dpb, gl_TexCoord[0].st+gl_TexCoord[1].st).r));
  
  r = vec4(float(texture2DRect(dpb, gl_TexCoord[0].st).r),
           float(texture2DRect(dpb, gl_TexCoord[0].st-gl_TexCoord[1].st).r),
           float(texture2DRect(dpb, gl_TexCoord[0].st-gl_TexCoord[1].st*2.0).r),
           float(texture2DRect(dpb, gl_TexCoord[0].st-gl_TexCoord[1].st*3.0).r));

  //Texturein contains the previous frame
  gl_FragColor = floor(( dot(l*coeffs_l, vec2(1.0, 1.0)) + dot(r*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 16.0)/32.0)/255.0;
  //gl_FragColor =  dot(l*coeffs_l, vec2(1.0, 1.0));
  //gl_FragColor =  l.x;
  //gl_FragColor = vec4(0.0, 0.5, 0.0, 0.0);
}
