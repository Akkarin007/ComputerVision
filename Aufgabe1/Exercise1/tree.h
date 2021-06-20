#ifndef TREE_H
#define TREE_H
#include <QVector3D>

class Tree
{
public:
    Tree();
    Tree * left;
    Tree * right;
    std::string split;
    int level;
    QVector3D data;
};

#endif // TREE_H
