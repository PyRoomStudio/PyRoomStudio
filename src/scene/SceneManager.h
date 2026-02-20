#pragma once

#include "core/Types.h"
#include "core/SoundSource.h"
#include "core/Listener.h"

#include <QString>
#include <vector>
#include <optional>
#include <utility>

namespace prs {

class SceneManager {
public:
    SceneManager() = default;

    SoundSource& addSoundSource(const Vec3f& position, const std::string& audioFile,
                                float volume = 1.0f, const std::string& name = "");
    Listener&    addListener(const Vec3f& position, const std::string& name = "",
                             std::optional<Vec3f> orientation = std::nullopt);

    bool removeSoundSource(int index);
    bool removeListener(int index);
    void clearAll();

    SoundSource*       getSoundSource(int index);
    const SoundSource* getSoundSource(int index) const;
    Listener*          getListener(int index);
    const Listener*    getListener(int index) const;

    int soundSourceCount() const { return static_cast<int>(soundSources_.size()); }
    int listenerCount()    const { return static_cast<int>(listeners_.size()); }
    bool hasMinimumObjects() const;

    std::pair<std::vector<Vec3f>, std::vector<Vec3f>> getAllPositions() const;

    void selectSource(int index);
    void selectListener(int index);
    void clearSelection();
    std::optional<std::pair<std::string, int>> getSelectedObject() const;
    bool deleteSelected();

    void saveToFile(const QString& filepath) const;
    void loadFromFile(const QString& filepath);

    QString getSummary() const;

    const std::vector<SoundSource>& soundSources() const { return soundSources_; }
    const std::vector<Listener>&    listeners()    const { return listeners_; }

    std::vector<SoundSource>& soundSources() { return soundSources_; }
    std::vector<Listener>&    listeners()    { return listeners_; }

private:
    std::vector<SoundSource> soundSources_;
    std::vector<Listener>    listeners_;
    int selectedSourceIndex_   = -1;
    int selectedListenerIndex_ = -1;
    int nextSourceId_   = 1;
    int nextListenerId_ = 1;
};

} // namespace prs
