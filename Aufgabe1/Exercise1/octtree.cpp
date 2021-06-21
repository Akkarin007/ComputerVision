#include <octtree.h>

Octtree::Octtree(QVector3D new_near_bot_left, QVector3D new_far_top_right, float new_length)
{
    Node *tmp = new Node(new_near_bot_left, new_far_top_right, new_length);
    this->root = tmp;
}

void Octtree::get_octtree_lines(std::vector<std::pair<QVector3D, QColor>> octtree_lines, QColor colour, int depth, Node *current)
{
    // handle leaf
    if (current->is_empty) {
        return;
    }

    // add lines
    QVector3D point1 = current->near_bot_left;
    QVector3D point2 = QVector3D(current->far_top_right.x(), current->near_bot_left.y(), current->near_bot_left.z());
    QVector3D point3 = QVector3D(current->far_top_right.x(), current->far_top_right.y(), current->near_bot_left.z());
    QVector3D point4 = QVector3D(current->near_bot_left.x(), current->far_top_right.y(), current->near_bot_left.z());

    QVector3D point5 = QVector3D(current->near_bot_left.x(), current->near_bot_left.y(), current->far_top_right.z());
    QVector3D point6 = QVector3D(current->far_top_right.x(), current->near_bot_left.y(), current->far_top_right.z());
    QVector3D point7 = current->far_top_right;
    QVector3D point8 = QVector3D(current->near_bot_left.x(), current->far_top_right.y(), current->far_top_right.z());

    // front lines:
    octtree_lines.push_back(std::make_pair(point1, colour));
    octtree_lines.push_back(std::make_pair(point2, colour));

    octtree_lines.push_back(std::make_pair(point2, colour));
    octtree_lines.push_back(std::make_pair(point3, colour));

    octtree_lines.push_back(std::make_pair(point3, colour));
    octtree_lines.push_back(std::make_pair(point4, colour));

    octtree_lines.push_back(std::make_pair(point4, colour));
    octtree_lines.push_back(std::make_pair(point1, colour));

    // back lines:
    octtree_lines.push_back(std::make_pair(point5, colour));
    octtree_lines.push_back(std::make_pair(point6, colour));

    octtree_lines.push_back(std::make_pair(point6, colour));
    octtree_lines.push_back(std::make_pair(point7, colour));

    octtree_lines.push_back(std::make_pair(point7, colour));
    octtree_lines.push_back(std::make_pair(point8, colour));

    octtree_lines.push_back(std::make_pair(point8, colour));
    octtree_lines.push_back(std::make_pair(point5, colour));

    // quere linien:
    octtree_lines.push_back(std::make_pair(point1, colour));
    octtree_lines.push_back(std::make_pair(point5, colour));

    octtree_lines.push_back(std::make_pair(point2, colour));
    octtree_lines.push_back(std::make_pair(point6, colour));

    octtree_lines.push_back(std::make_pair(point3, colour));
    octtree_lines.push_back(std::make_pair(point7, colour));

    octtree_lines.push_back(std::make_pair(point4, colour));
    octtree_lines.push_back(std::make_pair(point8, colour));
    // handle depth
    if (depth == 0)
    {
        return;
    }

    // get new nodes
    for (Node *new_current: root->children)
    {

        get_octtree_lines(octtree_lines, colour, depth - 1, new_current);
    }
}


bool Octtree::insert_point(QVector3D point, Node *current)
{
    // handle leaf
    if (current->is_leaf) {
        current->split();
    }

    // handle nullpointer in children
    if (current->is_empty)
    {
        current->set_leaf(-1, &point);
        return true;
    }

    // get index for new fitting node
    int index = current->get_index(point);
    if (index > -1)
    {
        if (insert_point(point, current->children[index]))
        {
            return true;
        }
    }
    return false;
}
