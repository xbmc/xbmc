//Basic Fragment shader
#extension GL_ARB_texture_rectangle : enable                                    
                                                                                
uniform sampler3D dpb;
                                                                                
void main(void)                                                                 
{                                       
//   gl_FragColor = texture3D(dpb, gl_TexCoord[0].xyz)               
   gl_FragColor = texture3D(dpb, vec3(gl_TexCoord[0].x,
   		  		      gl_TexCoord[0].yz)).r;
// gl_FragColor = vec4(0.0, 0.5, 0.0, 0.0);                                    
}                                                                               
