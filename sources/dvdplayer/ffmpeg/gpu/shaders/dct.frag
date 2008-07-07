//Basic Fragment shader
#extension GL_ARB_texture_rectangle : enable
                                                                                
uniform sampler2DRect dctelems;
                                                                                
void main(void)                                                                 
{                                       
   gl_FragColor = texture2DRect(dctelems, gl_TexCoord[0].xy)               ;
// gl_FragColor = vec4(0.0, 0.5, 0.0, 0.0);                                    
}                                                                               
