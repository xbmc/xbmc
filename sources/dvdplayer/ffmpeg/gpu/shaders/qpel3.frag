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
  vec2 horiz_l, vert_l;
  vec4 horiz_r, vert_r;
  vec3 mv;
  float halfpel1, halfpel2, result;

  // FIXME: is it a +?
  mv = gl_TexCoord[1].stp-0.5*gl_TexCoord[2].xyz;


  horiz_l = vec2(texture3D(dpb, gl_TexCoord[0].stp+
			   vec3(-2.0*gl_TexCoord[2].x, mv.y, 0.0)).b,
                 texture3D(dpb, gl_TexCoord[0].stp+
			   vec3(-1.0*gl_TexCoord[2].x, mv.y, 0.0)).b);
  
  horiz_r = vec4(texture3D(dpb, gl_TexCoord[0].stp+
			   vec3( 0.0*gl_TexCoord[2].x, mv.y, 0.0)).b,
                 texture3D(dpb, gl_TexCoord[0].stp+
			   vec3( 1.0*gl_TexCoord[2].x, mv.y, 0.0)).b,
                 texture3D(dpb, gl_TexCoord[0].stp+
			   vec3( 2.0*gl_TexCoord[2].x, mv.y, 0.0)).b,
                 texture3D(dpb, gl_TexCoord[0].stp+
			   vec3( 3.0*gl_TexCoord[2].x, mv.y, 0.0)).b);


  vert_l = vec2(texture3D(dpb, gl_TexCoord[0].stp+
			  vec3(mv.x, -2.0*gl_TexCoord[2].y, 0.0)).b,
                texture3D(dpb, gl_TexCoord[0].stp+
			  vec3(mv.x, -1.0*gl_TexCoord[2].y, 0.0)).b);
  
  vert_r = vec4(texture3D(dpb, gl_TexCoord[0].stp+
			  vec3(mv.x,  0.0*gl_TexCoord[2].y, 0.0)).b,
                texture3D(dpb, gl_TexCoord[0].stp+
			  vec3(mv.x,  1.0*gl_TexCoord[2].y, 0.0)).b,
                texture3D(dpb, gl_TexCoord[0].stp+
			  vec3(mv.x,  2.0*gl_TexCoord[2].y, 0.0)).b,
                texture3D(dpb, gl_TexCoord[0].stp+
			  vec3(mv.x,  3.0*gl_TexCoord[2].y, 0.0)).b);

  // Don't forget we're working with floats scaled from 0 to 1
  // (We add 16.0/255.0 not 16)
  halfpel1 = (dot(horiz_l*coeffs_l, vec2(1.0, 1.0)) +
	      dot(horiz_r*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0))
	      +16.0/255.0)/32.0;

  halfpel2 = (dot(vert_l*coeffs_l, vec2(1.0, 1.0)) +
	      dot(vert_r*coeffs_r, vec4(1.0, 1.0, 1.0, 1.0))
	      +16.0/255.0)/32.0;

  halfpel1    = floor(halfpel1*255.0)/255.0;
  halfpel2    = floor(halfpel2*255.0)/255.0;
  result     = (halfpel1 + halfpel2 + 1.0/255.0)/2.0;
  
  gl_FragColor = vec4(floor(result*255.0)/255.0);
}
