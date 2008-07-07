//Basic Fragment shader
                                                                                
uniform sampler3D dpb;
#define scaling 4.0/512.0
                                                                                
void main(void)                                                                 
{      
   mat4 ident = mat4(1);                                 
//   gl_FragColor = texture3D(dpb, vec3(287.0/512.0, 86.0/2048.0, 8.0));
     gl_FragColor = texture3D(dpb, vec3(gl_TexCoord[0].x/4.0, gl_TexCoord[0].y, 16.0));
//   gl_FragColor = dot(gl_FragColor, ident[floor(gl_TexCoord[0].x/scaling)]);
//      gl_FragColor = vec4(0.0, 1.0, 0.0, 0.0);              
   //   gl_FragColor.r = texture3D(dpb, vec3(0.0, 0.0, 3.0)).r;
}
