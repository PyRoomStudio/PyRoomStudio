#include "SceneManager.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace prs {

SoundSource& SceneManager::addSoundSource(const Vec3f& position, const std::string& audioFile, float volume,
                                          const std::string& name) {
    SoundSource src;
    src.position = position;
    src.audioFile = audioFile;
    src.volume = volume;
    src.name = name.empty() ? "Source " + std::to_string(nextSourceId_++) : name;
    soundSources_.push_back(std::move(src));
    return soundSources_.back();
}

Listener& SceneManager::addListener(const Vec3f& position, const std::string& name, std::optional<Vec3f> orientation) {
    Listener lst;
    lst.position = position;
    lst.name = name.empty() ? "Listener " + std::to_string(nextListenerId_++) : name;
    lst.orientation = orientation;
    listeners_.push_back(std::move(lst));
    return listeners_.back();
}

bool SceneManager::removeSoundSource(int index) {
    if (index < 0 || index >= static_cast<int>(soundSources_.size()))
        return false;
    soundSources_.erase(soundSources_.begin() + index);
    if (selectedSourceIndex_ == index)
        selectedSourceIndex_ = -1;
    else if (selectedSourceIndex_ > index)
        --selectedSourceIndex_;
    return true;
}

bool SceneManager::removeListener(int index) {
    if (index < 0 || index >= static_cast<int>(listeners_.size()))
        return false;
    listeners_.erase(listeners_.begin() + index);
    if (selectedListenerIndex_ == index)
        selectedListenerIndex_ = -1;
    else if (selectedListenerIndex_ > index)
        --selectedListenerIndex_;
    return true;
}

void SceneManager::clearAll() {
    soundSources_.clear();
    listeners_.clear();
    selectedSourceIndex_ = -1;
    selectedListenerIndex_ = -1;
}

SoundSource* SceneManager::getSoundSource(int index) {
    if (index < 0 || index >= static_cast<int>(soundSources_.size()))
        return nullptr;
    return &soundSources_[index];
}

const SoundSource* SceneManager::getSoundSource(int index) const {
    if (index < 0 || index >= static_cast<int>(soundSources_.size()))
        return nullptr;
    return &soundSources_[index];
}

Listener* SceneManager::getListener(int index) {
    if (index < 0 || index >= static_cast<int>(listeners_.size()))
        return nullptr;
    return &listeners_[index];
}

const Listener* SceneManager::getListener(int index) const {
    if (index < 0 || index >= static_cast<int>(listeners_.size()))
        return nullptr;
    return &listeners_[index];
}

bool SceneManager::hasMinimumObjects() const {
    return !soundSources_.empty() && !listeners_.empty();
}

std::pair<std::vector<Vec3f>, std::vector<Vec3f>> SceneManager::getAllPositions() const {
    std::vector<Vec3f> srcPos, lstPos;
    for (auto& s : soundSources_)
        srcPos.push_back(s.position);
    for (auto& l : listeners_)
        lstPos.push_back(l.position);
    return {srcPos, lstPos};
}

void SceneManager::selectSource(int index) {
    if (index >= 0 && index < static_cast<int>(soundSources_.size())) {
        selectedSourceIndex_ = index;
        selectedListenerIndex_ = -1;
    }
}

void SceneManager::selectListener(int index) {
    if (index >= 0 && index < static_cast<int>(listeners_.size())) {
        selectedListenerIndex_ = index;
        selectedSourceIndex_ = -1;
    }
}

void SceneManager::clearSelection() {
    selectedSourceIndex_ = -1;
    selectedListenerIndex_ = -1;
}

std::optional<std::pair<std::string, int>> SceneManager::getSelectedObject() const {
    if (selectedSourceIndex_ >= 0)
        return std::make_pair(std::string("source"), selectedSourceIndex_);
    if (selectedListenerIndex_ >= 0)
        return std::make_pair(std::string("listener"), selectedListenerIndex_);
    return std::nullopt;
}

bool SceneManager::deleteSelected() {
    if (selectedSourceIndex_ >= 0)
        return removeSoundSource(selectedSourceIndex_);
    if (selectedListenerIndex_ >= 0)
        return removeListener(selectedListenerIndex_);
    return false;
}

static QJsonArray vec3ToJson(const Vec3f& v) {
    return QJsonArray{v.x(), v.y(), v.z()};
}

static Vec3f jsonToVec3(const QJsonArray& a) {
    return Vec3f(a[0].toDouble(), a[1].toDouble(), a[2].toDouble());
}

void SceneManager::saveToFile(const QString& filepath) const {
    QJsonObject root;
    root["version"] = "1.0";
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray srcArr;
    for (auto& s : soundSources_) {
        QJsonObject obj;
        obj["position"] = vec3ToJson(s.position);
        obj["audio_file"] = QString::fromStdString(s.audioFile);
        obj["volume"] = s.volume;
        obj["name"] = QString::fromStdString(s.name);
        obj["marker_color"] = QJsonArray{s.markerColor[0], s.markerColor[1], s.markerColor[2]};
        srcArr.append(obj);
    }
    root["sound_sources"] = srcArr;

    QJsonArray lstArr;
    for (auto& l : listeners_) {
        QJsonObject obj;
        obj["position"] = vec3ToJson(l.position);
        obj["name"] = QString::fromStdString(l.name);
        if (l.orientation)
            obj["orientation"] = vec3ToJson(*l.orientation);
        obj["marker_color"] = QJsonArray{l.markerColor[0], l.markerColor[1], l.markerColor[2]};
        lstArr.append(obj);
    }
    root["listeners"] = lstArr;

    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void SceneManager::loadFromFile(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    auto doc = QJsonDocument::fromJson(file.readAll());
    auto root = doc.object();

    clearAll();

    for (auto val : root["sound_sources"].toArray()) {
        auto obj = val.toObject();
        SoundSource s;
        s.position = jsonToVec3(obj["position"].toArray());
        s.audioFile = obj["audio_file"].toString().toStdString();
        s.volume = static_cast<float>(obj["volume"].toDouble(1.0));
        s.name = obj["name"].toString().toStdString();
        auto mc = obj["marker_color"].toArray();
        if (mc.size() == 3)
            s.markerColor = {mc[0].toInt(), mc[1].toInt(), mc[2].toInt()};
        soundSources_.push_back(std::move(s));
    }

    for (auto val : root["listeners"].toArray()) {
        auto obj = val.toObject();
        Listener l;
        l.position = jsonToVec3(obj["position"].toArray());
        l.name = obj["name"].toString().toStdString();
        if (obj.contains("orientation") && !obj["orientation"].isNull())
            l.orientation = jsonToVec3(obj["orientation"].toArray());
        auto mc = obj["marker_color"].toArray();
        if (mc.size() == 3)
            l.markerColor = {mc[0].toInt(), mc[1].toInt(), mc[2].toInt()};
        listeners_.push_back(std::move(l));
    }
}

QString SceneManager::getSummary() const {
    return QString("Scene: %1 sound source(s), %2 listener(s)").arg(soundSources_.size()).arg(listeners_.size());
}

} // namespace prs
