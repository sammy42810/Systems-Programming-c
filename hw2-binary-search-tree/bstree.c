/*******************************************************************************
 * Name        : bstree.c
 * Author      : Samantha Bryan
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include "bstree.h"
#include <stdlib.h>
#include <string.h>

static void destroy_node(node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    destroy_node(node->left);
    destroy_node(node->right);
    free(node->data);
    free(node);
}

void add_node(void *data, size_t size, tree_t *tree, int (*cmp_func)(void *, void *))
{
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL)
    {
        return;
    }

    new_node->data = malloc(size);
    if (new_node->data == NULL)
    {
        free(new_node);
        return;
    }

    unsigned char *dst = (unsigned char *)new_node->data;
    unsigned char *src = (unsigned char *)data;
    for (size_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }

    new_node->left = NULL;
    new_node->right = NULL;

    if (tree->root == NULL)
    {
        tree->root = new_node;
        return;
    }

    node_t *current = tree->root;
    while (1)
    {
        int cmp = cmp_func(data, current->data);

        if (cmp < 0)
        {
            if (current->left == NULL)
            {
                current->left = new_node;
                return;
            }
            current = current->left;
        }
        else
        {
            if (current->right == NULL)
            {
                current->right = new_node;
                return;
            }
            current = current->right;
        }
    }
}

void print_tree(node_t *node, void (*print_func)(void *))
{
    if (node == NULL)
    {
        return;
    }

    print_tree(node->left, print_func);
    print_func(node->data);
    print_tree(node->right, print_func);
}

void destroy(tree_t *tree)
{
    if (tree == NULL)
    {
        return;
    }

    destroy_node(tree->root);
    tree->root = NULL;
}
