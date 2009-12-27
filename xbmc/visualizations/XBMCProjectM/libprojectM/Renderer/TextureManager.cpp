#ifdef LINUX
#include <GL/gl.h>
#endif
#ifdef WIN32
#include "glew.h"
#endif
#ifdef __APPLE__
#include <GL/gl.h>
#endif

#ifdef USE_DEVIL
#include <IL/ilut.h>
#else
#include "SOIL/SOIL.h"
#endif

#ifdef WIN32
#include "win32-dirent.h"
#endif

#ifdef LINUX
#include <dirent.h>
#endif

#ifdef MACOS
#include <dirent.h>
#endif
#include "TextureManager.hpp"
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
 loadTextureDir();
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
	 uint tex = SOIL_load_OGL_texture_from_memory(
					  M_data,
					  M_bytes,
					  SOIL_LOAD_AUTO,
					  SOIL_CREATE_NEW_ID,

					    SOIL_FLAG_POWER_OF_TWO
					  |  SOIL_FLAG_MULTIPLY_ALPHA
					 // |  SOIL_FLAG_COMPRESS_TO_DXT
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
					  //|  SOIL_FLAG_COMPRESS_TO_DXT
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
					 // |  SOIL_FLAG_COMPRESS_TO_DXT
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

void TextureManager::setTexture(const std::string name, const unsigned int texId, const int width, const int height)
{
		textures[name] = texId;
		widths[name] = width;
		heights[name] = height;
}

//void TextureManager::unloadTextures(const PresetOutputs::cshape_container &shapes)
//{
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
//}

GLuint TextureManager::getTexture(const std::string filename)
{
	std::string fullURL = presetURL + PATH_SEPARATOR + filename;
	return getTextureFullpath(filename,fullURL);
}

GLuint TextureManager::getTextureFullpath(const std::string filename, const std::string imageURL)
{

   if (textures.find(filename)!= textures.end())
     {
       return textures[filename];
     }
   else
     {

#ifdef USE_DEVIL
       GLuint tex = ilutGLLoadImage((char *)imageURL.c_str());
#else
       int width, height;

       uint tex = SOIL_load_OGL_texture_size(
    		   imageURL.c_str(),
					  SOIL_LOAD_AUTO,
					  SOIL_CREATE_NEW_ID,

					    //SOIL_FLAG_POWER_OF_TWO
					  //  SOIL_FLAG_MIPMAPS
					    SOIL_FLAG_MULTIPLY_ALPHA
					  //|  SOIL_FLAG_COMPRESS_TO_DXT
					  //| SOIL_FLAG_DDS_LOAD_DIRECT
					  ,&width,&height);

#endif
       textures[filename]=tex;
       widths[filename]=width;
       heights[filename]=height;
       return tex;


     }
}

int TextureManager::getTextureWidth(const std::string imageURL)
{
	return widths[imageURL];
}

int TextureManager::getTextureHeight(const std::string imageURL)
{
	return heights[imageURL];
}

unsigned int TextureManager::getTextureMemorySize()
{
  return 0;
}

void TextureManager::loadTextureDir()
{
	std::string dirname = CMAKE_INSTALL_PREFIX "/share/projectM/textures";

	  DIR * m_dir;

	 // Allocate a new a stream given the current directory name
	  if ((m_dir = opendir(dirname.c_str())) == NULL)
	  {
	    std::cout<<"No Textures Loaded from "<<dirname<<std::endl;
	    return; // no files loaded. m_entries is empty
	  }

	  struct dirent * dir_entry;

	  while ((dir_entry = readdir(m_dir)) != NULL)
	  {

	    // Convert char * to friendly string
	    std::string filename(dir_entry->d_name);

	    if (filename.length() > 0 && filename[0] == '.')
		continue;

	    // Create full path name
	    std::string fullname = dirname + PATH_SEPARATOR + filename;

	    unsigned int texId = getTextureFullpath(filename, fullname);
	    if(texId != 0)
	    {
	    	user_textures.push_back(texId);
	    	textures[filename]=texId;
	    	user_texture_names.push_back(filename);
	    }
	  }

	  if (m_dir)
	    {
	      closedir(m_dir);
	      m_dir = 0;
	    }

}

std::string TextureManager::getRandomTextureName(std::string random_id)
{
	if (user_texture_names.size() > 0)
	{
		std::string random_name = user_texture_names[rand() % user_texture_names.size()];
		random_textures.push_back(random_id);
		textures[random_id] = textures[random_name];
		return random_name;
	}
	else return "";
}

void TextureManager::clearRandomTextures()
{
	for (std::vector<std::string>::iterator pos = random_textures.begin(); pos	!= random_textures.end(); ++pos)
				{
					textures.erase(*pos);
					widths.erase(*pos);
					heights.erase(*pos);
				}
	random_textures.clear();

}
