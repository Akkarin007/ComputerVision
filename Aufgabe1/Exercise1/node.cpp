#include <Node.h>
#include <QVector3D>

Node::Node(QVector3D new_near_bot_left, QVector3D new_far_top_right, float new_length)
{
    near_bot_left = new_near_bot_left;
    far_top_right = new_far_top_right;
    length = new_length;
    is_leaf = false;
    is_empty = true;
}

void Node::split()
{
    QVector3D *tmp_leaf_value = leaf_value;

    float new_length = length/2;

    children[0] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y() + new_length, near_bot_left.z()),
                           QVector3D(far_top_right.x(), far_top_right.y(), far_top_right.z() - new_length), new_length);

    children[1] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y() + new_length, near_bot_left.z()),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y(), far_top_right.z() - new_length),new_length);

    children[2] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y(), near_bot_left.z()),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y() - new_length, far_top_right.z() - new_length), new_length);

    children[3] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y(), near_bot_left.z()),
                           QVector3D(far_top_right.x(), far_top_right.y() - new_length, far_top_right.z() - new_length), new_length);

    children[4] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y() + new_length, near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x(), far_top_right.y(), far_top_right.z()), new_length);

    children[5] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y() + new_length, near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y(), far_top_right.z()), new_length);

    children[6] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y(), near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y() - new_length, far_top_right.z()), new_length);

    children[7] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y(), near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x(), far_top_right.y() - new_length, far_top_right.z()), new_length);

    // remove leafe attribute
    this->leaf_value = nullptr;
    this->is_leaf = false;

    // add leave attribute
    int index = get_index(*tmp_leaf_value);
    {
        set_leaf(index, tmp_leaf_value);
    }
}

void Node::set_leaf(int index, QVector3D *tmp_leaf_value)
{
    if (index == -2)
    {
        this->is_empty = false;
        this->is_leaf = true;
        this->leaf_value = tmp_leaf_value;
    }
    else if (index == -1)
    {
        return;
    }
    else
    {
        this->children[index]->is_empty = false;
        this->children[index]->is_leaf = true;
        this->children[index]->leaf_value = tmp_leaf_value;
    }
}

void Node::set_leaf_2(QVector3D *tmp_leaf_value)
{
    this->is_empty = false;
    this->is_leaf = true;
    this->leaf_value = tmp_leaf_value;
}

int Node::get_index(QVector3D val)
{
    if (near_bot_left.z() <= val.z() && near_bot_left.z() + length > val.z())
    {
        return get_index_0_3(val);
    }
    else if (far_top_right.z() - length <= val.z() && far_top_right.z() >= val.z())
    {
        return get_index_4_7(val);
    }
    else
    {
        return -1;
    }
}

int Node::get_index_0_3(QVector3D val)
{
    if (near_bot_left.x() <= val.x() && near_bot_left.x() + length > val.x())
    {
        if (near_bot_left.y() <= val.y() && near_bot_left.y() + length > val.y())
        {
            return 2;
        }
        else if (far_top_right.y() - length <= val.y() && far_top_right.y() >= val.y())
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    else if (far_top_right.x() - length <= val.x() && far_top_right.x() >= val.x())
    {
        if (near_bot_left.y() <= val.y() && near_bot_left.y() + length > val.y())
        {
            return 3;
        }
        else if (far_top_right.y() - length <= val.y() && far_top_right.y() >= val.y())
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

int Node::get_index_4_7(QVector3D val)
{
    if (near_bot_left.x() <= val.x() && near_bot_left.x() + length > val.x())
    {
        if (near_bot_left.y() <= val.y() && near_bot_left.y() + length > val.y())
        {
            return 6;
        }
        else if (far_top_right.y() - length <= val.y() && far_top_right.y() >= val.y())
        {
            return 5;
        }
        else
        {
            return -1;
        }
    }
    else if (far_top_right.x() - length <= val.x() && far_top_right.x() >= val.x())
    {
        if (near_bot_left.y() <= val.y() && near_bot_left.y() + length > val.y())
        {
            return 7;
        }
        else if (far_top_right.y() - length <= val.y() && far_top_right.y() >= val.y())
        {
            return 4;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

QVector3D Node::get_value()
{
    QVector3D tmp = *leaf_value;
    return tmp;
};

