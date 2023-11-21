#ifndef TREE_H
#define TREE_H

#include <cassert>
#include <cstdlib>
#include <cstdio>

enum Way
{
    LEFT,
    RIGHT
};

struct Node
{
    Node* parent;
    Node* right;
    Node* left;

    char* name;
};

Node* CreateNode (Node* parent, Way mode);
void  TreeDtor   (Node* node);
void  TreeDump   (Node* node);
void  PrintTree  (Node* node, FILE* file);

#endif
