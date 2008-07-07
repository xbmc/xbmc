#extension GL_ARB_texture_rectangle : enable

//Texturein contains the previous frame
uniform sampler2DRect textureIn;

//Coefficients for the 6 tap filter
vec2 coeffs_l = vec4(1.0, -5.0);
vec2 coeffs_r = vec2(20.0, 20.0,-5.0, 1.0);


void main(void)
{//implementation based on xbox 360 GPU implementation

  vec2 l[6], v2;
  vec4 r[6], v4;
  mat3 pels;
  float b, j, s;

  for(int i=0; i<6; i++)
  {
      l[i] = vec2(texture2DRect(textureIn, gl_TexCoord[0].st+vec2(-2.0, i-2.0)).r/255,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(-1.0, i-2.0)).r/255);
      
      r[i] = vec4(texture2DRect(textureIn, gl_TexCoord[0].st+vec2(0.0, i-2.0)).r/255,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(1.0, i-2.0)).r/255,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(2.0, i-2.0)).r/255,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(3.0, i-2.0)).r/255);
  }

  //Left convolution
  v2 = l[0] - 5*l[1] + 20*l[2] + 20*l[3] -5*l[4] + l[5];

  //Right convolution
  v4 = r[0] - 5*r[1] + 20*r[2] + 20*r[3] -5*r[4] + r[5];

  j = dot(v2*coeffs_l, vec2(1.0, 1.0)) + dot(v4*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0));
o
  b = dot(l[2]*coeffs_l, vec2(1.0, 1.0)) + dot(r[2]*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0));

  s = dot(l[3]*coeffs_l, vec2(1.0, 1.0)) + dot(r[3]*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0));

  pels3 = mat3(r[2].x, v4.x, r[3].x, b, j, s, r[2].y, v4.y, r[3].y);



  


}
