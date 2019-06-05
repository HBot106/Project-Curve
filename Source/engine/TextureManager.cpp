#include "TextureManager.h"

using namespace std;
using namespace glm;

#define CONTAINS(map, key) map.find(key) != map.end()

TextureManager::TextureManager(string relativePath) {
    this->relativePath = relativePath;
}

shared_ptr<Texture> loadTexture(string relativePath, string file, int textureUnit, GLenum wrapX, GLenum wrapY) {
    shared_ptr<Texture> texture = make_shared<Texture>();
    texture->setFilename(relativePath + file);
    texture->init();
    texture->setUnit(textureUnit);
    texture->setWrapModes(wrapX, wrapY);
    return texture;
}

shared_ptr<Texture> TextureManager::get(string textureFile, int textureUnit, GLenum wrapX, GLenum wrapY)
{
    if (!(CONTAINS(loadedTextures, textureFile))) {
        loadedTextures[textureFile] = loadTexture(relativePath, textureFile, textureUnit, wrapX, wrapY);
    }

    return loadedTextures[textureFile];
}
