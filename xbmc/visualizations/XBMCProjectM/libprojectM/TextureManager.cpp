#include "TextureManager.hpp"
#include "CustomShape.hpp"
#include "Common.hpp"
#include "IdleTextures.hpp"



TextureManager::TextureManager(const std::string _presetURL): presetURL(_presetURL)
{
#ifdef USE_DEVIL
ilInit();
iluInit();
ilutInit();
ilutRenderer(ILUT_OPENGL);	
#endif

 Preload();
}

TextureManager::~TextureManager()
{  
 Clear();
}

void TextureManager::Preload()
{
	
#ifdef USE_DEVIL
	ILuint image;
	ilGenImages(1, &image);
	ilBindImage(image);
	ilLoadL(IL_TYPE_UNKNOWN,(ILvoid*) M_data, M_bytes);
	GLuint tex = ilutGLBindTexImage();
#else
	 GLuint tex = SOIL_load_OGL_texture_from_memory(
					  M_data,
					  M_bytes,
					  SOIL_LOAD_AUTO,
					  SOIL_CREATE_NEW_ID,
				  
					    SOIL_FLAG_POWER_OF_TWO	       
					  |  SOIL_FLAG_MULTIPLY_ALPHA
					  |  SOIL_FLAG_COMPRESS_TO_DXT	  
					  );
#endif
	 
  textures["M.tga"]=tex;
  
#ifdef USE_DEVIL
  ilLoadL(IL_TYPE_UNKNOWN,(ILvoid*) project_data,project_bytes);
  tex = ilutGLBindTexImage();
#else
  tex = SOIL_load_OGL_texture_from_memory(
					  project_data,
					  project_bytes,
					  SOIL_LOAD_AUTO,
					  SOIL_CREATE_NEW_ID,
					  
					  SOIL_FLAG_POWER_OF_TWO	       
					  |  SOIL_FLAG_MULTIPLY_ALPHA
					  |  SOIL_FLAG_COMPRESS_TO_DXT	  
					  );
#endif
  
  textures["project.tga"]=tex;
  
#ifdef USE_DEVIL
  ilLoadL(IL_TYPE_UNKNOWN,(ILvoid*) headphones_data, headphones_bytes);
  tex = ilutGLBindTexImage();
#else
  tex = SOIL_load_OGL_texture_from_memory(
					  headphones_data,
					  headphones_bytes,
					  SOIL_LOAD_AUTO,
					  SOIL_CREATE_NEW_ID,
					  
					  SOIL_FLAG_POWER_OF_TWO	       
					  |  SOIL_FLAG_MULTIPLY_ALPHA
					  |  SOIL_FLAG_COMPRESS_TO_DXT	  
					  );
#endif
  
  textures["headphones.tga"]=tex;
}

void TextureManager::Clear()
{
	

  for(std::map<std::string, GLuint>::const_iterator iter = textures.begin(); iter != textures.end(); iter++)
    {
      glDeleteTextures(1,&iter->second);
    }
  textures.clear();
}


void TextureManager::unloadTextures(const PresetOutputs::cshape_container &shapes)
{
  /*
   for (PresetOutputs::cshape_container::const_iterator pos = shapes.begin();
	pos != shapes.end(); ++pos) 
    {

      if( (*pos)->enabled==1)
	{
	 
	  if ( (*pos)->textured)
	    {
	      std::string imageUrl = (*pos)->getImageUrl();
	      if (imageUrl != "")
		{
		  std::string fullUrl = presetURL + "/" + imageUrl;
		  ReleaseTexture(LoadTexture(fullUrl.c_str()));		 
		}
	    }
	}
    }
  */
}


GLuint TextureManager::getTexture(const std::string imageURL)
{
 
   if (textures.find(imageURL)!= textures.end())
     {
       return textures[imageURL];
     }
   else
     {
       std::string fullURL = presetURL + PATH_SEPARATOR + imageURL;
#ifdef USE_DEVIL
       GLuint tex = ilutGLLoadImage((char *)fullURL.c_str());
#else
       GLuint tex = SOIL_load_OGL_texture(
					  fullURL.c_str(),
					  SOIL_LOAD_AUTO,
					  SOIL_CREATE_NEW_ID,
					  
					    SOIL_FLAG_POWER_OF_TWO
					  //  SOIL_FLAG_MIPMAPS
					  |  SOIL_FLAG_MULTIPLY_ALPHA
					  |  SOIL_FLAG_COMPRESS_TO_DXT
					  //| SOIL_FLAG_DDS_LOAD_DIRECT
					  );
#endif
       textures[imageURL]=tex;
       return tex;
       

     }   
}

unsigned int TextureManager::getTextureMemorySize()
{
  return 0;
}
