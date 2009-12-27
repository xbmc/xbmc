#ifndef TextureManager_HPP
#define TextureManager_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>

class TextureManager
{
  std::string presetURL;
  std::map<std::string,unsigned int> textures;
  std::map<std::string,unsigned int> heights;
  std::map<std::string,unsigned int> widths;
  std::vector<unsigned int> user_textures;
  std::vector<std::string> user_texture_names;
  std::vector<std::string> random_textures;
public:
  ~TextureManager();
  TextureManager(std::string _presetURL);
  //void unloadTextures(const PresetOutputs::cshape_container &shapes);
  void Clear();
  void Preload();
  unsigned int getTexture(const std::string filenamne);
  unsigned int getTextureFullpath(const std::string filename, const std::string imageUrl);
  unsigned int getTextureMemorySize();
  int getTextureWidth(const std::string imageUrl);
  int getTextureHeight(const std::string imageUrl);
  void setTexture(const std::string name, const unsigned int texId, const int width, const int height);
  void loadTextureDir();
  std::string getRandomTextureName(std::string rand_name);
  void clearRandomTextures();
};

#endif
