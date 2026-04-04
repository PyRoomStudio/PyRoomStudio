#pragma once

#include "RenderOptions.h"
#include "core/ProjectFile.h"
#include "acoustics/SimulationWorker.h"

#include <QString>

#include <vector>

namespace prs::RenderPipeline {

bool buildSimulationParams(const QString& projectPath, const ProjectData& project, const RenderOptions& options,
                           const std::vector<int>& selectedListenerIndices, const QString& outputDir,
                           SimulationWorker::Params* params, QString* error);

} // namespace prs::RenderPipeline
