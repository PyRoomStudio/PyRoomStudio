#pragma once

#include "core/Types.h"
#include "core/PlacedPoint.h"
#include "core/Material.h"
#include "Camera.h"
#include "MeshData.h"
#include "SurfaceGrouper.h"
#include "TextureManager.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>

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
    void restorePlacedPoints(const std::vector<PlacedPoint>& points);
    int  activePointIndex() const { return activePointIndex_; }
    const std::vector<PlacedPoint>& placedPoints() const { return placedPoints_; }
    std::vector<PlacedPoint>& placedPoints() { return placedPoints_; }

    // Measure mode
    void setMeasureMode(bool enabled);
    bool measureMode() const { return measureMode_; }

    // Surface selection
    int  selectedSurfaceIndex() const { return selectedSurfaceIndex_; }
    void selectSurface(int index);
    void deselectSurface();
    void setSurfaceColor(int surfIdx, const Color3f& color);
    void assignMaterial(int surfIdx, const Material& material);
    std::optional<Material> getSurfaceMaterial(int surfIdx) const;
    const std::vector<std::optional<Material>>& surfaceMaterials() const { return surfaceMaterials_; }
    void toggleSurfaceTexture(int surfIdx);
    bool loadTexture(const QString& filepath);
    bool hasTexture() const { return textureId_ != 0; }

    // Acoustic simulation helpers
    std::pair<int, int> countSourcesAndListeners() const;
    struct SourceData { Vec3f position; std::string audioFile; std::string name; float volume; };
    struct ListenerData { Vec3f position; std::string name; Vec3f orientation; };
    std::pair<std::vector<SourceData>, std::vector<ListenerData>>
        getSourcesAndListeners(const std::string& audioFile) const;

    struct WallInfo {
        std::vector<int> triangleIndices;
        std::array<float, NUM_FREQ_BANDS> absorption = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
        float scattering = DEFAULT_SCATTERING;
    };
    std::vector<WallInfo> getWallsForAcoustic() const;
    Vec3f getScaledRoomCenter() const;
    std::vector<Vec3f> getScaledModelVertices() const;

    const std::vector<std::set<int>>& surfaces() const { return surfaces_; }
    const std::vector<Color3f>& surfaceColors() const { return surfaceColors_; }

    // Display settings
    void applyDisplaySettings();
    bool gridVisible() const { return gridVisible_; }
    float gridSpacing() const { return gridSpacing_; }
    float transparencyAlpha() const { return transparencyAlpha_; }
    int markerSize() const { return markerSize_; }

    // Select all / multi-selection
    void selectAllPoints();
    void clearPointSelection();
    const std::set<int>& selectedPointIndices() const { return selectedPointIndices_; }

    // Move mode
    void setMoveMode(bool enabled);
    bool moveMode() const { return moveMode_; }

    // Mesh data access
    const MeshData& meshData() const { return mesh_; }

signals:
    void modelLoaded(const QString& filepath);
    void meshOpenWarning(int boundaryEdges);
    void pointPlaced(int index);
    void pointSelected(int index);
    void pointDeselected();
    void surfaceSelected(int index);
    void surfaceDeselected();
    void surfaceMaterialChanged(int index, const QString& materialName);
    void placementModeChanged(bool enabled);
    void scaleChanged(float factor);
    void measurementResult(float distance);
    void moveFinished(int pointIndex, const PlacedPoint& oldState, const PlacedPoint& newState);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

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
    std::vector<std::optional<Material>> surfaceMaterials_;
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

    // Multi-selection
    std::set<int> selectedPointIndices_;

    // Move mode
    bool moveMode_ = false;
    int movingPointIndex_ = -1;
    PlacedPoint moveOriginal_;

    // Mouse state
    bool mouseDown_ = false;
    QPoint lastMousePos_;
    QPoint mouseDownPos_;
    bool transparentMode_ = false;
    bool measureMode_ = false;
    std::optional<Vec3f> measurePoint1_;
    std::optional<Vec3f> measurePoint2_;

    // Display settings (read from QSettings)
    bool gridVisible_ = true;
    float gridSpacing_ = 1.0f;
    float transparencyAlpha_ = 0.55f;
    int markerSize_ = 15;

    Color3f defaultSurfaceColor_ = {0.6f, 0.8f, 1.0f};
    unsigned int textureId_ = 0;
    QImage textureImage_;
    TextureManager textureManager_;
    std::vector<GLuint> surfaceTextureIds_;
};

} // namespace prs
