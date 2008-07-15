/*
  Motion compensation for half pixel resolution motion vectors.
  This handles the um...something case
*/

uniform sampler3D dpb;

vec2 coeffs_l = vec2(1.0, -5.0);
vec4 coeffs_r = vec4(20.0, 20.0,-5.0, 1.0);

void main(void)
{
  vec2 l;
  vec4 r;
  vec3 fullpel_off = sign(gl_TexCoord[1].xyz)*gl_TexCoord[2].xyz;
  float halfpel;
  float intpel;
  float result;

  l = vec2(texture3D(dpb, gl_TexCoord[0].stp-fullpel_off*2.0).b,
           texture3D(dpb, gl_TexCoord[0].stp-fullpel_off*1.0).b);
  
  r = vec4(texture3D(dpb, gl_TexCoord[0].stp).b,
           texture3D(dpb, gl_TexCoord[0].stp+fullpel_off*1.0).b,
           texture3D(dpb, gl_TexCoord[0].stp+fullpel_off*2.0).b,
           texture3D(dpb, gl_TexCoord[0].stp+fullpel_off*3.0).b);

  // Texturein contains the previous frame
  halfpel = (dot(l*coeffs_l, vec2(1.0, 1.0)) + 
	     dot(r*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0)) + 
	     16.0/255.0)/32.0;
  
  //FIXME: is it + ?
  intpel = texture3D(dpb, gl_TexCoord[0].stp+gl_TexCoord[1].stp-gl_TexCoord[2].stp*0.5).b;

  // The division in h.264 motion compensation is integer division. So we need
  // to multiply out from the normalized range and do a floor so rounding errors
  // don't screw us over.
  halfpel = floor(halfpel*255.0)/255.0;
  intpel  = floor( intpel*255.0)/255.0;
  result  = (intpel + halfpel + 1.0/255.0)/2.0;
  gl_FragColor = vec4(floor(result*255.0)/255.0);
}
