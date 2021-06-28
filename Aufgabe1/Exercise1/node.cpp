#include <Node.h>
#include <QVector3D>

Node::Node(QVector3D new_near_bot_left, QVector3D new_far_top_right, float new_length)
{
    near_bot_left = new_near_bot_left;
    far_top_right = new_far_top_right;
    length = new_length;
    is_leaf = false;
    is_set = false;
}

void Node::split()
{
    float new_length = length/2;

    //NE_1
    children[0] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y() + new_length, near_bot_left.z()),
                           QVector3D(far_top_right.x(), far_top_right.y(), far_top_right.z() - new_length), new_length);

    //NW_1
    children[1] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y() + new_length, near_bot_left.z()),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y(), far_top_right.z() - new_length),new_length);
    //SW_1
    children[2] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y(), near_bot_left.z()),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y() - new_length, far_top_right.z() - new_length), new_length);
    //SE_1
    children[3] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y(), near_bot_left.z()),
                           QVector3D(far_top_right.x(), far_top_right.y() - new_length, far_top_right.z() - new_length), new_length);
    //NE_2
    children[4] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y() + new_length, near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x(), far_top_right.y(), far_top_right.z()), new_length);
    //NW_2
    children[5] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y() + new_length, near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y(), far_top_right.z()), new_length);
    //SW_2
    children[6] = new Node(QVector3D(near_bot_left.x(), near_bot_left.y(), near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x() - new_length, far_top_right.y() - new_length, far_top_right.z()), new_length);
    //SE_2
    children[7] = new Node(QVector3D(near_bot_left.x() + new_length, near_bot_left.y(), near_bot_left.z() + new_length),
                           QVector3D(far_top_right.x(), far_top_right.y() - new_length, far_top_right.z()), new_length);

    // remove leafe attribute
    this->is_leaf = false;
}

void Node::set_leaf(QVector3D tmp_leaf_value)
{
    this->is_set = true;
    this->is_leaf = true;
    this->leaf_value = tmp_leaf_value;
}

int Node::get_index(QVector3D val)
{
    //printf("%f, %f, %f \n", val.x(), val.y(), val.z());
    int index = 0;
    for (Node * new_node: this->children)
    {
        Node tmp = *new_node;
        if (       val.x() >= (tmp.near_bot_left).x()
                && val.x() <= tmp.far_top_right.x()
                && val.y() >= tmp.near_bot_left.y()
                && val.y() <= tmp.far_top_right.y()
                && val.z() >= tmp.near_bot_left.z()
                && val.z() <= tmp.far_top_right.z())
        {
            //printf("index %d\n", index);
            return index;
        }
        index = index + 1;
    }
    if (index == 8)
    {
        return -1;
    }
}

QVector3D Node::get_value()
{
    QVector3D tmp = this->leaf_value;
    return tmp;
}

bool Node::insert_point(QVector3D point, int depth)
{
    //is in boundingBox?
    if (    !( point.x() >= (this->near_bot_left).x()
            && point.x() <= this->far_top_right.x()
            && point.y() >= this->near_bot_left.y()
            && point.y() <= this->far_top_right.y()
            && point.z() >= this->near_bot_left.z()
            && point.z() <= this->far_top_right.z()))
    {
        return false;
    }

    if (depth == -1) {
        return false;
    }

    // handle leaf
    if (this->is_leaf) {

        // if in leafe return
        QVector3D tmp = this->get_value();
        // if same Point as its node Value, return.
        if (tmp.x() == point.x() && tmp.y() == point.y() && tmp.z() == point.z())
        {
            return true;
        }

        //else split
        this->split();
        this->leaf_value = QVector3D();
        //printf("reset point\n ");
        int index = this->get_index(tmp);
        this->children[index]->set_leaf(tmp);
    }

    // handle empty node
    if (!this->is_set)
    {
        this->set_leaf(point);
        return true;
    }

    // get index for new fitting node
    int index = this->get_index(point);
    if (index > -1)
    {
        return this->children[index]->insert_point(point, depth - 1);
    }
    else
    {
        printf("index out of bounds\n");
        printf("%f, %f, %f \n", point.x(), point.y(), point.z());
    }
    return false;
}

