
// (c) Nico Brügel, 2021

#include "glwidget.h"
#include <QtGui>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <gl/GL.h>
#include <gl/GLU.h>
#endif

#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>

#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <utility>

#include "mainwindow.h"

//static const size_t POINT_STRIDE = 4; // x, y, z, index

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent),
    _pointSize(1)
{
    setMouseTracking(true);
    // axes cross
    _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(1.0, 0.0, 0.0)));
    _axesLines.push_back(std::make_pair(QVector3D(1.0, 0.0, 0.0), QColor(1.0, 0.0, 0.0)));
    _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(0.0, 1.0, 0.0)));
    _axesLines.push_back(std::make_pair(QVector3D(0.0, 1.0, 0.0), QColor(0.0, 1.0, 0.0)));
    _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(0.0, 0.0, 1.0)));
    _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 1.0), QColor(0.0, 0.0, 1.0)));


}

GLWidget::~GLWidget()
{
    this->cleanup();
}

void GLWidget::cleanup()
{
  makeCurrent();
 // _vertexBuffer.destroy();
  _shaders.reset();
  doneCurrent();
}

void GLWidget::initializeGL()
{
  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

  initializeOpenGLFunctions();
  glClearColor(0, 0, 0, 1.0);

  // the world is still for now
  _worldMatrix.setToIdentity();

  // create shaders and map attributes
  initShaders();

  // create array container and load points into buffer
  createContainers();
}

void GLWidget::paintGL()
{
    // ensure GL flags
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); //required for gl_PointSize

    // create shaders and map attributes
    initShaders();

    // create array container and load points into buffer
    const QVector<float>& pointsData =pointcloud.getData();
    if(!_vao.isCreated()) _vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&_vao);
    if(!_vertexBuffer.isCreated()) _vertexBuffer.create();
    _vertexBuffer.bind();
    _vertexBuffer.allocate(pointsData.constData(), pointsData.size() * sizeof(GLfloat));
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat) + sizeof(GLfloat), nullptr);
    f->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat) + sizeof(GLfloat), reinterpret_cast<void *>(3*sizeof(GLfloat)));
    _vertexBuffer.release();

    //
    // set camera
    //
    const CameraState camera = _currentCamera->state();
    // position and angles
    _cameraMatrix.setToIdentity();
    _cameraMatrix.translate(camera.position.x(), camera.position.y(), camera.position.z());
    _cameraMatrix.rotate(camera.rotation.x(), 1, 0, 0);
    _cameraMatrix.rotate(camera.rotation.y(), 0, 1, 0);
    _cameraMatrix.rotate(camera.rotation.z(), 0, 0, 1);

    // set clipping planes
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    const double rearClippingPlane[] = {0., 0., -1., camera.rearClippingDistance};
    glClipPlane(GL_CLIP_PLANE1 , rearClippingPlane);
    const double frontClippingPlane[] = {0., 0., 1., camera.frontClippingDistance};
    glClipPlane(GL_CLIP_PLANE2 , frontClippingPlane);

    //
    // draw points cloud
    //
    drawPointCloud();

    initQuader(_quaderOne, QVector4D(0.0, 0.0, 5.0, 1.0), 0.80);
    initQuader(_quaderTwo, QVector4D(1.0, 1.0, 4.0, 1.0), 1.20);
    initPerspectiveCameraModel(QVector4D(1.0, 1.0, 1.0, 1.0));
    drawFrameAxis();
    drawQuaderAxis(_quaderOne);
    drawQuaderAxis(_quaderTwo);
    drawQuaderAxis(_perspectiveCameraModelAxesLines);

    // Assignement 1, Part 1
    // Draw here your objects as in drawFrameAxis();

    // Assignement 1, Part 2
    // Draw here your perspective camera model

    // Assignement 1, Part 3
    // Draw here the perspective projection
}

