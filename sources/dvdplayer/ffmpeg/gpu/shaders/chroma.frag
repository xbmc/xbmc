/**
   Fragment shader for chroma motion compensation. This is basically a ghetto
   version of bilinear
*/

uniform sampler3D dpb;

void main(void)
{
  // A = upper left, B = upper right, C = Lower left D = lower right
  float A, B, C, D, result;

  A = texture3D(dpb, gl_TexCoord[0].stp).b;
  B = texture3D(dpb, gl_TexCoord[0].stp + gl_TexCoord[2].stp*vec3(1.0, 0.0, 0.0)).b;
  C = texture3D(dpb, gl_TexCoord[0].stp + gl_TexCoord[2].stp*vec3(1.0, 1.0, 0.0)).b;
  D = texture3D(dpb, gl_TexCoord[0].stp + gl_TexCoord[2].stp*vec3(0.0, 1.0, 0.0)).b;

  result = ((8.0-gl_TexCoord[1].x)*(8.0-gl_TexCoord[1].y)*A +
		  (    gl_TexCoord[1].x)*(8.0-gl_TexCoord[1].y)*B +
		  (8.0-gl_TexCoord[1].x)*(    gl_TexCoord[1].y)*C +
		  (    gl_TexCoord[1].x)*(    gl_TexCoord[1].y)*D +
 	    32.0/255.0)/64.0;

  // The division in h.264 motion compensation is integer division. So we need
  // to multiply out from the normalized range and do a floor so rounding errors
  // don't screw us over.
  gl_FragColor = floor(result*255.0)/255.0;
    
    
}
