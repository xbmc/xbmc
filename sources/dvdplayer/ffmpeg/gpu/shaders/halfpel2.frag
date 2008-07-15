/*
  Motion compensation for half pixel resolution motion vectors.
  This handles the center case
*/


uniform sampler3D dpb;

//Coefficients of the 6 tap filter
vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{

  vec2 l[6], v2;
  vec4 r[6], v4;
  vec4 result;
  float b, j, s;

  for(int i=0.0; i<6.0; i++)
  {
    float y_offset = (i-2.0)*gl_TexCoord[1].y;
    l[i] = vec2(float(texture3D(dpb, gl_TexCoord[0].stp +
				vec3(-2.0*gl_TexCoord[1].x, y_offset, 0.0))),
		float(texture3D(dpb, gl_TexCoord[0].stp +
				vec3(-1.0*gl_TexCoord[1].x, y_offset, 0.0))));

    r[i] = vec4(float(texture3D(dpb, gl_TexCoord[0].stp +
				vec3(  0.0*gl_TexCoord[1].x, y_offset, 0.0))),
		float(texture3D(dpb, gl_TexCoord[0].stp +
				vec3(  1.0*gl_TexCoord[1].x, y_offset, 0.0))),
		float(texture3D(dpb, gl_TexCoord[0].stp +
				vec3(  2.0*gl_TexCoord[1].x, y_offset, 0.0))),
		float(texture3D(dpb, gl_TexCoord[0].stp +
				vec3(  3.0*gl_TexCoord[1].x, y_offset, 0.0))));
  }

  // Left convolution
  v2 = l[0] - 5*l[1] + 20*l[2] + 20*l[3] -5*l[4] + l[5];

  // Right convolution
  v4 = r[0] - 5*r[1] + 20*r[2] + 20*r[3] -5*r[4] + r[5];

  // Don't forget we're working with floats scaled from 0 to 1
  // (We add 512.0/255.0 not 512)
  result = vec4((dot(v2*coeffs_l, vec2(1.0, 1.0)) + 
		 dot(v4*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) +
		 512.0/255.0)/1024.0);
  
  // The division in h.264 motion compensation is integer division. So we need
  // to multiply out from the normalized range and do a floor so rounding errors
  // don't screw us over.
  gl_FragColor = floor(result*255.0)/255.0;
}
