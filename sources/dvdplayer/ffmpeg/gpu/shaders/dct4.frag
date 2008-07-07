//Basic Fragment shader
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_draw_buffers : enable
uniform sampler2DRect dctelems;
                             
void main(void)                                                                 
{                              
  mat4 right = mat4(1, 1, 1, 0.5, 1, 0.5, -1, -1, 1, -0.5, -1, 1,1, -1, 1, -0.5);
  mat4 left = mat4(1, 1, 1, 1,  1, 0.5, -0.5, -1, 1, -1, -1, 1, 0.5, -1, 1, -0.5); 
  mat4 result;
  mat4 coeffs = mat4(texture2DRect(dctelems, gl_TexCoord[0].xy),
       	      	     texture2DRect(dctelems, gl_TexCoord[0].xy-vec2(0,1.0)),
        	     texture2DRect(dctelems, gl_TexCoord[0].xy-vec2(0,2.0)),
       	     	     texture2DRect(dctelems, gl_TexCoord[0].xy-vec2(0,3.0)));
  coeffs[0].r += 32;
  result = (left*coeffs*right)/64.0;
  gl_FragData[0] = floor(result[0]);
  gl_FragData[1] = floor(result[1]);
  gl_FragData[2] = floor(result[2]);
  gl_FragData[3] = floor(result[3]);
}                                                                               
