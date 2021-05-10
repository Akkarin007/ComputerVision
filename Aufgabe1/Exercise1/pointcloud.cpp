#include "pointcloud.h"
#include <fstream>
#include <sstream>
#include <iostream>

PointCloud::PointCloud()
{}

PointCloud::~PointCloud()
{}


bool PointCloud::loadPLY(const QString& filePath)
{

    // open stream
    std::fstream is;
    is.open(filePath.toStdString().c_str(), std::fstream::in);

    // ensure format with magic header
    std::string line;
    std::getline(is, line);
    if (line != "ply") {
      throw std::runtime_error("not a ply file");
        return false;
    }

    // parse header looking only for 'element vertex' section size
    _pointsCount = 0;
    while (is.good()) {
      std::getline(is, line);
      if (line == "end_header") {
        break;
      } else {
        std::stringstream ss(line);
        std::string tag1, tag2, tag3;
        ss >> tag1 >> tag2 >> tag3;
        if (tag1 == "element" && tag2 == "vertex") {
          _pointsCount = std::atof(tag3.c_str());
        }
      }
    }

    // read and parse 'element vertex' section
    if (_pointsCount > 0) {
      _pointsData.resize(_pointsCount * POINT_STRIDE);

      std::stringstream ss;
      std::string line;
      float *p = _pointsData.data();
      for (size_t i = 0; is.good() && i < _pointsCount; ++i) {
        std::getline(is, line);
        ss.str(line);
        float x, y, z;
        ss >> x >> y >> z;

        *p++ = x;
        *p++ = y;
        *p++ = z;
        *p++ = i;

        // updates for AABB
        _pointsBoundMax[0] = std::max(x, _pointsBoundMax[0]);
        _pointsBoundMax[1] = std::max(y, _pointsBoundMax[1]);
        _pointsBoundMax[2] = std::max(z, _pointsBoundMax[2]);
        _pointsBoundMin[0] = std::min(x, _pointsBoundMin[0]);
        _pointsBoundMin[1] = std::min(y, _pointsBoundMin[1]);
        _pointsBoundMin[2] = std::min(z, _pointsBoundMin[2]);
      }

      // basic validation
      if (p - _pointsData.data() < _pointsData.size()) {
        throw std::runtime_error("broken ply file");
          return false;
      }

      std::cout << "number of points: " + std::to_string(_pointsCount) << std::endl;

    }
    return true;
}
