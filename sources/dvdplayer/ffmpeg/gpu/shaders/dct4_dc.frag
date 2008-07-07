//Basic Fragment shader
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_draw_buffers : enable
uniform sampler2DRect dctelems;
                             
void main(void)                                                                 
{                              
  gl_FragData[0] = floor((texture2DRect(dctelems, gl_TexCoord[0].xy).xxxx + vec4(32.0))/62.0);
  gl_FragData[3] = gl_FragData[2] = gl_FragData[1] = gl_FragData[0];
}                                                                               
