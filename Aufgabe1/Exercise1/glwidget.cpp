
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

#define PI 3.14159265
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


QMatrix4x4 GLWidget::rotation_z(float alpha)
{
    //alpha1
    float theta = alpha * PI / 180.0;
    float c = cosf(theta);
    float s = sinf(theta);
    QVector4D v1 = QVector4D(c, -s, 0, 0);
    QVector4D v2 = QVector4D(s, c, 0, 0);
    QVector4D v3 = QVector4D(0, 0, 1, 0);
    QVector4D v4 = QVector4D(0, 0, 0, 1);

    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.setColumn(0, v1);
    matrix.setColumn(1, v2);
    matrix.setColumn(2, v3);
    matrix.setColumn(3, v4);

    return matrix;
}

QMatrix4x4 GLWidget::rotation_x(float alpha)
{
    //alpha1
    float theta = alpha * PI / 180.0;
    float c = cosf(theta);
    float s = sinf(theta);
    QVector4D v1 = QVector4D(1,0,0,0);
    QVector4D v2 = QVector4D(0, c, -s, 0);
    QVector4D v3 = QVector4D(0, s, c, 0);
    QVector4D v4 = QVector4D(0, 0, 0, 1);

    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.setColumn(0, v1);
    matrix.setColumn(1, v2);
    matrix.setColumn(2, v3);
    matrix.setColumn(3, v4);

    return matrix;
}

QMatrix4x4 GLWidget::rotation_y(float alpha)
{
    //alpha1
    float theta = alpha * PI / 180.0;
    float c = cosf(theta);
    float s = sinf(theta);
    QVector4D v1 = QVector4D(c, 0, s, 0);
    QVector4D v2 = QVector4D(0, 1, 0, 0);
    QVector4D v3 = QVector4D(-s, 0, c, 0);
    QVector4D v4 = QVector4D(0, 0, 0, 1);

    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.setColumn(0, v1);
    matrix.setColumn(1, v2);
    matrix.setColumn(2, v3);
    matrix.setColumn(3, v4);

    return matrix;
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


    // draw world cordinate system
    drawLines(_axesLines);

    if (_show_aufgabe_1 == true)
    {
        aufgabe_1();
    }
    if (_show_aufgabe_2 == true)
    {
        aufgabe_2();
    }
}

void GLWidget::aufgabe_1()
{
    // Vector sets:
    std::vector<std::pair<QVector3D, QColor> > quaderOne;
    std::vector<std::pair<QVector3D, QColor> > quaderTwo;
    std::vector<std::pair<QVector3D, QColor> > perspectiveCameraModelAxesLines;
    std::vector<std::pair<QVector3D, QColor> > imagePlaneLines;
    std::vector<std::pair<QVector3D, QColor> > imagePlaneAxes;
    std::vector<std::pair<QVector3D, QColor> > projectionLines;
    std::vector<std::pair<QVector3D, QColor> > quaderOneProjection;
    std::vector<std::pair<QVector3D, QColor> > quaderTwoProjection;

    // Assignement 1, Part 1
    // Draw here your objects as in drawFrameAxis();
    QVector4D positionWorldQuaderOne = QVector4D(0.0, 0.0, 8.0, 1.0);
    QVector4D positionWorldQuaderTwo = QVector4D(1.0, 1.0, 6.0, 1.0);
    if (!_disable_cubes) {
        initQuader(quaderOne, positionWorldQuaderOne, 0.80, 0.0, 30, 0.0);
        initQuader(quaderTwo, positionWorldQuaderTwo, 1.20, 30.0, 0.0, 0.0);

        drawLines(quaderOne);
        drawLines(quaderTwo);
    }

    // Assignement 1, Part 2
    // Draw here your perspective camera model
    QVector4D positionWorldCamera = QVector4D(1.0, 1.0, 1.0, 1.0);
    float focalLength = 2;
    float imagePlaneSize = 1;
    QVector3D cameraRotation = QVector3D(0, 5, 0);
    QVector4D projectionCenter = positionWorldCamera;
    QVector4D imagePrinciplePoint = calculateImagePrinciplePoint(focalLength, positionWorldCamera, cameraRotation);

    initPerspectiveCameraModel(perspectiveCameraModelAxesLines, positionWorldCamera, cameraRotation);
    drawLines(perspectiveCameraModelAxesLines);

    if (!_disable_image_plane) {
        initImagePlane(imagePlaneLines, imagePlaneAxes ,positionWorldCamera, imagePlaneSize, focalLength, cameraRotation, imagePrinciplePoint);
        drawLines(imagePlaneLines);
        drawLines(imagePlaneAxes);
    }

    // Assignement 1, Part 3
    // Draw here the perspective projection
    if (!_disable_projection) {
        initProjection(quaderOne, quaderOneProjection, focalLength, projectionCenter, imagePrinciplePoint, cameraRotation);
        initProjection(quaderTwo, quaderTwoProjection, focalLength, projectionCenter, imagePrinciplePoint, cameraRotation);
        drawLines(quaderOneProjection);
        drawLines(quaderTwoProjection);
    }

    // Draw projection Lines
    if (!_disable_rays_camera1) {
        initProjectionLines(quaderOne, projectionLines, projectionCenter);
        initProjectionLines(quaderTwo, projectionLines, projectionCenter);
        drawLines(projectionLines);
    }
}

