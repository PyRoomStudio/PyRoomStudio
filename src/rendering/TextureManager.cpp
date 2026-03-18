#include "TextureManager.h"
#include <QImage>

namespace prs {

TextureManager::~TextureManager() {
    releaseAll();
}

GLuint TextureManager::getOrLoad(const QString& path) {
    if (path.isEmpty()) return 0;

    auto it = cache_.find(path);
    if (it != cache_.end())
        return it->second;

    QImage img(path);
    if (img.isNull()) return 0;

    QImage glImg = img.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 glImg.width(), glImg.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, glImg.constBits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    cache_[path] = texId;
    return texId;
}

void TextureManager::release(const QString& path) {
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        glDeleteTextures(1, &it->second);
        cache_.erase(it);
    }
}

void TextureManager::releaseAll() {
    for (auto& [path, texId] : cache_)
        glDeleteTextures(1, &texId);
    cache_.clear();
}

bool TextureManager::has(const QString& path) const {
    return cache_.find(path) != cache_.end();
}

} // namespace prs
