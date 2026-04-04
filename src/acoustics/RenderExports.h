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

// --- Binaural export helpers ---

// Returns the canonical output path for a binaural render of one source-listener pair.
// Layout: <outputDir>/binaural/<listenerName>/<sourceName>.wav
// Both listenerName and sourceName are sanitised (spaces → underscores) internally.
QString binauralListenerOutputPath(const QString& outputDir, const QString& listenerName,
                                   const QString& sourceName);

// Adds HRTF provenance fields to an existing metrics JSON object.
// hrtfDatasetPath may be empty when no external HRTF file was used.
void addHrtfMetadata(QJsonObject& obj, const QString& hrtfDatasetPath);

} // namespace prs::RenderExports
