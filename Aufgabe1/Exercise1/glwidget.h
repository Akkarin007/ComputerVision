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
  void drawFrameAxis();
  void drawQuaderAxis(std::vector<std::pair<QVector3D, QColor>>);
  void drawPointCloud();
  void initQuader(std::vector<std::pair<QVector3D, QColor>>&, QVector4D, float);
  void initPerspectiveCameraModel(QVector4D translation);
  
  float _pointSize;
  std::vector<std::pair<QVector3D, QColor> > _axesLines;
  std::vector<std::pair<QVector3D, QColor> > _perspectiveCameraModelAxesLines;

  std::vector<std::pair<QVector3D, QColor> > _quaderOne;
  std::vector<std::pair<QVector3D, QColor> > _quaderTwo;


  QPoint _prevMousePosition;
  QOpenGLVertexArrayObject _vao;
  QOpenGLBuffer _vertexBuffer;
  QScopedPointer<QOpenGLShaderProgram> _shaders;

  QMatrix4x4 _projectionMatrix;
  QMatrix4x4 _cameraMatrix;
  QMatrix4x4 _worldMatrix;

  PointCloud pointcloud;

  QSharedPointer<Camera> _currentCamera;
};
