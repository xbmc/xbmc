//Basic vertex shader

#define TEXWIDTH 512.0
#define TEXHEIGHT 2048.0
void main()
{    
     vec4 scale = vec4(TEXWIDTH, TEXHEIGHT, 1, 0);
     gl_TexCoord[0] = gl_Vertex/scale;
     gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
}