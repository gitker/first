
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <vector>

using namespace std;

#define NILL 0
#define RED 1
#define BLACK 2

typedef struct _rbnode
{
    int key;
    int val;
    int color;
    struct _rbnode *left;
    struct _rbnode *right;
    struct _rbnode *parent;
} rbnode;

typedef struct _rbtree
{
    rbnode *head;
    int count;
} rbtree;

void init_node(rbtree *tree, rbnode *p, int k, int v, int color)
{
    p->color = color;
    p->key = k;
    p->val = v;
    p->left = (rbnode *)malloc(sizeof(rbnode));
    p->right = (rbnode *)malloc(sizeof(rbnode));
    memset(p->left, 0, sizeof(rbnode));
    memset(p->right, 0, sizeof(rbnode));
    p->left->parent = p;
    p->right->parent = p;
    tree->count++;
}

void free_node(rbtree *tree, rbnode *current)
{
    free(current->left);
    free(current->right);
    current->left = current->right = NULL;
    current->color = NILL;
    tree->count--;
}

rbtree *create()
{
    rbtree *ptr = (rbtree *)malloc(sizeof(rbtree));

    ptr->head = (rbnode *)malloc(sizeof(rbnode));
    ptr->count = 0;
    memset(ptr->head, 0, sizeof(rbnode));

    return ptr;
}

void set_left_child(rbnode *parent, rbnode *child)
{

    parent->left = child;

    if (child != NULL)
        child->parent = parent;
}

void set_right_child(rbnode *parent, rbnode *child)
{
    parent->right = child;
    if (child != NULL)
        child->parent = parent;
}

void left_rotate(rbtree *tree, rbnode *p)
{
    rbnode *child = p->right;
    rbnode *parent = p->parent;

    set_right_child(p, child->left);
    set_left_child(child, p);

    child->parent = parent;

    if (parent != NULL)
    {
        if (parent->left == p)
        {
            parent->left = child;
        }
        else
        {
            parent->right = child;
        }
    }
    else
    {
        tree->head = child;
    }
}

void right_rotate(rbtree *tree, rbnode *p)
{
    rbnode *child = p->left;
    rbnode *parent = p->parent;

    set_left_child(p, child->right);
    set_right_child(child, p);
    child->parent = parent;
    if (parent != NULL)
    {
        if (parent->left == p)
        {
            parent->left = child;
        }
        else
        {
            parent->right = child;
        }
    }
    else
    {
        tree->head = child;
    }
}

rbnode *find(rbtree *tree, int key)
{
    rbnode *current = tree->head;
    if (tree->head->color == NILL)
    {
        return NULL;
    }
    while (current->color != NILL)
    {
        if (key == current->key)
        {

            return current;
        }
        if (key < current->key)
        {
            current = current->left;
        }
        else
        {
            current = current->right;
        }
    }
    return NULL;
}

int insert(rbtree *tree, int key, int val)
{
    rbnode *current = tree->head;

    rbnode *parent;
    rbnode *grandparent;

    if (tree->head->color == NILL)
    {
        init_node(tree, tree->head, key, val, BLACK);
        return 0;
    }

    while (current->color != NILL)
    {
        if (key == current->key)
        {
            current->val = val;
            return 0;
        }
        if (key < current->key)
        {
            current = current->left;
        }
        else
        {
            current = current->right;
        }
    }

    init_node(tree, current, key, val, RED);

again:
    if (current->parent == NULL)
    {
        tree->head = current;
        current->color = BLACK;
        return 0;
    }
    if (current->parent->color == BLACK)
    {
        return 0;
    }

    parent = current->parent;
    grandparent = parent->parent;
    if (grandparent->left->color == RED && grandparent->right->color == RED)
    {
        grandparent->color = RED;
        grandparent->left->color = BLACK;
        grandparent->right->color = BLACK;
        current = grandparent;
        goto again;
    }

    if (grandparent->left == parent)
    {

        if (parent->right == current)
        {

            set_right_child(parent, current->left);
            set_left_child(current, parent);
            set_left_child(grandparent, current);

            current = parent;
            parent = current->parent;
            grandparent = parent->parent;
        }
        parent->color = BLACK;
        grandparent->color = RED;
        right_rotate(tree, grandparent);
    }
    else
    {
        if (parent->left == current)
        {

            set_left_child(parent, current->right);
            set_right_child(current, parent);
            set_right_child(grandparent, current);
            current = parent;
            parent = current->parent;
            grandparent = parent->parent;
        }
        parent->color = BLACK;
        grandparent->color = RED;
        left_rotate(tree, grandparent);
    }

    return 0;
}

void swap_data(rbnode *a, rbnode *b)
{
    int k = a->key;
    int v = a->val;
    a->key = b->key;
    a->val = b->val;
    b->key = k;
    b->val = v;
}

rbnode *next_node(rbnode *current)
{
    if (current->right->color == NILL)
    {
        if (current->parent == NULL)
        {
            return NULL;
        }
        if (current->parent->left == current)
        {
            return current->parent;
        }
        while (current->parent != NULL && current->parent->right == current)
        {
            current = current->parent;
        }
        return current->parent;
    }
    else
    {
        current = current->right;
        while(current->left->color !=NILL)
        {
            current = current->left;
        }
        return current;
    }
    return NULL;
}

rbnode *rb_begin(rbtree *tree)
{
    rbnode *current = tree->head;
    if (current->color == NILL)
    {
        return NULL;
    }
    while (current->left->color != NILL)
    {
        current = current->left;
    }
    return current;
}


