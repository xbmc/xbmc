
uniform sampler2DRect textureIn;

//Coefficients of the 6 tap filter
vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{

  vec2 mv, horiz_l, vert_l;
  vec4 horiz_r, vert_r;
  float intpel1, intpel2;

  mv = floor(gl_TexCoord[1].st+0.5);


  horiz_l = vec2(texture2DRect(textureIn, gl_TexCoord[0].st+vec2(-2.0, mv.y)).r,
                 texture2DRect(textureIn, gl_TexCoord[0].st+vec2(-1.0, mv.y)).r);
  
  horiz_r = vec4(texture2DRect(textureIn, gl_TexCoord[0].st+vec2( 0.0, mv.y)).r,
                 texture2DRect(textureIn, gl_TexCoord[0].st+vec2( 1.0, mv.y)).r,
                 texture2DRect(textureIn, gl_TexCoord[0].st+vec2( 2.0, mv.y)).r,
                 texture2DRect(textureIn, gl_TexCoord[0].st+vec2( 3.0, mv.y)).r);


  vert_l = vec2(texture2DRect(textureIn, gl_TexCoord[0].st+vec2(mv.x, -2.0)).r,
                texture2DRect(textureIn, gl_TexCoord[0].st+vec2(mv.x, -1.0)).r);
  
  vert_r = vec4(texture2DRect(textureIn, gl_TexCoord[0].st+vec2(mv.x,  0.0)).r,
                texture2DRect(textureIn, gl_TexCoord[0].st+vec2(mv.x,  1.0)).r,
                texture2DRect(textureIn, gl_TexCoord[0].st+vec2(mv.x,  2.0)).r,
                texture2DRect(textureIn, gl_TexCoord[0].st+vec2(mv.x,  3.0)).r);

  
  intpel1 = floor( (dot(horiz_l*coeffs_l, vec2(1.0, 1.0)) + dot(horiz_r*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 16)/32);
  intpel2 = floor( (dot(vert_l *coeffs_l, vec2(1.0, 1.0)) + dot(vert_r *coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 16)/32);

  gl_FragColor = floor((intpel1+intpel2+1)/2)/255;
  
}
