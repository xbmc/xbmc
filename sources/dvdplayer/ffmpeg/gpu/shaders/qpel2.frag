uniform sampler2DRect textureIn;

//Coefficients of the 6 tap filter
vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{

  vec2 l[6], v2,row, col;
  vec4 r[6], v4;
  mat3 pels;
  float mv_x, mv_y, halfpel;

  mv_x = gl_TexCoord[1].s;
  mv_y = gl_TexCoord[1].t;

  if(mv_y == 0.5)
    {
      row = vec2(1.0, 0.0);
      col = vec2(0.0, 1.0);
    }
  else
    {
      row = vec2(0.0, 1.0);
      col = vec2(1.0, 0.0);
    }


  for(int i=0; i<6; i++)
  {
    vec2 col_pos =  col*(i-2.0);
    l[i] = vec2(texture2DRect(textureIn, gl_TexCoord[0].st + row*-2.0 + col_pos).r,
                texture2DRect(textureIn, gl_TexCoord[0].st - row      + col_pos).r);
    
    r[i] = vec4(texture2DRect(textureIn, gl_TexCoord[0].st            + col_pos).r,
                texture2DRect(textureIn, gl_TexCoord[0].st + row      + col_pos).r,
                texture2DRect(textureIn, gl_TexCoord[0].st + row*2.0  + col_pos).r,
                texture2DRect(textureIn, gl_TexCoord[0].st + row*3.0  + col_pos).r);
  }



  //Left convolution
  v2 = l[0] - 5*l[1] + 20*l[2] + 20*l[3] -5*l[4] + l[5];

  //Right convolution
  v4 = r[0] - 5*r[1] + 20*r[2] + 20*r[3] -5*r[4] + r[5];

  if(mv_x > 0.5 || mv_y > 0.5)
    halfpel = floor((v4.y + 16)/32);
  else
    halfpel = floor((v4.x + 16)/32);

  //Texturein contains the previous frame
  // gl_FragColor = floor((intpel + dot(v2*coeffs_l, vec2(1.0, 1.0)) + dot(v4*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 1)/2);
  gl_FragColor = floor( (floor((dot(v2*coeffs_l, vec2(1.0, 1.0)) + dot(v4*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 512)/1024) + halfpel +1)/2)/255;
  //gl_FragColor = vec4(0.0, 1.0, 0.0, 0.0);
}
