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
#include "tree.h"


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
    void radioButton3Clicked();
    void disable_rays();
    void disable_cubes();
    void disable_projection();
    void disable_image_plane();
    void disable_camera_1();
    void disable_camera_2();
    void disable_reconstruction();
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
  void drawKDTreeLines(std::vector<std::pair<QVector3D, QColor>>);

  void drawPointCloud();
  void initQuader(std::vector<std::pair<QVector3D, QColor>>&, QVector4D, float, float, float, float);
  void initPerspectiveCameraModel(std::vector<std::pair<QVector3D, QColor>> &perspectiveCameraModelAxesLines, QVector4D translation, QVector3D rotation);
  void initImagePlane(std::vector<std::pair<QVector3D, QColor>> &imagePlaneLines, std::vector<std::pair<QVector3D, QColor>> &imagePlaneAxes, QVector4D positionInWorld, float size, float focal_length, QVector3D rotation, QVector4D imagePrinciplePoint);
  void initProjection(std::vector<std::pair<QVector3D, QColor>> quader, std::vector<std::pair<QVector3D, QColor>> &projectionQuader, int focalLength, QVector4D projectionCenter, QVector4D imagePrinciplePoint, QVector3D camera_rotation);
  void initStereoVisionNormalCaseReconstruction(std::vector<std::pair<QVector3D, QColor>> projection1, std::vector<std::pair<QVector3D, QColor>> projection2, std::vector<std::pair<QVector3D, QColor>> &reconstruction, float focalLength, QVector3D camera_1_pos, QVector3D camera_2_pos);
  QVector4D calculateImagePrinciplePoint(float focalLength, QVector4D positionCamera, QVector3D cameraRotation);
  QVector4D calculate_image_plane_equation(QVector3D imagePrinciplePoint, QVector3D rotation);
  
  Tree root;
  float _pointSize;
  std::vector<std::pair<QVector3D, QColor> > _axesLines;

  std::vector<QVector3D> x_array;
  std::vector<QVector3D> y_array;
  std::vector<QVector3D> z_array;

  QMatrix4x4 rotation_x(float);
  QMatrix4x4 rotation_y(float);
  QMatrix4x4 rotation_z(float);

  QPoint _prevMousePosition;
  QOpenGLVertexArrayObject _vao;
  QOpenGLBuffer _vertexBuffer;
  QScopedPointer<QOpenGLShaderProgram> _shaders;

  void aufgabe_1();
  void aufgabe_2();
  void aufgabe_3();
  void constructBalanced3DTree(int left, int right, Tree * node, int d, int maxLvl);
  void partitionField(std::vector<QVector3D> test, int left, int right, QVector3D medianVec, int m, std::string dir);
  std::vector<std::pair<QVector3D, QColor> > _kdTreeLines;

  bool _show_aufgabe_1 = false;
  bool _show_aufgabe_2 = true;
  bool _show_aufgabe_3 = false;
  bool _disable_rays = false;
  bool _disable_cubes = false;
  bool _disable_projection = false;
  bool _disable_image_plane = false;
  bool _disable_camera1 = false;
  bool _disable_camera2 = false;
  bool _disable_reconstruction = false;

  QMatrix4x4 _projectionMatrix;
  QMatrix4x4 _cameraMatrix;
  QMatrix4x4 _worldMatrix;

  QVector3D _rotation_camera_1 = QVector3D(0, 0, 0);
  QVector3D _rotation_camera_2 = QVector3D(0, 2, 0);

  PointCloud pointcloud;

  QSharedPointer<Camera> _currentCamera;
  QVector3D centralProjection(float focalLength, QVector3D vertex, QVector3D projectionCenter, QVector3D imagePrinciplePoint, QVector3D rotation, QVector4D image_plane);
  QVector3D stereoVisionNormalCaseReconstruction(float focalLength, float b, QVector3D vertex1, QVector3D vertex2);
  void initProjectionLines(std::vector<std::pair<QVector3D, QColor>> quader, std::vector<std::pair<QVector3D, QColor>> &projectionLines, QVector4D projectionCenter);
};
