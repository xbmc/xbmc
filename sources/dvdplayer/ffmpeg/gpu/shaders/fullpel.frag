uniform sampler3D dpb;

void main(void)
{
  gl_FragColor = texture3D(dpb, gl_TexCoord[0].stp);
}