void GLWidget::aufgabe_2()
{
    // Vector sets:
    std::vector<std::pair<QVector3D, QColor> > quaderOne;
    std::vector<std::pair<QVector3D, QColor> > quaderTwo;
    std::vector<std::pair<QVector3D, QColor> > perspectiveCameraModelAxesLines;
    std::vector<std::pair<QVector3D, QColor> > imagePlaneLines;
    std::vector<std::pair<QVector3D, QColor> > imagePlaneAxes;
    std::vector<std::pair<QVector3D, QColor> > projectionLines;
    std::vector<std::pair<QVector3D, QColor> > camera_1_quaderOneProjection;
    std::vector<std::pair<QVector3D, QColor> > camera_1_quaderTwoProjection;
    std::vector<std::pair<QVector3D, QColor> > camera_2_quaderOneProjection;
    std::vector<std::pair<QVector3D, QColor> > camera_2_quaderTwoProjection;

    // Draw Cubes
    QVector4D positionWorldQuaderOne = QVector4D(0.0, 0.0, 8.0, 1.0);
    QVector4D positionWorldQuaderTwo = QVector4D(1.0, 1.0, 6.0, 1.0);
    if (!_disable_cubes) {
        initQuader(quaderOne, positionWorldQuaderOne, 0.80, 0.0, 30, 0.0);
        initQuader(quaderTwo, positionWorldQuaderTwo, 1.20, 30.0, 0.0, 0.0);

        drawLines(quaderOne);
        drawLines(quaderTwo);
    }

    // Camera 1
    QVector4D camera_1_positionWorld = QVector4D(1.0, 1.0, 1.0, 1.0);
    float camera_1_focalLength = 2;
    float camera_1_imagePlaneSize = 1;
    QVector3D camera_1_cameraRotation = QVector3D(10, 0, 0);
    QVector4D camera_1_projectionCenter = camera_1_positionWorld;
    QVector4D camera_1_imagePrinciplePoint = calculateImagePrinciplePoint(camera_1_focalLength, camera_1_positionWorld, camera_1_cameraRotation);
    if (!_disable_camera1) {
        initPerspectiveCameraModel(perspectiveCameraModelAxesLines, camera_1_positionWorld, camera_1_cameraRotation);
        drawLines(perspectiveCameraModelAxesLines);

        // draw camera
        if (!_disable_image_plane) {
            initImagePlane(imagePlaneLines, imagePlaneAxes ,camera_1_positionWorld, camera_1_imagePlaneSize, camera_1_focalLength, camera_1_cameraRotation, camera_1_imagePrinciplePoint);
            drawLines(imagePlaneLines);
            drawLines(imagePlaneAxes);
        }

        // draw projection
        if (!_disable_projection) {
            initProjection(quaderOne, camera_1_quaderOneProjection, camera_1_focalLength, camera_1_projectionCenter, camera_1_imagePrinciplePoint, camera_1_cameraRotation);
            initProjection(quaderTwo, camera_1_quaderTwoProjection, camera_1_focalLength, camera_1_projectionCenter, camera_1_imagePrinciplePoint, camera_1_cameraRotation);
            drawLines(camera_1_quaderOneProjection);
            drawLines(camera_1_quaderTwoProjection);
        }

        // draw projection Lines
        if (!_disable_rays_camera1) {
            initProjectionLines(quaderOne, projectionLines,  camera_1_projectionCenter);
            initProjectionLines(quaderTwo, projectionLines, camera_1_projectionCenter);
            drawLines(projectionLines);
        }
    }

    // Camera 2
    QVector4D camera_2_positionWorld = QVector4D(6.0, 1.0, 2.0, 1.0);
    float camera_2_focalLength = 3;
    float camera_2_imagePlaneSize = 1;
    QVector3D camera_2_cameraRotation = QVector3D(0, 45, 0);
    QVector4D camera_2_projectionCenter = camera_2_positionWorld;
    QVector4D camera_2_imagePrinciplePoint = calculateImagePrinciplePoint(camera_2_focalLength, camera_2_positionWorld, camera_2_cameraRotation);
    if (!_disable_camera2) {
        initPerspectiveCameraModel(perspectiveCameraModelAxesLines, camera_2_positionWorld, camera_2_cameraRotation);
        drawLines(perspectiveCameraModelAxesLines);
        // draw camera
        if (!_disable_image_plane) {
            initImagePlane(imagePlaneLines, imagePlaneAxes ,camera_2_positionWorld, camera_2_imagePlaneSize, camera_2_focalLength, camera_2_cameraRotation, camera_2_imagePrinciplePoint);
            drawLines(imagePlaneLines);
            drawLines(imagePlaneAxes);
        }

        // draw projection
        if (!_disable_projection) {
            initProjection(quaderOne, camera_2_quaderOneProjection, camera_2_focalLength, camera_2_projectionCenter, camera_2_imagePrinciplePoint, camera_2_cameraRotation);
            initProjection(quaderTwo, camera_2_quaderTwoProjection, camera_2_focalLength, camera_2_projectionCenter, camera_2_imagePrinciplePoint, camera_2_cameraRotation);
            drawLines(camera_2_quaderOneProjection);
            drawLines(camera_2_quaderTwoProjection);
        }

        // draw projection lines
        if (!_disable_rays_camera2) {
            initProjectionLines(quaderOne, projectionLines, camera_2_projectionCenter);
            initProjectionLines(quaderTwo, projectionLines, camera_2_projectionCenter);
            drawLines(projectionLines);
        }
    }

    // reconstruct cubes

    // reconstruct cubes wrong
}

