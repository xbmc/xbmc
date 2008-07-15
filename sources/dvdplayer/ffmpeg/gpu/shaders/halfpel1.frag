/*
  Motion compensation for half pixel resolution motion vectors.
  This handles the non-center case
*/


uniform sampler3D dpb;

//Coefficients of the 6 tap filter
vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{
  vec2 l;
  vec4 r;
  float result;

  l = vec2(float(texture3D(dpb, gl_TexCoord[0].stp-gl_TexCoord[1].stp*2.0)),
           float(texture3D(dpb, gl_TexCoord[0].stp-gl_TexCoord[1].stp)));
  
  r = vec4(float(texture3D(dpb, gl_TexCoord[0].stp)),
           float(texture3D(dpb, gl_TexCoord[0].stp+gl_TexCoord[1].stp)),
           float(texture3D(dpb, gl_TexCoord[0].stp+gl_TexCoord[1].stp*2.0)),
           float(texture3D(dpb, gl_TexCoord[0].stp+gl_TexCoord[1].stp*3.0)));
  
  
  // Don't forget we're working with floats scaled from 0 to 1
  // (We add 16.0/255.0 not 16)
  result = vec4((dot(l*coeffs_l, vec2(1.0, 1.0)) +
		 dot(r*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0))
		 +16.0/255.0)/32.0);
  
  // The division in h.264 motion compensation is integer division. So we need
  // to multiply out from the normalized range and do a floor so rounding errors
  // don't screw us over.
  gl_FragColor = floor(result*255.0)/255.0;
}
