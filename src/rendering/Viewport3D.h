#pragma once

#include "core/Types.h"
#include "core/PlacedPoint.h"
#include "Camera.h"
#include "MeshData.h"
#include "SurfaceGrouper.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <vector>
#include <set>
#include <map>
#include <optional>
#include <functional>

namespace prs {

class Viewport3D : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit Viewport3D(QWidget* parent = nullptr);
    ~Viewport3D() override = default;

    bool loadModel(const QString& filepath);
    bool hasModel() const { return mesh_.triangleCount() > 0; }

    // Scale
    void setScaleFactor(float factor);
    float scaleFactor() const { return scaleFactor_; }
    float originalSize() const { return mesh_.diagonalSize(); }
    Vec3f getRealWorldDimensions() const;
    float getRealWorldSize() const;

    // Point placement
    void setPlacementMode(bool enabled);
    bool placementMode() const { return placementMode_; }
    void updateActivePointDistance(float distance);
    void adjustActivePointDistance(float delta);
    float getActivePointDistance() const;
    void setActivePointType(const std::string& type);
    std::string getActivePointType() const;
    void removeActivePoint();
    void deselectPoint();
    void clearPlacedPoints();
    int  activePointIndex() const { return activePointIndex_; }
    const std::vector<PlacedPoint>& placedPoints() const { return placedPoints_; }

    // Surface selection
    int  selectedSurfaceIndex() const { return selectedSurfaceIndex_; }
    void deselectSurface();
    void setSurfaceColor(int surfIdx, const Color3f& color);
    void toggleSurfaceTexture(int surfIdx);

    // Acoustic simulation helpers
    std::pair<int, int> countSourcesAndListeners() const;
    struct SourceData { Vec3f position; std::string audioFile; std::string name; float volume; };
    struct ListenerData { Vec3f position; std::string name; };
    std::pair<std::vector<SourceData>, std::vector<ListenerData>>
        getSourcesAndListeners(const std::string& audioFile) const;

    struct WallInfo { std::vector<int> triangleIndices; };
    std::vector<WallInfo> getWallsForAcoustic() const;
    Vec3f getScaledRoomCenter() const;
    std::vector<Vec3f> getScaledModelVertices() const;

    const std::vector<std::set<int>>& surfaces() const { return surfaces_; }
    const std::vector<Color3f>& surfaceColors() const { return surfaceColors_; }

    // Mesh data access
    const MeshData& meshData() const { return mesh_; }

signals:
    void modelLoaded(const QString& filepath);
    void pointPlaced(int index);
    void pointSelected(int index);
    void pointDeselected();
    void surfaceSelected(int index);
    void surfaceDeselected();
    void placementModeChanged(bool enabled);
    void scaleChanged(float factor);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void autoNormalizeScale();
    void updateProjection();

    // Drawing helpers
    void drawMeasurementGrid();
    void drawModel();
    void drawPointMarkers();
    void drawSelectedSurfaceOutline();
    void drawPlaceholder();

    // Ray picking
    std::pair<Vec3f, Vec3f> getRayFromMouse(const QPoint& pos);
    struct IntersectionResult { Vec3f point; Vec3f normal; int triIndex; };
    std::optional<IntersectionResult> getIntersectionPoint(const QPoint& pos);
    std::optional<int> getPointAtMouse(const QPoint& pos);
    bool trySelectPointAtMouse(const QPoint& pos);
    bool trySelectSurfaceAtMouse(const QPoint& pos);
    void addPointAtMouse(const QPoint& pos);

    // Data
    MeshData mesh_;
    Camera camera_;
    float scaleFactor_ = 1.0f;
    float originalSize_ = 0.0f;
    Vec3f modelCenter_ = Vec3f::Zero();

    // Surface grouping
    SurfaceGrouper::EdgeSet featureEdges_;
    std::vector<std::set<int>> surfaces_;
    std::vector<Color3f> surfaceColors_;
    std::vector<bool> surfaceTextured_;
    std::map<int, int> triangleToSurface_;

    // Points
    std::vector<PlacedPoint> placedPoints_;
    int activePointIndex_ = -1;
    bool placementMode_ = false;
    int nextPointColorIndex_ = 0;
    static const std::vector<Color3f>& pointColors();

    // Surface selection
    int selectedSurfaceIndex_ = -1;

    // Mouse state
    bool mouseDown_ = false;
    QPoint lastMousePos_;
    QPoint mouseDownPos_;
    bool transparentMode_ = false;

    Color3f defaultSurfaceColor_ = {0.6f, 0.8f, 1.0f};
};

} // namespace prs
