#ifndef NODE_H
#define NODE_H

# include "QVector3D"
# include "QColor"

class Node
{
    public:
        Node *children[8];
        QVector3D far_top_right;
        QVector3D near_bot_left;
        float length;
        bool is_empty;
        // if leaf
        bool is_leaf;
        QVector3D *leaf_value;

        //functions
        Node(QVector3D new_near_bot_left, QVector3D new_far_top_right, float new_length);
        void split();
        void set_leaf(int index, QVector3D *tmp_leaf_value);
        void set_leaf_2(QVector3D *tmp_leaf_value);
        int get_index(QVector3D val);
        int get_index_0_3(QVector3D val);
        int get_index_4_7(QVector3D val);
        QVector3D get_value();

};

#endif // NODE_H
