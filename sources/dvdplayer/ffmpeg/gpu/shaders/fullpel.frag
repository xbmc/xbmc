uniform sampler3D dpb;

void main(void)
{
  gl_FragColor = texture3D(dpb, vec3(gl_TexCoord[0].x,
                                     gl_TexCoord[0].y,
                                     gl_TexCoord[0].z));
}
