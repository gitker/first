

#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <iostream>
#include <vector>
using namespace std;

int freqs[256];

struct BTree
{
    char c;
    int freq;
    BTree *left;
    BTree *right;
    BTree *parent;
    BTree(char a, int b)
    {
        c = a;
        freq = b;
        left = right = parent = NULL;
    }
    BTree *merge(BTree *a, BTree *b)
    {
        BTree *c = new BTree(0, a->freq + b->freq);
        c->left = a;
        c->right = b;
        a->parent = c;
        b->parent = c;
        return c;
    }
};

struct cmp
{
    bool operator()(BTree *a, BTree *b)
    { //默认是less函数

        return (a->freq > b->freq);
    }
};

struct HufNode
{
    vector<int> hufcode;
};

struct Encoder
{
    int freqs[256];

    BTree *q[256];

    HufNode nodes[256];
    vector<unsigned char> output;

    unsigned char literal;
    int peding_bits;

    Encoder()
    {
        literal = 0;
        peding_bits = 0;
    }

    void generate_hufmancode(BTree *leaf, HufNode *node, int c)
    {

        BTree *parent = leaf->parent;
        while (parent != NULL)
        {
            if (parent->left == leaf)
            {
                node->hufcode.push_back(0);
            }
            else
            {
                node->hufcode.push_back(1);
            }
            leaf = parent;
            parent = parent->parent;
        }
        int vsz = node->hufcode.size();
        for (int i = 0; i < vsz - 1 - i; i++)
        {
            int tmp = node->hufcode[i];
            node->hufcode[i] = node->hufcode[vsz - 1 - i];
            node->hufcode[vsz - 1 - i] = tmp;
        }
    }

    int build_up_tree()
    {
        priority_queue<BTree *, vector<BTree *>, cmp> d;
        for (int i = 0; i < 256; i++)
        {
            q[i] = new BTree(i, freqs[i]);
            d.push(q[i]);
        }

        while (d.size() >= 2)
        {
            BTree *a = d.top();
            d.pop();
            BTree *b = d.top();
            d.pop();
            BTree *c = a->merge(a, b);
            d.push(c);
        }
        d.pop();
        int total=0;
        for (int i = 0; i < 256; i++)
        {
            generate_hufmancode(q[i], nodes + i, i);
            total+=nodes[i].hufcode.size();
        }
        printf("total size %d\n",total);

        return 0;
    }
    int push_bit(int v)
    {
        literal = (literal << 1) | (v);
        peding_bits++;
        if (peding_bits == 8)
        {
            output.push_back(literal);
            literal = 0;
            peding_bits = 0;
        }
        return 0;
    }
    int encode(unsigned char c)
    {
        int idx = (int)c;
        for (int i = 0; i < nodes[idx].hufcode.size(); i++)
        {
            push_bit(nodes[idx].hufcode[i]);
        }

        return 0;
    }
    void flush()
    {
        if (peding_bits != 0)
        {
            for (int i = 0; i < 8; i++)
            {
                push_bit(0);
            }
        }
    }
};

struct Decoder
{
    HufNode *nodes;
    vector<unsigned char> *output;
    int pos;
    unsigned char literal;
    int bits;
    BTree *root;

    Decoder(HufNode *n, vector<unsigned char> *o)
    {
        pos = 0;
        literal = 0;
        bits = 0;
        nodes = n;
        output = o;
    }

    void push_node(BTree *root, HufNode *nd, char c)
    {
        for (int i = 0; i < nd->hufcode.size(); i++)
        {
            if (nd->hufcode[i] == 0)
            {
                if (root->left == NULL)
                {
                    root->left = new BTree((char)0, 0);
                }
                root = root->left;
            }
            else
            {
                if (root->right == NULL)
                {
                    root->right = new BTree((char)0, 0);
                }
                root = root->right;
            }
        }
        root->c = c;
    }

    int build_tree()
    {
        root = new BTree(0, 0);
        for (int i = 0; i < 256; i++)
        {
            push_node(root, nodes + i, i);
        }

        return 0;
    }
    int get_bit()
    {
        if (bits == 0)
        {
            if (pos >= output->size())
            {
                return -1;
            }
            literal = (*output)[pos];
            pos++;
        }
        int ret = (literal >> (7 - bits)) & 1;
        bits++;
        if (bits == 8)
        {
            bits = 0;
        }
        return ret;
    }
    char decode()
    {
        BTree *top = root;
        while (top->left != NULL || top->right != NULL)
        {
            int ret = get_bit();
            if (ret < 0)
            {
                return -1;
            }
            if (ret == 0)
            {
                top = top->left;
            }
            else
            {
                top = top->right;
            }
        }

        return top->c;
    }
};

#define MAX 0x1000000

unsigned char buf[MAX];

int main(int argc, char **argv)
{
    Encoder enc;
    FILE *fp = fopen(argv[1], "r");
    int ret = fread(buf, 1, MAX, fp);
    printf("in size %d\n", ret);
    for (int i = 0; i < ret; i++)
    {
        enc.freqs[buf[i]]++;
    }
    enc.build_up_tree();
    for (int i = 0; i < ret; i++)
    {
        enc.encode(buf[i]);
    }
    enc.flush();
    printf("enc size %d\n", enc.output.size());
    Decoder dec(enc.nodes, &enc.output);
    dec.build_tree();
    for (int i = 0; i < ret; i++)
    {
        char t = dec.decode();
        if (t != (char)buf[i])
        {
            printf("shitt %d %d %d\n", i, buf[i], t);
        }
    }
    printf("\n");
    return 0;
}