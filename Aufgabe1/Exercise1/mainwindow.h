//
// (c) Georg Umlauf, 2021
//

#pragma once

#include <QtWidgets/QMainWindow>
#include <QVector3D>
#include <QSharedPointer>

#include "ui_mainwindow.h"

#include "camera.h"
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
	~MainWindow();

protected slots:
  void openFileDialog();
  void updatePointSize(size_t);


private:
	Ui::MainWindowClass *ui;
    QSharedPointer<Camera> _camera;
};
