#pragma once

#include "core/Types.h"
#include "core/PlacedPoint.h"
#include "core/Material.h"

#include <QUndoCommand>
#include <vector>
#include <optional>
#include <functional>

namespace prs {

class Viewport3D;

class AddPointCommand : public QUndoCommand {
public:
    AddPointCommand(Viewport3D* vp, const PlacedPoint& point);
    void undo() override;
    void redo() override;
private:
    Viewport3D* vp_;
    PlacedPoint point_;
    int index_ = -1;
};

class RemovePointCommand : public QUndoCommand {
public:
    RemovePointCommand(Viewport3D* vp, int index);
    void undo() override;
    void redo() override;
private:
    Viewport3D* vp_;
    PlacedPoint point_;
    int index_;
};

class ChangePointTypeCommand : public QUndoCommand {
public:
    ChangePointTypeCommand(Viewport3D* vp, int index, const std::string& newType);
    void undo() override;
    void redo() override;
private:
    Viewport3D* vp_;
    int index_;
    std::string oldType_;
    std::string newType_;
};

class ChangeSurfaceColorCommand : public QUndoCommand {
public:
    ChangeSurfaceColorCommand(Viewport3D* vp, int surfIdx, const Color3f& newColor);
    void undo() override;
    void redo() override;
private:
    Viewport3D* vp_;
    int surfIdx_;
    Color3f oldColor_;
    Color3f newColor_;
};

class ChangeScaleCommand : public QUndoCommand {
public:
    ChangeScaleCommand(Viewport3D* vp, float oldScale, float newScale);
    void undo() override;
    void redo() override;
private:
    Viewport3D* vp_;
    float oldScale_;
    float newScale_;
};

class MovePointCommand : public QUndoCommand {
public:
    MovePointCommand(Viewport3D* vp, int index, const PlacedPoint& oldState, const PlacedPoint& newState);
    void undo() override;
    void redo() override;
private:
    Viewport3D* vp_;
    int index_;
    PlacedPoint oldState_;
    PlacedPoint newState_;
};

class ClearAllPointsCommand : public QUndoCommand {
public:
    ClearAllPointsCommand(Viewport3D* vp);
    void undo() override;
    void redo() override;
private:
    Viewport3D* vp_;
    std::vector<PlacedPoint> savedPoints_;
};

} // namespace prs
