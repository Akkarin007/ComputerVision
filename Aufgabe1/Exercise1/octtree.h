#ifndef OCTTREE_H
#define OCTTREE_H
#include <QVector3D>
#include <QColor>
#include "Node.h"


class Octtree
{
public:

    // variables
    Node *root;

    //functions
    Octtree(QVector3D new_near_bot_left, QVector3D new_far_top_right, float new_length);
    void get_octtree_lines(std::vector<std::pair<QVector3D, QColor>> &octtree_lines, QColor colour, int depth, Node *current);
    bool insert_point(QVector3D point, Node *current);
};
#endif // OCTTREE_H