QVector4D GLWidget::calculateImagePrinciplePoint(float focalLength, QVector4D positionCamera, QVector3D cameraRotation)
{
    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.setColumn(3, positionCamera);

    QMatrix4x4 rotation = rotation_x(cameraRotation.x()) * rotation_y(cameraRotation.y()) * rotation_z(cameraRotation.z());
    QVector4D imagePrinciplePoint = QVector4D(0, 0, focalLength, 1);
    imagePrinciplePoint = translationMatrix * rotation * imagePrinciplePoint;
    return imagePrinciplePoint;
}

void GLWidget::initQuader(std::vector<std::pair<QVector3D, QColor>> &quader, QVector4D translation, float size, float alpha_x, float alpha_y, float alpha_z)
{
    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.setColumn(3, translation);

    QVector3D a1 = QVector3D(0.0, 0.0, 0.0);
    QVector3D a2 = QVector3D(1.0, 0.0, 0.0);
    QVector3D a3 = QVector3D(1.0, 1.0, 0.0);
    QVector3D a4 = QVector3D(0.0, 1.0, 0.0);

    //rotate points.
    QMatrix4x4 rotationMatrix = rotation_x(alpha_x) * rotation_y(alpha_y) * rotation_z(alpha_z);
    a1 = rotationMatrix * a1;
    a2 = rotationMatrix * a2;
    a3 = rotationMatrix * a3;
    a4 = rotationMatrix * a4;

    //move and resize points.
    a1 = translationMatrix * a1 * size;
    a2 = translationMatrix * a2 * size;
    a3 = translationMatrix * a3 * size;
    a4 = translationMatrix * a4 * size;

    QVector3D b1 = QVector3D(0.0, 0.0, 1.0);
    QVector3D b2 = QVector3D(1.0, 0.0, 1.0);
    QVector3D b3 = QVector3D(1.0, 1.0, 1.0);
    QVector3D b4 = QVector3D(0.0, 1.0, 1.0);

    //rotate points.
    b1 = rotationMatrix * b1;
    b2 = rotationMatrix * b2;
    b3 = rotationMatrix * b3;
    b4 = rotationMatrix * b4;

    //move and resize points.
    b1 = translationMatrix * b1 * size;
    b2 = translationMatrix * b2 * size;
    b3 = translationMatrix * b3 * size;
    b4 = translationMatrix * b4 * size;

    //connect points in order to display quader correctly.
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

void GLWidget::initPerspectiveCameraModel(std::vector<std::pair<QVector3D, QColor>> &perspectiveCameraModelAxesLines, QVector4D translation, QVector3D rotation)
{
    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.setColumn(3, translation);

    QMatrix4x4 rotationMatrix = rotation_x(rotation.x()) * rotation_y(rotation.y()) * rotation_z(rotation.z());

    QVector3D center = QVector3D(0.0, 0.0, 0.0);
    QVector3D x = QVector3D(0.5, 0.0, 0.0);
    QVector3D y = QVector3D(0.0, 0.5, 0.0);
    QVector3D z = QVector3D(0.0, 0.0, 0.5);

    center = translationMatrix * rotationMatrix * center;
    x = translationMatrix * rotationMatrix * x;
    y = translationMatrix * rotationMatrix * y;
    z = translationMatrix * rotationMatrix * z;

    perspectiveCameraModelAxesLines.push_back(std::make_pair(center, QColor(1.0, 0.0, 0.0)));
    perspectiveCameraModelAxesLines.push_back(std::make_pair(x, QColor(1.0, 0.0, 0.0)));
    perspectiveCameraModelAxesLines.push_back(std::make_pair(center, QColor(0.0, 1.0, 0.0)));
    perspectiveCameraModelAxesLines.push_back(std::make_pair(y, QColor(0.0, 1.0, 0.0)));
    perspectiveCameraModelAxesLines.push_back(std::make_pair(center, QColor(0.0, 0.0, 1.0)));
    perspectiveCameraModelAxesLines.push_back(std::make_pair(z, QColor(0.0, 0.0, 1.0)));
}

void GLWidget::initImagePlane(std::vector<std::pair<QVector3D, QColor>> &imagePlaneLines, std::vector<std::pair<QVector3D, QColor>> &imagePlaneAxes, QVector4D positionInWorld, float size, float focal_length, QVector3D rotation, QVector4D imagePrinciplePoint)
{
    QMatrix4x4 scalingMatrix;
    scalingMatrix = QMatrix4x4(size, 0.0, 0.0, 0.0,
                                0.0, size, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0,
                                0.0, 0.0, 0.0, 1.0);

    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.setColumn(3, positionInWorld);

    QMatrix4x4 rotationMatrix = rotation_x(rotation.x()) * rotation_y(rotation.y()) * rotation_z(rotation.z());

    QVector3D a1 = QVector3D(1.0, 1.0, focal_length);
    QVector3D a2 = QVector3D(1.0, -1.0, focal_length);
    QVector3D a3 = QVector3D(-1.0, -1.0, focal_length);
    QVector3D a4 = QVector3D(-1.0, 1.0, focal_length);

    a1 = translationMatrix * rotationMatrix * scalingMatrix * a1;
    a2 = translationMatrix * rotationMatrix * scalingMatrix * a2;
    a3 = translationMatrix * rotationMatrix * scalingMatrix * a3;
    a4 = translationMatrix * rotationMatrix * scalingMatrix * a4;
    QColor color = QColor(1.0, 0.0, 0.0);

    imagePlaneLines.push_back(std::make_pair(a1, color));
    imagePlaneLines.push_back(std::make_pair(a2, color));

    imagePlaneLines.push_back(std::make_pair(a2, color));
    imagePlaneLines.push_back(std::make_pair(a3, color));

    imagePlaneLines.push_back(std::make_pair(a3, color));
    imagePlaneLines.push_back(std::make_pair(a4, color));

    imagePlaneLines.push_back(std::make_pair(a4, color));
    imagePlaneLines.push_back(std::make_pair(a1, color));

    // add axes

    QVector3D x = QVector3D(0.5, 0.0, focal_length);
    QVector3D y = QVector3D(0.0, 0.5, focal_length);

    x = translationMatrix * rotationMatrix * x;
    y = translationMatrix * rotationMatrix * y;

    imagePlaneAxes.push_back(std::make_pair(imagePrinciplePoint.toVector3D(), color));
    imagePlaneAxes.push_back(std::make_pair(x, color));

    imagePlaneAxes.push_back(std::make_pair(imagePrinciplePoint.toVector3D(), color));
    imagePlaneAxes.push_back(std::make_pair(y, color));

}

void GLWidget::initProjectionLines(std::vector<std::pair<QVector3D, QColor>> quader, std::vector<std::pair<QVector3D, QColor>> &projectionLines,  QVector4D projectionCenter)
{
    QColor color = QColor(0.0, 0.0, 1.0);
    for (auto vertex : quader) {
        projectionLines.push_back(std::make_pair(vertex.first, color));
        projectionLines.push_back(std::make_pair(projectionCenter.toVector3D(), color));
    }
}

void GLWidget::initProjection(std::vector<std::pair<QVector3D, QColor>> quader, std::vector<std::pair<QVector3D, QColor>> &projectionQuader, int focalLength, QVector4D projectionCenter, QVector4D imagePrinciplePoint, QVector3D camera_rotation)
{
    QVector4D image_plane = calculate_image_plane_equation(imagePrinciplePoint.toVector3D(), camera_rotation);
    QColor color = QColor(0.0, 1.0, 0.0);
    for (auto vertex : quader) {
        QVector3D projected_point = centralProjection(focalLength, vertex.first, projectionCenter.toVector3D(), imagePrinciplePoint.toVector3D(), camera_rotation, image_plane);
        projectionQuader.push_back(std::make_pair(projected_point, color));
    }
}

QVector4D GLWidget::calculate_image_plane_equation(QVector3D imagePrinciplePoint, QVector3D rotation)
{
    // used to calculate z point on image plane: a*x+b*y+c*z=w
    // Returns: an vector where vector.x = a, vector.y = b, vector.z = c and vector.w = w

    QMatrix4x4 rotation_matrix = rotation_x(rotation.x()) * rotation_y(rotation.y()) * rotation_z(rotation.z());

    QVector3D plane_v1 = QVector3D(1.0, 0.0, 0);
    QVector3D plane_v2 = QVector3D(0.0, 1.0, 0);

    plane_v1 = rotation_matrix * plane_v1;
    plane_v2 = rotation_matrix * plane_v2;

    float normal_vektor_x = (plane_v1.y() * plane_v2.z()) - (plane_v1.z() * plane_v2.y());
    float normal_vektor_y = (plane_v1.z() * plane_v2.x()) - (plane_v1.x() * plane_v2.z());
    float normal_vektor_z = (plane_v1.x() * plane_v2.y()) - (plane_v1.y() * plane_v2.x());
    float result_plane_calc = normal_vektor_x * imagePrinciplePoint.x()  + normal_vektor_y * imagePrinciplePoint.y() + normal_vektor_z * imagePrinciplePoint.z();

    return QVector4D(normal_vektor_x, normal_vektor_y, normal_vektor_z, result_plane_calc);
}

QVector3D GLWidget::centralProjection(float focalLength, QVector3D vertex, QVector3D projectionCenter, QVector3D imagePrinciplePoint, QVector3D rotation, QVector4D image_plane)
{
    QMatrix4x4 rotation_matrix = (rotation_x(rotation.x()) * rotation_y(rotation.y()) * rotation_z(rotation.z())).transposed();

    //calculate x
    float x_term_1 = rotation_matrix.column(0).x() * ( vertex.x() - projectionCenter.x());
    float x_term_2 = rotation_matrix.column(1).x() * ( vertex.y() - projectionCenter.y());
    float x_term_3 = rotation_matrix.column(2).x() * ( vertex.z() - projectionCenter.z());
    float x_term_4 = rotation_matrix.column(0).z() * ( vertex.x() - projectionCenter.x());
    float x_term_5 = rotation_matrix.column(1).z() * ( vertex.y() - projectionCenter.y());
    float x_term_6 = rotation_matrix.column(2).z() * ( vertex.z() - projectionCenter.z());
    float x = imagePrinciplePoint.x() + focalLength * ( (x_term_1 + x_term_2 + x_term_3) / (x_term_4 + x_term_5 + x_term_6));

    // calculate y
    float y_term_1 = rotation_matrix.column(0).y() * ( vertex.x() - projectionCenter.x());
    float y_term_2 = rotation_matrix.column(1).y() * ( vertex.y() - projectionCenter.y());
    float y_term_3 = rotation_matrix.column(2).y() * ( vertex.z() - projectionCenter.z());
    float y_term_4 = rotation_matrix.column(0).z() * ( vertex.x() - projectionCenter.x());
    float y_term_5 = rotation_matrix.column(1).z() * ( vertex.y() - projectionCenter.y());
    float y_term_6 = rotation_matrix.column(2).z() * ( vertex.z() - projectionCenter.z());
    float y = imagePrinciplePoint.y() + focalLength * ( (y_term_1 + y_term_2 + y_term_3) / (y_term_4 + y_term_5 + y_term_6));

    // calculate z
    float z = (1 / image_plane.z()) * (image_plane.w() -(image_plane.x() * x) - (image_plane.y() * y)) ;
    return QVector3D(x, y, z);
}

void GLWidget::drawLines(std::vector<std::pair<QVector3D, QColor>> quader)
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
    _show_aufgabe_1 = true;
    _show_aufgabe_2 = false;
    update();
}

void GLWidget::radioButton2Clicked()
{
    _show_aufgabe_1 = false;
    _show_aufgabe_2 = true;
    update();
}

void GLWidget::disable_cubes()
{
    if (_disable_cubes == true) {
        _disable_cubes = false;
    } else {
        _disable_cubes = true;
    }
    update();
}

void GLWidget::disable_projection()
{
    if (_disable_projection == true) {
        _disable_projection = false;
    } else {
        _disable_projection = true;
    }
    update();
}

void GLWidget::disable_rays()
{
    if (_disable_rays_camera1 == true) {
        _disable_rays_camera1 = false;
    } else {
        _disable_rays_camera1 = true;
    }
    update();
}

void GLWidget::disable_image_plane()
{
    if (_disable_image_plane == true) {
        _disable_image_plane = false;
    } else {
        _disable_image_plane = true;
    }
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
