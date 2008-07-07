uniform sampler2DRect textureIn;

//Coefficients of the 6 tap filter
vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{

  vec2 l[6], v2;
  vec4 r[6], v4;
  mat3 pels;
  float b, j, s;

  for(int i=0; i<6; i++)
  {
      l[i] = vec2(texture2DRect(textureIn, gl_TexCoord[0].st+vec2(-2.0, i-2.0)).r,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(-1.0, i-2.0)).r);
      
      r[i] = vec4(texture2DRect(textureIn, gl_TexCoord[0].st+vec2(0.0, i-2.0)).r,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(1.0, i-2.0)).r,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(2.0, i-2.0)).r,
                  texture2DRect(textureIn, gl_TexCoord[0].st+vec2(3.0, i-2.0)).r);
  }

  //Left convolution
  v2 = l[0] - 5*l[1] + 20*l[2] + 20*l[3] -5*l[4] + l[5];

  //Right convolution
  v4 = r[0] - 5*r[1] + 20*r[2] + 20*r[3] -5*r[4] + r[5];

  //Texturein contains the previous frame
  gl_FragColor = floor((dot(v2*coeffs_l, vec2(1.0, 1.0)) + dot(v4*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 512)/1024)/255;
  //gl_FragColor = v4.x;

}