void GLWidget::initQuader(std::vector<std::pair<QVector3D, QColor>> &quader, QVector4D translation, float size)
{
    QMatrix4x4 translateMatrix;
    translateMatrix.setToIdentity();
    translateMatrix.setColumn(3, translation);

    QVector3D a1 = QVector3D(0.0, 0.0, 0.0);
    QVector3D a2 = QVector3D(1.0, 0.0, 0.0);
    QVector3D a3 = QVector3D(1.0, 1.0, 0.0);
    QVector3D a4 = QVector3D(0.0, 1.0, 0.0);
    a1 = translateMatrix * a1 * size;
    a2 = translateMatrix * a2 * size;
    a3 = translateMatrix * a3 * size;
    a4 = translateMatrix * a4 * size;

    QVector3D b1 = QVector3D(0.0, 0.0, 1.0);
    QVector3D b2 = QVector3D(1.0, 0.0, 1.0);
    QVector3D b3 = QVector3D(1.0, 1.0, 1.0);
    QVector3D b4 = QVector3D(0.0, 1.0, 1.0);
    b1 = translateMatrix * b1 * size;
    b2 = translateMatrix * b2 * size;
    b3 = translateMatrix * b3 * size;
    b4 = translateMatrix * b4 * size;

    quader.push_back(std::make_pair(a1, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(a2, QColor(0.0, 1.0, 0.0)));

    quader.push_back(std::make_pair(a2, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(a3, QColor(0.0, 1.0, 0.0)));


    quader.push_back(std::make_pair(a3, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(a4, QColor(0.0, 1.0, 0.0)));

    quader.push_back(std::make_pair(a4, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(a1, QColor(0.0, 1.0, 0.0)));

//-----------------------

    quader.push_back(std::make_pair(b1, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b2, QColor(0.0, 1.0, 0.0)));

    quader.push_back(std::make_pair(b2, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b3, QColor(0.0, 1.0, 0.0)));


    quader.push_back(std::make_pair(b3, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b4, QColor(0.0, 1.0, 0.0)));

    quader.push_back(std::make_pair(b4, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b1, QColor(0.0, 1.0, 0.0)));

//-----------------------

    quader.push_back(std::make_pair(a1, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b1, QColor(0.0, 1.0, 0.0)));

    quader.push_back(std::make_pair(a2, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b2, QColor(0.0, 1.0, 0.0)));

    quader.push_back(std::make_pair(a3, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b3, QColor(0.0, 1.0, 0.0)));

    quader.push_back(std::make_pair(a4, QColor(0.0, 1.0, 0.0)));
    quader.push_back(std::make_pair(b4, QColor(0.0, 1.0, 0.0)));
}

void GLWidget::initPerspectiveCameraModel(QVector4D translation)
{
    QMatrix4x4 translateMatrix;
    translateMatrix.setToIdentity();
    translateMatrix.setColumn(3, translation);

    QVector3D center = QVector3D(0.0, 0.0, 0.0);
    QVector3D x = QVector3D(1.0, 0.0, 0.0);
    QVector3D y = QVector3D(0.0, 1.0, 0.0);
    QVector3D z = QVector3D(0.0, 0.0, 1.0);

    center = translateMatrix * center;
    x = translateMatrix * x;
    y = translateMatrix * y;
    z = translateMatrix * z;

    _perspectiveCameraModelAxesLines.push_back(std::make_pair(center, QColor(1.0, 0.0, 0.0)));
    _perspectiveCameraModelAxesLines.push_back(std::make_pair(x, QColor(1.0, 0.0, 0.0)));
    _perspectiveCameraModelAxesLines.push_back(std::make_pair(center, QColor(1.0, 0.0, 0.0)));
    _perspectiveCameraModelAxesLines.push_back(std::make_pair(y, QColor(1.0, 0.0, 0.0)));
    _perspectiveCameraModelAxesLines.push_back(std::make_pair(center, QColor(1.0, 0.0, 0.0)));
    _perspectiveCameraModelAxesLines.push_back(std::make_pair(z, QColor(1.0, 0.0, 0.0)));


}

void GLWidget::drawQuaderAxis(std::vector<std::pair<QVector3D, QColor>> quader)
{
  glBegin(GL_LINES);
  QMatrix4x4 mvMatrix = _cameraMatrix * _worldMatrix;
  mvMatrix.scale(0.05f); // make it small
  for (auto vertex : quader) {
    const auto translated = _projectionMatrix * mvMatrix * vertex.first;
    glColor3f(vertex.second.red(), vertex.second.green(), vertex.second.blue());
    glVertex3f(translated.x(), translated.y(), translated.z());
  }
  glEnd();
}


void GLWidget::drawFrameAxis()
{
  glBegin(GL_LINES);
  QMatrix4x4 mvMatrix = _cameraMatrix * _worldMatrix;
  mvMatrix.scale(0.05f); // make it small
  for (auto vertex : _axesLines) {
    const auto translated = _projectionMatrix * mvMatrix * vertex.first;
    glColor3f(vertex.second.red(), vertex.second.green(), vertex.second.blue());
    glVertex3f(translated.x(), translated.y(), translated.z());
  }
  glEnd();
}


void GLWidget::resizeGL(int w, int h)
{
  _projectionMatrix.setToIdentity();
  _projectionMatrix.perspective(70.0f, GLfloat(w) / h, 0.01f, 100.0f);
}

void GLWidget::wheelEvent(QWheelEvent* event)
{
  if (event->angleDelta().y() > 0) {
    _currentCamera->forward();
  } else {
    _currentCamera->backward();
  }
}

void GLWidget::keyPressEvent(QKeyEvent * event)
{
    switch ( event->key() )
    {
      case Qt::Key_Escape:
        QApplication::instance()->quit();
        break;

      case Qt::Key_Left:
      case Qt::Key_A:
        _currentCamera->left();
        break;

      case Qt::Key_Right:
      case Qt::Key_D:
        _currentCamera->right();
        break;

      case Qt::Key_Up:
      case Qt::Key_W:
        _currentCamera->forward();
        break;

      case Qt::Key_Down:
      case Qt::Key_S:
        _currentCamera->backward();
        break;

      case Qt::Key_Space:
      case Qt::Key_Q:
        _currentCamera->up();
        break;

      case Qt::Key_C:
      case Qt::Key_Z:
        _currentCamera->down();
        break;

      default:
        QWidget::keyPressEvent(event);
    }
	update();
}


void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
  const int dx = event->x() - _prevMousePosition.x();
  const int dy = event->y() - _prevMousePosition.y();

  _prevMousePosition = event->pos();

  if (event->buttons() & Qt::LeftButton)
  {
      _currentCamera->rotate(dy, dx, 0);
  }
  else if ( event->buttons() & Qt::RightButton)
  {
      if (dx < 0) {
        _currentCamera->right();
      }
      if (dx > 0) {
        _currentCamera->left();
      }
      if (dy < 0) {
        _currentCamera->down();
      }
      if (dy > 0) {
        _currentCamera->up();
      }
  }
}

void GLWidget::attachCamera(QSharedPointer<Camera> camera)
{
  if (_currentCamera)
  {
    disconnect(_currentCamera.data(), &Camera::changed, this, &GLWidget::onCameraChanged);
  }
  _currentCamera = camera;
  connect(camera.data(), &Camera::changed, this, &GLWidget::onCameraChanged);
}


void GLWidget::onCameraChanged(const CameraState&)
{
  update();
}

void GLWidget::setPointSize(size_t size)
{
  assert(size > 0);
  _pointSize = static_cast<float>(size);
  update();
}

void GLWidget::openFileDialog()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open PLY file"), "", tr("PLY Files (*.ply)"));
     if (!filePath.isEmpty())
     {
         std::cout << filePath.toStdString() << std::endl;
         pointcloud.loadPLY(filePath);
         update();
     }
}

void GLWidget::radioButton1Clicked()
{
    // TODO: toggle to Jarvis' march
    QMessageBox::warning(this, "Feature" ,"upsi hier fehlt noch was");
    update();
}

void GLWidget::radioButton2Clicked()
{
    // TODO: toggle to Graham's scan
    QMessageBox::warning(this, "Feature" ,"upsi hier fehlt noch was");
    update();
}

void GLWidget::initShaders()
{
    _shaders.reset(new QOpenGLShaderProgram());
    auto vsLoaded = _shaders->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertex_shader.glsl");
    auto fsLoaded = _shaders->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment_shader.glsl");
    assert(vsLoaded && fsLoaded);
    // vector attributes
    _shaders->bindAttributeLocation("vertex", 0);
    _shaders->bindAttributeLocation("pointRowIndex", 1);
    // constants
    _shaders->bind();
    _shaders->setUniformValue("lightPos", QVector3D(0, 0, 50));
    _shaders->setUniformValue("pointsCount", static_cast<GLfloat>(pointcloud.getCount()));
    _shaders->link();
    _shaders->release();

   }

void GLWidget::createContainers()
{
    // create array container and load points into buffer
    const QVector<float>& pointsData =pointcloud.getData();
    if(!_vao.isCreated()) _vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&_vao);
    if(!_vertexBuffer.isCreated()) _vertexBuffer.create();
    _vertexBuffer.bind();
    _vertexBuffer.allocate(pointsData.constData(), pointsData.size() * sizeof(GLfloat));
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat) + sizeof(GLfloat), nullptr);
    f->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat) + sizeof(GLfloat), reinterpret_cast<void *>(3*sizeof(GLfloat)));
    _vertexBuffer.release();
}

void GLWidget::drawPointCloud()
{
    const auto viewMatrix = _projectionMatrix * _cameraMatrix * _worldMatrix;
    _shaders->bind();
    _shaders->setUniformValue("pointsCount", static_cast<GLfloat>(pointcloud.getCount()));
    _shaders->setUniformValue("viewMatrix", viewMatrix);
    _shaders->setUniformValue("pointSize", _pointSize);
    //_shaders->setUniformValue("colorAxisMode", static_cast<GLfloat>(_colorMode));
    _shaders->setUniformValue("colorAxisMode", static_cast<GLfloat>(0));
    _shaders->setUniformValue("pointsBoundMin", pointcloud.getMin());
    _shaders->setUniformValue("pointsBoundMax", pointcloud.getMax());
    glDrawArrays(GL_POINTS, 0, pointcloud.getData().size());
    _shaders->release();
}
