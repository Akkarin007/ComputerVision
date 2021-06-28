
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

bool compareX(QVector3D v1, QVector3D v2)
{
    return (v1.x() < v2.x());
}

bool compareY(QVector3D v1, QVector3D v2)
{
    return (v1.y() < v2.y());
}

bool compareZ(QVector3D v1, QVector3D v2)
{
    return (v1.z() < v2.z());
}

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

    if (_show_aufgabe_1 == true)
    {
        // draw world cordinate system
        drawLines(_axesLines);
        aufgabe_1();
    } else if (_show_aufgabe_2 == true)
    {
        // draw world cordinate system
        drawLines(_axesLines);
        aufgabe_2();
    } else if (_show_aufgabe_3_1 == true)
    {
        //
        // draw points cloud
        //
        drawPointCloud();

        aufgabe_3_1();
    } else if (_show_aufgabe_3_2 == true)
    {
        //
        // draw points cloud
        //
        drawPointCloud();
        aufgabe_3_2();
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
    initQuader(quaderOne, positionWorldQuaderOne, 0.80, 0.0, 30, 0.0);
    initQuader(quaderTwo, positionWorldQuaderTwo, 1.20, 30.0, 0.0, 0.0);
    if (!_disable_cubes) {
        drawLines(quaderOne);
        drawLines(quaderTwo);
    }

    // Assignement 1, Part 2
    // Draw here your perspective camera model
    QVector4D positionWorldCamera = QVector4D(1.0, 1.0, 1.0, 1.0);
    float focalLength = 2;
    float imagePlaneSize = 1;
    QVector4D projectionCenter = positionWorldCamera;
    QVector4D imagePrinciplePoint = calculateImagePrinciplePoint(focalLength, positionWorldCamera, _rotation_camera_1);

    initPerspectiveCameraModel(perspectiveCameraModelAxesLines, positionWorldCamera, _rotation_camera_1);
    initImagePlane(imagePlaneLines, imagePlaneAxes ,positionWorldCamera, imagePlaneSize, focalLength, _rotation_camera_1, imagePrinciplePoint);
    drawLines(perspectiveCameraModelAxesLines);

    if (!_disable_image_plane) {
        drawLines(imagePlaneLines);
        drawLines(imagePlaneAxes);
    }

    // Assignement 1, Part 3
    // Draw here the perspective projection
    initProjection(quaderOne, quaderOneProjection, focalLength, projectionCenter, imagePrinciplePoint, _rotation_camera_1);
    initProjection(quaderTwo, quaderTwoProjection, focalLength, projectionCenter, imagePrinciplePoint, _rotation_camera_1);
    if (!_disable_projection) {
        drawLines(quaderOneProjection);
        drawLines(quaderTwoProjection);
    }

    // Draw projection Lines
    initProjectionLines(quaderOne, projectionLines, projectionCenter);
    initProjectionLines(quaderTwo, projectionLines, projectionCenter);
    if (!_disable_rays) {
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
    initQuader(quaderOne, positionWorldQuaderOne, 0.80, 0.0, 30, 0.0);
    initQuader(quaderTwo, positionWorldQuaderTwo, 1.20, 30.0, 0.0, 0.0);
    if (!_disable_cubes) {
        drawLines(quaderOne);
        drawLines(quaderTwo);
    }

    // Camera 1
    QVector4D camera_1_positionWorld = QVector4D(0.5, 1.0, 1.0, 1.0);
    float camera_1_focalLength = 2;
    float camera_1_imagePlaneSize = 1;
    QVector4D camera_1_projectionCenter = camera_1_positionWorld;
    QVector4D camera_1_imagePrinciplePoint = calculateImagePrinciplePoint(camera_1_focalLength, camera_1_positionWorld, _rotation_camera_1);
    if (!_disable_camera1) {
        initPerspectiveCameraModel(perspectiveCameraModelAxesLines, camera_1_positionWorld, _rotation_camera_1);
        initImagePlane(imagePlaneLines, imagePlaneAxes ,camera_1_positionWorld, camera_1_imagePlaneSize, camera_1_focalLength, _rotation_camera_1, camera_1_imagePrinciplePoint);
        drawLines(perspectiveCameraModelAxesLines);

        // draw camera
        if (!_disable_image_plane) {
            drawLines(imagePlaneLines);
            drawLines(imagePlaneAxes);
        }

        // draw projection
        initProjection(quaderOne, camera_1_quaderOneProjection, camera_1_focalLength, camera_1_projectionCenter, camera_1_imagePrinciplePoint, _rotation_camera_1);
        initProjection(quaderTwo, camera_1_quaderTwoProjection, camera_1_focalLength, camera_1_projectionCenter, camera_1_imagePrinciplePoint, _rotation_camera_1);
        if (!_disable_projection) {
            drawLines(camera_1_quaderOneProjection);
            drawLines(camera_1_quaderTwoProjection);
        }

        // draw projection Lines
        initProjectionLines(quaderOne, projectionLines,  camera_1_projectionCenter);
        initProjectionLines(quaderTwo, projectionLines, camera_1_projectionCenter);
        if (!_disable_rays) {
            drawLines(projectionLines);
        }
    }

    // Camera 2
    QVector4D camera_2_positionWorld = QVector4D(2.5, 1.0, 1, 1.0);
    float camera_2_focalLength = 2;
    float camera_2_imagePlaneSize = 1;
    QVector4D camera_2_projectionCenter = camera_2_positionWorld;
    QVector4D camera_2_imagePrinciplePoint = calculateImagePrinciplePoint(camera_2_focalLength, camera_2_positionWorld, _rotation_camera_2);
    if (!_disable_camera2) {
        initPerspectiveCameraModel(perspectiveCameraModelAxesLines, camera_2_positionWorld, _rotation_camera_2);
        initImagePlane(imagePlaneLines, imagePlaneAxes ,camera_2_positionWorld, camera_2_imagePlaneSize, camera_2_focalLength, _rotation_camera_2, camera_2_imagePrinciplePoint);
        drawLines(perspectiveCameraModelAxesLines);
        // draw camera
        if (!_disable_image_plane) {
            drawLines(imagePlaneLines);
            drawLines(imagePlaneAxes);
        }

        // draw projection
        initProjection(quaderOne, camera_2_quaderOneProjection, camera_2_focalLength, camera_2_projectionCenter, camera_2_imagePrinciplePoint, _rotation_camera_2);
        initProjection(quaderTwo, camera_2_quaderTwoProjection, camera_2_focalLength, camera_2_projectionCenter, camera_2_imagePrinciplePoint, _rotation_camera_2);
        if (!_disable_projection) {
            drawLines(camera_2_quaderOneProjection);
            drawLines(camera_2_quaderTwoProjection);
        }

        // draw projection lines
        initProjectionLines(quaderOne, projectionLines, camera_2_projectionCenter);
        initProjectionLines(quaderTwo, projectionLines, camera_2_projectionCenter);
        if (!_disable_rays) {
            drawLines(projectionLines);
        }
    }

    // reconstruct cubes
    std::vector<std::pair<QVector3D, QColor> > quaderOneCorrectReconstruction;
    std::vector<std::pair<QVector3D, QColor> > quaderTwoCorrectReconstruction;
    if (!_disable_camera1 && !_disable_camera2) {
        initStereoVisionNormalCaseReconstruction(camera_1_quaderOneProjection, camera_2_quaderOneProjection, quaderOneCorrectReconstruction, camera_1_focalLength, camera_1_positionWorld.toVector3D(), camera_2_positionWorld.toVector3D());
        initStereoVisionNormalCaseReconstruction(camera_1_quaderTwoProjection, camera_2_quaderTwoProjection, quaderTwoCorrectReconstruction, camera_1_focalLength, camera_1_positionWorld.toVector3D(), camera_2_positionWorld.toVector3D());
        if (!_disable_reconstruction) {
            drawLines(quaderOneCorrectReconstruction);
            drawLines(quaderTwoCorrectReconstruction);
        }
    }
    // reconstruct cubes wrong
}

void GLWidget::aufgabe_3_1()
{
    if (_load_point_cloud) {
        load_point_cloud();
    }


    std::vector<std::pair<QVector3D, QColor> > kdTreeLines;
    std::vector<std::pair<QVector3D, QColor> > kdTreePoints;
    constructBalanced3DTree(kdTreeLines, kdTreePoints, 0, x_array.size()-2, NULL, 0, 4);


    if (!_disable_tree)
    {
        drawKDTreeLines(kdTreeLines);

        drawKDTreePoints(kdTreePoints);
    }
}

void GLWidget::aufgabe_3_2()
{
    if (_load_point_cloud) {
        load_point_cloud();
    }
    std::vector<std::pair<QVector3D, QColor> > octtree_lines;

    Octtree new_tree = init_octtree(octtree_lines);
    if (!_disable_tree)
    {
        drawLines(octtree_lines);
    }
}

Octtree GLWidget::init_octtree(std::vector<std::pair<QVector3D, QColor> > &octtree_lines)
{
    QVector3D point1 = QVector3D(-6,-5,-6);
    QVector3D point2 = QVector3D(4,5,4);
    float length = 10.0;
    Octtree *octtree = new Octtree(point1, point2, length);

    //octtree_lines.push_back(std::make_pair(point1, QColor(1,0,0)));
    //octtree_lines.push_back(std::make_pair(point2, QColor(1,0,0)));
    //drawLines(octtree_lines);

    // add points into octtree
    for (QVector3D point: x_array)
    {
        if (!octtree->insert_point(point))
        {
            //printf("Point outside of octtree! x: %f, y: %f, z: %f\n", point.x(), point.y(), point.z());
        }
    }

    // read octtree_lines
    int depth = 5;
    octtree->get_octtree_lines(octtree_lines, QColor(0,0,1), depth, *octtree->root);
    return *octtree;
}

void GLWidget::load_point_cloud()
{
    _load_point_cloud = false;
    x_array.clear();
    y_array.clear();
    z_array.clear();

    pointcloud.loadPLY(_point_cloud_path);
    printf("%f %f %f",pointcloud.getMax().x(), pointcloud.getMax().y(), pointcloud.getMax().z());
    printf("%f %f %f",pointcloud.getMin().x(), pointcloud.getMin().y(), pointcloud.getMin().z());
    const QVector<float>& pointsData = pointcloud.getData();
    QVectorIterator<float> i(pointsData);
    const float *p = pointsData.data();
    for (size_t i = 0; i < pointsData.size(); ++i) {

      float x, y, z;

      x = *p++;
      y = *p++;
      z = *p++;
      i = *p++;
      QVector3D vec = QVector3D(x,y,z);
      x_array.push_back(vec);
      y_array.push_back(vec);
      z_array.push_back(vec);

    }

    std::sort(x_array.begin(), x_array.end(), compareX);
    std::sort(y_array.begin(), y_array.end(), compareY);
    std::sort(z_array.begin(), z_array.end(), compareZ);

    std::cout << "sorting done for x, y, z Arrays in Task 3"<< std::endl;
    //for (auto x : x_array)
    //    std::cout << "x[" << x.x() << ", " << x.y() << ", " << x.z() << "] "<< std::endl;
    //for (auto y : y_array)
    //    std::cout << "y[" << y.x() << ", " << y.y() << ", " << y.z()<< "] "<< std::endl;
    //for (auto z : z_array)
    //    std::cout << "z[" << z.x() << ", " << z.y() << ", " << z.z() << "] "<< std::endl;

    update();
}

void GLWidget::constructBalanced3DTree(std::vector<std::pair<QVector3D, QColor> > &kdTreeLines, std::vector<std::pair<QVector3D, QColor> > &points, int left, int right, Tree * node, int d, int maxLvl)
{
    if (maxLvl > 0) {
        node = new Tree;

        if (left <= right){
            int m = (left + right) / 2;
            if (d == 0) {
                node->data = y_array[m];
                node->split = "y-split";

                points.push_back(std::make_pair(QVector3D(y_array[m].x(), y_array[m].y(), y_array[m].z()), QColor(0.0, 1.0, 1.0)));

                kdTreeLines.push_back(std::make_pair(QVector3D(pointcloud.getMin().x(), y_array[m].y(), y_array[m].z()), QColor(1.0, 0.0, 0.0)));
                kdTreeLines.push_back(std::make_pair(QVector3D(pointcloud.getMax().x(), y_array[m].y(), y_array[m].z()), QColor(1.0, 0.0, 0.0)));

                partitionField(x_array, left, right, y_array[m], m, "y-split");
            } else if (d == 1) {
                node->data = x_array[m];
                node->split = "x-split";

                points.push_back(std::make_pair(QVector3D(x_array[m].x(), x_array[m].y(), x_array[m].z()), QColor(0.0, 1.0, 1.0)));

                kdTreeLines.push_back(std::make_pair(QVector3D(x_array[m].x(), pointcloud.getMin().y(), x_array[m].z()), QColor(0.0, 1.0, 0.0)));
                kdTreeLines.push_back(std::make_pair(QVector3D(x_array[m].x(), pointcloud.getMax().y(), x_array[m].z()), QColor(0.0, 1.0, 0.0)));
                partitionField(z_array, left, right, x_array[m], m, "x-split");
            } else if (d == 2) {
                node->data = z_array[m];
                node->split = "z-split";

                points.push_back(std::make_pair(QVector3D(z_array[m].x(), z_array[m].y(), z_array[m].z()), QColor(0.0, 1.0, 1.0)));

                kdTreeLines.push_back(std::make_pair(QVector3D(z_array[m].x(), z_array[m].y(), pointcloud.getMin().z()), QColor(0.0, 0.0, 1.0)));
                kdTreeLines.push_back(std::make_pair(QVector3D(z_array[m].x(), z_array[m].y(), pointcloud.getMax().z()), QColor(0.0, 0.0, 1.0)));

                partitionField(y_array, left, right, z_array[m], m, "z-split");

            }

            constructBalanced3DTree(kdTreeLines, points,left, m-1, node->left, (d + 1) % 3, maxLvl - 1);
            constructBalanced3DTree(kdTreeLines, points, m+1, right, node->right, (d + 1 ) % 3, maxLvl - 1);
        }
    }
}

void GLWidget::partitionField(std::vector<QVector3D> array, int left, int right, QVector3D medianVector, int m, std::string direction)
{
    std::vector<QVector3D> tmp1;
    std::vector<QVector3D> tmp2;

    for (int i = left; i <= right; i++) {
        if (direction == "x-split") {
            if(array[i].x() < medianVector.x()) tmp1.push_back(array[i]);
            if(array[i].x() > medianVector.x()) tmp2.push_back(array[i]);
        } else if (direction == "y-split") {
            if(array[i].y() < medianVector.y()) tmp1.push_back(array[i]);
            if(array[i].y() > medianVector.y()) tmp2.push_back(array[i]);
        } else if (direction == "z-split") {
            if(array[i].z() < medianVector.z()) tmp1.push_back(array[i]);
            if(array[i].z() > medianVector.z()) tmp2.push_back(array[i]);
        }



    }
    for (int i = 0; i < tmp1.size(); i++) array[left + i] = tmp1[i];
    for (int i = 0; i < tmp2.size(); i++) array[m + i + 1] = tmp2[i];

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

void GLWidget::initStereoVisionNormalCaseReconstruction(std::vector<std::pair<QVector3D, QColor>> projection1, std::vector<std::pair<QVector3D, QColor>> projection2, std::vector<std::pair<QVector3D, QColor>> &reconstruction, float focalLength, QVector3D camera_1_pos, QVector3D camera_2_pos)
{
    int count = 0;
    QColor color = QColor(0.0, 0.0, 1.0);
    while(count < (int) projection1.size()) {
        QVector3D tmp_v1 = QVector3D(projection1[count].first.x() - camera_1_pos.x(), projection1[count].first.y() - camera_1_pos.y(), projection1[count].first.z() - camera_1_pos.z());
        QVector3D tmp_v2 = QVector3D(projection2[count].first.x() - camera_2_pos.x(), projection1[count].first.y() - camera_2_pos.y(), projection1[count].first.z() - camera_2_pos.z());
        QVector3D projected_point = stereoVisionNormalCaseReconstruction(-focalLength, camera_1_pos.x() - camera_2_pos.x(), tmp_v1, tmp_v2);
        projected_point = QVector3D(camera_1_pos.x() + projected_point.x(), camera_1_pos.y() + projected_point.y(), camera_1_pos.z() + projected_point.z());
        reconstruction.push_back(std::make_pair(projected_point, color));
        count++;
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

QVector3D GLWidget::stereoVisionNormalCaseReconstruction(float focalLength, float b, QVector3D vertex1, QVector3D vertex2)
{
    // calculate z
    // b = disparity in x direction between images
    float z = -focalLength * ( b / (vertex2.x() - vertex1.x()));
    float y = -z * (vertex1.y()/focalLength);
    // float y_control = -z * (vertex2.y()/focalLength);
    float x = -z * (vertex1.x()/focalLength);
    return QVector3D(x, y, z);
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
    QMatrix4x4 rotation_matrix_t = (rotation_x(rotation.x()) * rotation_y(rotation.y()) * rotation_z(rotation.z())).transposed();
    QMatrix4x4 rotation_matrix = (rotation_x(rotation.x()) * rotation_y(rotation.y()) * rotation_z(rotation.z()));

    //calculate x
    float x_term_1 = rotation_matrix_t.column(0).x() * ( vertex.x() - projectionCenter.x());
    float x_term_2 = rotation_matrix_t.column(1).x() * ( vertex.y() - projectionCenter.y());
    float x_term_3 = rotation_matrix_t.column(2).x() * ( vertex.z() - projectionCenter.z());
    float x_term_4 = rotation_matrix_t.column(0).z() * ( vertex.x() - projectionCenter.x());
    float x_term_5 = rotation_matrix_t.column(1).z() * ( vertex.y() - projectionCenter.y());
    float x_term_6 = rotation_matrix_t.column(2).z() * ( vertex.z() - projectionCenter.z());
    float x = focalLength * ( (x_term_1 + x_term_2 + x_term_3) / (x_term_4 + x_term_5 + x_term_6));

    // calculate y
    float y_term_1 = rotation_matrix_t.column(0).y() * ( vertex.x() - projectionCenter.x());
    float y_term_2 = rotation_matrix_t.column(1).y() * ( vertex.y() - projectionCenter.y());
    float y_term_3 = rotation_matrix_t.column(2).y() * ( vertex.z() - projectionCenter.z());
    float y_term_4 = rotation_matrix_t.column(0).z() * ( vertex.x() - projectionCenter.x());
    float y_term_5 = rotation_matrix_t.column(1).z() * ( vertex.y() - projectionCenter.y());
    float y_term_6 = rotation_matrix_t.column(2).z() * ( vertex.z() - projectionCenter.z());
    float y = focalLength * ( (y_term_1 + y_term_2 + y_term_3) / (y_term_4 + y_term_5 + y_term_6));

    // calculate z
    float z = focalLength;

    QVector3D result = QVector3D(x,y,z);
    result = rotation_matrix * result;

    return QVector3D(result.x() + projectionCenter.x(), result.y() + projectionCenter.y(), result.z() + projectionCenter.z());
}

void GLWidget::drawLines(std::vector<std::pair<QVector3D, QColor>> quader)
{
  glBegin(GL_LINES);
  QMatrix4x4 mvMatrix = _projectionMatrix * _cameraMatrix * _worldMatrix;
  mvMatrix.scale(0.05f); // make it small
  for (auto vertex : quader) {
    const auto translated = mvMatrix * vertex.first;
    glColor3f(vertex.second.red(), vertex.second.green(), vertex.second.blue());
    glVertex3f(translated.x(), translated.y(), translated.z());
  }
  glEnd();
}

void GLWidget::drawKDTreePoints(std::vector<std::pair<QVector3D, QColor>> quader)
{
  glEnable(GL_PROGRAM_POINT_SIZE);
  glPointSize(8.0f);
  glBegin(GL_POINTS);
   const auto viewMatrix = _projectionMatrix * _cameraMatrix * _worldMatrix;
  for (auto vertex : quader) {
    const auto translated = viewMatrix * vertex.first;
    glColor3f(vertex.second.red(), vertex.second.green(), vertex.second.blue());
    glVertex3f(translated.x(), translated.y(), translated.z());
  }
  glEnd();
}

void GLWidget::drawKDTreeLines(std::vector<std::pair<QVector3D, QColor>> quader)
{
  glBegin(GL_LINES);

   const auto viewMatrix = _projectionMatrix * _cameraMatrix * _worldMatrix;
  for (auto vertex : quader) {
    const auto translated = viewMatrix * vertex.first;
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

    if (_show_aufgabe_1 || _show_aufgabe_2)
    {
        return;
    }

    if (!filePath.isEmpty())
    {
        _point_cloud_path = filePath;
        std::cout << filePath.toStdString() << std::endl;
        load_point_cloud();
        update();
    }
}

void GLWidget::radioButton1Clicked()
{
    // TODO: toggle to Jarvis' march
    _show_aufgabe_1 = true;
    _show_aufgabe_2 = false;
    _show_aufgabe_3_1 = false;
    _show_aufgabe_3_2 = false;

    update();
}

void GLWidget::radioButton2Clicked()
{
    _show_aufgabe_1 = false;
    _show_aufgabe_2 = true;
    _show_aufgabe_3_1 = false;
    _show_aufgabe_3_2 = false;

    update();
}

void GLWidget::radioButton3Clicked()
{
    _show_aufgabe_1 = false;
    _show_aufgabe_2 = false;
    _show_aufgabe_3_1 = true;
    _show_aufgabe_3_2 = false;

    update();
}

void GLWidget::radioButton4Clicked()
{
    _show_aufgabe_1 = false;
    _show_aufgabe_2 = false;
    _show_aufgabe_3_1 = false;
    _show_aufgabe_3_2 = true;

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
    if (_disable_rays == true) {
        _disable_rays = false;
    } else {
        _disable_rays = true;
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

void GLWidget::disable_camera_1()
{
    if (_disable_camera1 == true) {
        _disable_camera1 = false;
    } else {
        _disable_camera1 = true;
    }
    update();
}

void GLWidget::disable_camera_2()
{
    if (_disable_camera2 == true) {
        _disable_camera2 = false;
    } else {
        _disable_camera2 = true;
    }
    update();
}

void GLWidget::disable_reconstruction()
{
    if (_disable_reconstruction == true) {
        _disable_reconstruction = false;
    } else {
        _disable_reconstruction = true;
    }
    update();
}

void GLWidget::disable_tree()
{
    if (_disable_tree == true) {
        _disable_tree = false;
    } else {
        _disable_tree = true;
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
