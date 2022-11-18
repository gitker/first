
#include <stdio.h>
#include <vector>
#include <string>

using namespace std;

struct TreeNode
{
    int idx;
    char c;
    TreeNode *first_child;
    TreeNode *sibling;

    TreeNode()
    {
        idx = 0;
        c = 0;
        first_child = sibling = NULL;
    }

    TreeNode *find_child(char c)
    {
        TreeNode *p = first_child;
        while (p != NULL)
        {
            if (p->c == c)
            {
                return p;
            }
            p = p->sibling;
        }
        return NULL;
    }
    TreeNode *add_child(char c, int &count)
    {
        TreeNode *new_node = new TreeNode();
        count++;
        new_node->c = c;
        new_node->idx = count;

        if (first_child == NULL)
        {
            first_child = new_node;
        }
        else
        {
            new_node->sibling = first_child;
            first_child = new_node;
        }
        return new_node;
    }
};

struct Pair
{
    int idx;
    char c;
    Pair(int a, char b)
    {
        idx = a;
        c = b;
    }
};

struct Encoder
{
    TreeNode *root;
    int count;
    vector<Pair> res;
    TreeNode *loc;
    Encoder()
    {
        root = new TreeNode();
        count = 0;
        loc = root;
    }
    int encode(char c)
    {
        TreeNode *node;

        node = loc->find_child(c);
        if (node == NULL)
        {
            Pair p(loc->idx, c);
            loc->add_child(c, count);
            res.push_back(p);
            loc = root;
        }
        else
        {
            loc = node;
        }
        return 0;
    }
    void dump_res()
    {
        for (int i = 0; i < res.size(); i++)
        {
            printf("%d %c\n", res[i].idx, res[i].c);
        }
    }
};

struct Range
{
    int start;
    int end;
    Range(int a, int b)
    {
        start = a;
        end = b;
    }
};

struct Decoder
{

    int count;
    vector<Pair> *res;
    vector<char> bits;
    vector<Range> bmap;
    Decoder(vector<Pair> *t)
    {

        count = 0;
        res = t;
        bmap.push_back(Range(-1, -1));
    }
    int decode()
    {
        for (int i = 0; i < res->size(); i++)
        {
            Pair p = (*res)[i];
            if (p.idx == 0)
            {
                bits.push_back(p.c);
                bmap.push_back(Range(bits.size() - 1, bits.size()));
            }
            else
            {
                int start_idx = bits.size();
                for (int j = bmap[p.idx].start; j < bmap[p.idx].end; j++)
                {
                    bits.push_back(bits[j]);
                }
                bits.push_back(p.c);
                bmap.push_back(Range(start_idx, bits.size()));
            }
        }
        return 0;
    }
    void dump()
    {
        for (int i = 0; i < bits.size(); i++)
        {
            printf("%c ", bits[i]);
        }
        printf("\n");
    }
};

#define BUFSZ 64*1024

char bf[BUFSZ];

int main(int argc,char **argv)
{
    string s = "ABBCBCABABCAABCAAB";
    int total_in=0;
    int totoal_out=0;
    Encoder enc;
    for (int i = 0; i < s.size(); i++)
    {
        enc.encode(s[i]);
    }
    FILE *fp = fopen(argv[1],"r");
    int ret;
    while(1)
    {
        ret = fread(bf,1,BUFSZ,fp);
        if(ret<=0)
        {
            break;
        }
        total_in+=ret;
        Encoder *enc = new Encoder();
        for(int i=0;i<ret;i++)
        {
            enc->encode(bf[i]);
        }
        totoal_out+=enc->res.size()*3;
    }
    printf("int %d out %d\n",total_in,totoal_out);
    

    return 0;
}