#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <vector>

namespace prs::RenderExports {

bool saveMonoWav(const QString& path, int sampleRate, const std::vector<float>& samples);
bool saveStereoWav(const QString& path, int sampleRate, const std::vector<float>& left,
                   const std::vector<float>& right);

QJsonObject buildMetricsSummary(const QJsonArray& pairs, int sampleRate);
bool saveJsonObject(const QString& path, const QJsonObject& obj);
bool saveMetricsCsv(const QString& path, const QJsonArray& pairs);

} // namespace prs::RenderExports
