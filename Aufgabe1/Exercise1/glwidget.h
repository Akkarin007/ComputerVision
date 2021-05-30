//
// Widget für Interaktion und Kontrolle
//
// (c) Georg Umlauf, 2021
//

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QSharedPointer>

#include <vector>

#include "camera.h"
#include "pointcloud.h"


class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GLWidget(QWidget* parent = nullptr);
    ~GLWidget();

public slots:
    // open a PLY file
    void openFileDialog();
    void radioButton1Clicked();
    void radioButton2Clicked();
    void disable_rays();
    void disable_cubes();
    void disable_projection();
    void disable_image_plane();
    void setPointSize(size_t size);
    void attachCamera(QSharedPointer<Camera> camera);

protected:
    void paintGL() Q_DECL_OVERRIDE;
    void initializeGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;

    // navigation
    void keyPressEvent(QKeyEvent   *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent (QMouseEvent *event) Q_DECL_OVERRIDE;


private slots:
  void onCameraChanged(const CameraState& state);

private:
  void initShaders();
  void createContainers();
  void cleanup();
  void drawLines(std::vector<std::pair<QVector3D, QColor>>);

  void drawPointCloud();
  void initQuader(std::vector<std::pair<QVector3D, QColor>>&, QVector4D, float, float, float, float);
  void initPerspectiveCameraModel(std::vector<std::pair<QVector3D, QColor>> &perspectiveCameraModelAxesLines, QVector4D translation, QVector3D rotation);
  void initImagePlane(std::vector<std::pair<QVector3D, QColor>> &imagePlaneLines, std::vector<std::pair<QVector3D, QColor>> &imagePlaneAxes, QVector4D positionInWorld, float size, float focal_length, QVector3D rotation, QVector4D imagePrinciplePoint);
  void initProjection(std::vector<std::pair<QVector3D, QColor>> quader, std::vector<std::pair<QVector3D, QColor>> &projectionQuader, int focalLength, QVector4D projectionCenter, QVector4D imagePrinciplePoint, QVector3D camera_rotation);
  QVector4D calculateImagePrinciplePoint(float focalLength, QVector4D positionCamera, QVector3D cameraRotation);
  QVector4D calculate_image_plane_equation(QVector3D imagePrinciplePoint, QVector3D rotation);
  

  float _pointSize;
  std::vector<std::pair<QVector3D, QColor> > _axesLines;

  QMatrix4x4 rotation_x(float);
  QMatrix4x4 rotation_y(float);
  QMatrix4x4 rotation_z(float);

  QPoint _prevMousePosition;
  QOpenGLVertexArrayObject _vao;
  QOpenGLBuffer _vertexBuffer;
  QScopedPointer<QOpenGLShaderProgram> _shaders;

  void aufgabe_1();
  void aufgabe_2();

  boolean _show_aufgabe_1 = false;
  boolean _show_aufgabe_2 = true;
  boolean _disable_rays_camera1 = false;
  boolean _disable_rays_camera2 = false;
  boolean _disable_cubes = false;
  boolean _disable_projection = false;
  boolean _disable_image_plane = false;
  boolean _disable_camera1 = true;
  boolean _disable_camera2 = false;

  QMatrix4x4 _projectionMatrix;
  QMatrix4x4 _cameraMatrix;
  QMatrix4x4 _worldMatrix;

  PointCloud pointcloud;

  QSharedPointer<Camera> _currentCamera;
  QVector3D centralProjection(float focalLength, QVector3D vertex, QVector3D projectionCenter, QVector3D imagePrinciplePoint, QVector3D rotation, QVector4D image_plane);
  void initProjectionLines(std::vector<std::pair<QVector3D, QColor>> quader, std::vector<std::pair<QVector3D, QColor>> &projectionLines, QVector4D projectionCenter);
};
