#include "UndoCommands.h"
#include "rendering/Viewport3D.h"

namespace prs {

AddPointCommand::AddPointCommand(Viewport3D* vp, const PlacedPoint& point)
    : QUndoCommand("Add Point"), vp_(vp), point_(point) {}

void AddPointCommand::undo() {
    auto& pts = vp_->placedPoints();
    if (!pts.empty()) {
        pts.pop_back();
        vp_->deselectPoint();
        vp_->notifyPlacedPointsChanged();
        vp_->update();
    }
}

void AddPointCommand::redo() {
    vp_->placedPoints().push_back(point_);
    index_ = static_cast<int>(vp_->placedPoints().size()) - 1;
    vp_->notifyPlacedPointsChanged();
    vp_->update();
}

RemovePointCommand::RemovePointCommand(Viewport3D* vp, int index)
    : QUndoCommand("Remove Point"), vp_(vp), index_(index) {
    if (index >= 0 && index < static_cast<int>(vp->placedPoints().size()))
        point_ = vp->placedPoints()[index];
}

void RemovePointCommand::undo() {
    auto& pts = vp_->placedPoints();
    if (index_ >= 0 && index_ <= static_cast<int>(pts.size()))
        pts.insert(pts.begin() + index_, point_);
    vp_->notifyPlacedPointsChanged();
    vp_->update();
}

void RemovePointCommand::redo() {
    auto& pts = vp_->placedPoints();
    if (index_ >= 0 && index_ < static_cast<int>(pts.size()))
        pts.erase(pts.begin() + index_);
    vp_->deselectPoint();
    vp_->notifyPlacedPointsChanged();
    vp_->update();
}

ChangePointTypeCommand::ChangePointTypeCommand(Viewport3D* vp, int index, const std::string& newType)
    : QUndoCommand("Change Point Type"), vp_(vp), index_(index), newType_(newType) {
    if (index >= 0 && index < static_cast<int>(vp->placedPoints().size()))
        oldType_ = vp->placedPoints()[index].pointType;
}

void ChangePointTypeCommand::undo() {
    auto& pts = vp_->placedPoints();
    if (index_ >= 0 && index_ < static_cast<int>(pts.size()))
        pts[index_].pointType = oldType_;
    vp_->notifyPlacedPointsChanged();
    vp_->update();
}

void ChangePointTypeCommand::redo() {
    auto& pts = vp_->placedPoints();
    if (index_ >= 0 && index_ < static_cast<int>(pts.size()))
        pts[index_].pointType = newType_;
    vp_->notifyPlacedPointsChanged();
    vp_->update();
}

ChangeSurfaceColorCommand::ChangeSurfaceColorCommand(Viewport3D* vp, int surfIdx, const Color3f& newColor)
    : QUndoCommand("Change Surface Color"), vp_(vp), surfIdx_(surfIdx), newColor_(newColor) {
    if (surfIdx >= 0 && surfIdx < static_cast<int>(vp->surfaceColors().size()))
        oldColor_ = vp->surfaceColors()[surfIdx];
}

void ChangeSurfaceColorCommand::undo() {
    vp_->setSurfaceColor(surfIdx_, oldColor_);
}

void ChangeSurfaceColorCommand::redo() {
    vp_->setSurfaceColor(surfIdx_, newColor_);
}

ChangeScaleCommand::ChangeScaleCommand(Viewport3D* vp, float oldScale, float newScale)
    : QUndoCommand("Change Scale"), vp_(vp), oldScale_(oldScale), newScale_(newScale) {}

void ChangeScaleCommand::undo() {
    vp_->setScaleFactor(oldScale_);
}

void ChangeScaleCommand::redo() {
    vp_->setScaleFactor(newScale_);
}

MovePointCommand::MovePointCommand(Viewport3D* vp, int index, const PlacedPoint& oldState, const PlacedPoint& newState)
    : QUndoCommand("Move Point"), vp_(vp), index_(index), oldState_(oldState), newState_(newState) {}

void MovePointCommand::undo() {
    auto& pts = vp_->placedPoints();
    if (index_ >= 0 && index_ < static_cast<int>(pts.size())) {
        pts[index_].surfacePoint = oldState_.surfacePoint;
        pts[index_].normal = oldState_.normal;
    }
    vp_->update();
}

void MovePointCommand::redo() {
    auto& pts = vp_->placedPoints();
    if (index_ >= 0 && index_ < static_cast<int>(pts.size())) {
        pts[index_].surfacePoint = newState_.surfacePoint;
        pts[index_].normal = newState_.normal;
    }
    vp_->update();
}

ClearAllPointsCommand::ClearAllPointsCommand(Viewport3D* vp)
    : QUndoCommand("Clear All Points"), vp_(vp) {
    savedPoints_ = vp->placedPoints();
}

void ClearAllPointsCommand::undo() {
    vp_->restorePlacedPoints(savedPoints_);
}

void ClearAllPointsCommand::redo() {
    vp_->clearPlacedPoints();
}

} // namespace prs
