//Basic Fragment shader
#extension GL_ARB_texture_rectangle : enable                                    
                                                                                
//uniform sampler2DRect texture;
                                                                                
void main(void)                                                                 
{                                       
   //gl_FragColor = texture2DRect(texture, gl_TexCoord[0].st);               
      gl_FragColor = vec4(1.0, 1.5, 3.0, 4.0);                                    
//      gl_FragColor = 100;
}                                                                               