int mark_node_red(rbtree *tree, rbnode *current)
{
    rbnode *parent;

again:
    if (current->color == RED)
    {
        return 0;
    }
    parent = current->parent;
    if (parent == NULL)
    {
        current->color = RED;
        return 0;
    }

    if (parent->left == current)
    {
        rbnode *brother = parent->right;
        if (brother->color == RED)
        {
            parent->color = RED;
            brother->color = BLACK;
            left_rotate(tree, parent);
            goto again;
        }
        else
        {
            if (brother->right->color == RED)
            {
                brother->color = parent->color;
                parent->color = BLACK;
                brother->right->color = BLACK;
                left_rotate(tree, parent);
                current->color = RED;
                return 0;
            }
            else
            {
                if (brother->left->color == RED)
                {
                    brother->color = RED;
                    brother->left->color = BLACK;
                    right_rotate(tree, brother);
                    goto again;
                }
                else
                {
                    brother->color = RED;
                    if (parent->color == RED)
                    {
                        parent->color = BLACK;
                        current->color = RED;
                        return 0;
                    }
                    else
                    {
                        mark_node_red(tree, parent);
                        parent->color = BLACK;
                        current->color = RED;
                        return 0;
                    }
                }
            }
        }
    }
    else
    {
        rbnode *brother = parent->left;
        if (brother->color == RED)
        {
            parent->color = RED;
            brother->color = BLACK;
            right_rotate(tree, parent);
            goto again;
        }
        else
        {
            if (brother->left->color == RED)
            {
                brother->color = parent->color;
                parent->color = BLACK;
                brother->left->color = BLACK;
                right_rotate(tree, parent);
                current->color = RED;
                return 0;
            }
            else
            {
                if (brother->right->color == RED)
                {
                    brother->color = RED;
                    brother->right->color = BLACK;
                    left_rotate(tree, brother);
                    goto again;
                }
                else
                {
                    brother->color = RED;
                    if (parent->color == RED)
                    {
                        parent->color = BLACK;
                        current->color = RED;
                        return 0;
                    }
                    else
                    {
                        mark_node_red(tree, parent);
                        parent->color = BLACK;
                        current->color = RED;
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

int remove(rbtree *tree, int key)
{
    rbnode *current ;
    
    current = find(tree,key);

    if (current == NULL)
    {
        printf("can not find item\n");
        return 0;
    }
find_bottom:

    if (current->left->color == NILL && current->right->color == NILL)
    {
        goto reblance;
    }
    if (current->left->color != NILL && current->right->color != NILL)
    {
        rbnode *hou = next_node(current);

        swap_data(current, hou);
        current = hou;
        goto find_bottom;
    }
    if (current->left->color != NILL)
    {

        swap_data(current, current->left);
        current = current->left;
        goto find_bottom;
    }
    if (current->right->color != NILL)
    {
        swap_data(current, current->right);
        current = current->right;
        goto find_bottom;
    }

reblance:
    mark_node_red(tree, current);
    free_node(tree, current);

    return 0;
}

int get_black_cnt(rbnode *p)
{
    int cnt = 0;
    while (p != NULL)
    {
        if (p->color == BLACK)
        {
            cnt++;
        }
        p = p->parent;
    }
    return cnt;
}

int bfs(rbtree *root)
{
    queue<rbnode *> q;
    vector<rbnode *> leaf;
    int cnt = 0;

    int high = -1;

    q.push(root->head);
    cnt++;
    if (root->head->left == NULL)
    {
        return 0;
    }
    while (!q.empty())
    {
        rbnode *a = q.front();
        if (a->color != NILL)
        {
        }
        q.pop();
        if (a->left->color != NILL)
        {
            q.push(a->left);
            cnt++;
        }
        else
        {
            leaf.push_back(a->left);
        }
        if (a->right->color != NILL)
        {
            q.push(a->right);
            cnt++;
        }
        else
        {
            leaf.push_back(a->right);
        }
    }

    for (int i = 0; i < leaf.size(); i++)
    {
        if (high == -1)
        {
            high = get_black_cnt(leaf[i]);
        }
        else
        {
            if (get_black_cnt(leaf[i]) != high)
            {
                printf("fuck error\n");
                exit(0);
            }
        }
    }
    return 1;
}

#define N 100

#include <time.h>

void shuffle(int *ary, int size)
{
    int i;
    for (i = size - 1; i >= 0; i--)
    {
        int n = i + 1;
        int idx = rand() % n;
        int tmp = ary[i];
        ary[i] = ary[idx];
        ary[idx] = tmp;
    }
}

int ary[N];

void dfs(rbtree *tree)
{
    rbnode *start = rb_begin(tree);
    while (start != NULL)
    {
        printf("%d\n", start->key);
        start = next_node(start);
    }
}

void test(rbtree *t)
{
    shuffle(ary, N);
    for (int i = 0; i < N; i++)
    {
        insert(t, ary[i], i);
    }
    

  
    shuffle(ary, N);

    for (int i = 0; i < N; i++)
    {
        int c = t->count;
        remove(t, ary[i]);
        bfs(t);
    }
  
}

#include <unistd.h>

int main()
{
    rbtree *t = create();
    int cnt = 0;

    srand(time(0));

    for (int i = 0; i < N; i++)
    {

        ary[i] = i;
    }

    while (1)
    {
        test(t);
        usleep(100 * 1000);
        cnt++;
    }

    //  bfs(t);
}