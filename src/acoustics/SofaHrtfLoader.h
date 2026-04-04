#pragma once

#include "HrtfDataset.h"

namespace prs {

std::unique_ptr<HrtfDataset> loadSofaHrtfDataset(const QString& path, int sampleRate, QString* error);

} // namespace prs
