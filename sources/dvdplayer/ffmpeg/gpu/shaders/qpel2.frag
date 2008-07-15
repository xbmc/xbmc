/*
  Motion compensation for quarter pixel resolution motion vectors.
  This handles the umm.. something case
*/


uniform sampler3D dpb;

//Coefficients of the 6 tap filter
vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{

  vec2 l[6], v2;
  vec3 row, col;
  vec4 r[6], v4;
  float mv_x, mv_y, halfpel, centerpel, result;

  mv_x = gl_TexCoord[1].s;
  mv_y = gl_TexCoord[1].t;

  if(mv_y == 0.5)
    {
      row = vec3(1.0, 0.0, 0.0);
      col = vec3(0.0, 1.0, 0.0);
    }
  else
    {
      row = vec3(0.0, 1.0, 0.0);
      col = vec3(1.0, 0.0, 0.0);
    }

  row *= gl_TexCoord[2].xyz;

  for(int i=0; i<6; i++)
  {
    vec3 col_pos =  col*(i-2.0)*gl_TexCoord[2].y;
    l[i] = vec2(texture3D(dpb, gl_TexCoord[0].stp - row*2.0  + col_pos).b,
                texture3D(dpb, gl_TexCoord[0].stp  - row      + col_pos).b);
    
    r[i] = vec4(texture3D(dpb, gl_TexCoord[0].stp            + col_pos).b,
                texture3D(dpb, gl_TexCoord[0].stp + row      + col_pos).b,
                texture3D(dpb, gl_TexCoord[0].stp + row*2.0  + col_pos).b,
                texture3D(dpb, gl_TexCoord[0].stp + row*3.0  + col_pos).b);
  }



  //Left convolution
  v2 = l[0] - 5*l[1] + 20*l[2] + 20*l[3] -5*l[4] + l[5];

  //Right convolution
  v4 = r[0] - 5*r[1] + 20*r[2] + 20*r[3] -5*r[4] + r[5];

  if(mv_x > 0.5 || mv_y > 0.5)
    halfpel = (v4.y + 16.0/255.0)/32.0;
  else
    halfpel = (v4.x + 16.0/255.0)/32.0;

  centerpel = (dot(v2*coeffs_l, vec2(1.0, 1.0)) + 
	       dot(v4*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) +
	       512.0/255.0)/1024.0;

  halfpel    = floor(  halfpel*255.0)/255.0;
  centerpel  = floor(centerpel*255.0)/255.0;
  result     = (centerpel + halfpel + 1.0/255.0)/2.0;
 
  gl_FragColor = vec4(floor(result*255.0)/255.0);
}
