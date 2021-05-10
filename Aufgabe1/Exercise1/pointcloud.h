#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include <QString>
#include <QVector>
#include <QVector3D>


static const size_t POINT_STRIDE = 4; // x, y, z, index

class PointCloud
{
public:
    PointCloud();
    ~PointCloud();

    bool loadPLY(const QString&);

private:

    size_t _pointsCount=0;
    QVector<float> _pointsData;
    QVector3D _pointsBoundMin;
    QVector3D _pointsBoundMax;

public:
    size_t getCount() const { return _pointsCount; }
    QVector3D getMin() const { return _pointsBoundMin; }
    QVector3D getMax() const { return _pointsBoundMax; }
    const QVector<float>& getData() const { return _pointsData; }

};

#endif // POINTCLOUD_H
