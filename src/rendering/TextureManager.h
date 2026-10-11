#pragma once

#include "GLHeaders.h"
#include <QString>
#include <map>

namespace prs {

class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    GLuint getOrLoad(const QString& path);
    void releaseAll();

private:
    std::map<QString, GLuint> cache_;
};

} // namespace prs
