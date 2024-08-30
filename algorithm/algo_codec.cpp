

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string.h>
#include <stdint.h>

using namespace std;

#define IMAX 4294967296

#define N 4096

struct Float
{
    vector<uint64_t> digits;
    int leading_zeros;
    double to_float()
    {
        double res = 0;
        double kim = IMAX;
        for (int i = 0; i < digits.size(); i++)
        {
            if (digits[i] != 0)
            {
                res += digits[i] / kim;
            }
            kim *= IMAX;
        }
        int ll = leading_zeros;
        while (ll > 0)
        {
            res = res / IMAX;
            ll--;
        }
        return res;
    }

    Float()
    {
        digits.clear();
        leading_zeros = 0;
    }
    void dump()
    {
        printf("0.");

        for (int i = 0; i < digits.size(); i++)
        {
            printf("%lu ", digits[i]);
        }
        printf("\n");
        printf("%d %d\n", leading_zeros, (int)digits.size());
    }

    Float(const char *str)
    {
        int len = strlen(str);
        int lead = 1;
        leading_zeros = 0;
        digits.clear();
        for (int i = 0; i < len; i++)
        {
            char c = str[i];
            uint64_t val = 0;
            if (c != '0')
            {
                lead = 0;
                val = c - '0';
            }
            else
            {
                val = 0;
            }
            if (lead)
            {
                leading_zeros++;
            }
            else
            {
                digits.push_back(val);
            }
        }
    }
};

struct RNG
{
    Float start;
    Float length;
};

uint64_t get_bit(const Float &a, int idx)
{
    if (idx >= a.leading_zeros + a.digits.size() || idx < a.leading_zeros)
    {
        return 0;
    }
    return a.digits[idx - a.leading_zeros];
}

Float add(Float &a, Float &b)
{
    Float res;
    int alen = a.digits.size() + a.leading_zeros;
    int blen = b.digits.size() + b.leading_zeros;
    if (a.digits.size() == 0)
    {
        return b;
    }
    if (b.digits.size() == 0)
    {
        return a;
    }
    if (alen > blen)
    {
        return add(b, a);
    }

    int start_index = blen - 1;
    int leading_zeros = a.leading_zeros;
    if (a.leading_zeros > b.leading_zeros)
    {
        leading_zeros = b.leading_zeros;
    }
    int end_index = leading_zeros;
    res.leading_zeros = leading_zeros;

    int dlen = start_index - end_index + 1;
    res.digits.resize(dlen);
    int pos = dlen - 1;

    uint64_t c = 0;
    for (int i = start_index; i >= end_index; i--)
    {
        uint64_t tval = get_bit(a, i) + get_bit(b, i) + c;
        if (tval < IMAX)
        {
            c = 0;
        }
        else
        {
            tval -= IMAX;
            c = 1;
        }
        res.digits[pos] = tval;
        pos--;
    }
    if (c > 0 && end_index >= 1)
    {

        res.digits.insert(res.digits.begin(), c);
        res.leading_zeros -= 1;
    }
    int cnt = 0;
    for (int i = res.digits.size() - 1; i >= 0; i--)
    {
        if (res.digits[i] != 0)
        {
            break;
        }
        cnt++;
    }
    while (cnt > 0)
    {
        res.digits.pop_back();
        cnt--;
    }

    return res;
}

void mul(Float &a, uint64_t v)
{
    uint64_t c = 0;

    for (int i = a.digits.size() - 1; i >= 0; i--)
    {
        uint64_t tval = a.digits[i] * v + c;
        uint64_t tt = tval % IMAX;
        c = tval / IMAX;
        a.digits[i] = tt;
    }
    if (c > 0)
    {
        a.digits.insert(a.digits.begin(), c);
        a.leading_zeros--;
    }
    int cnt = 0;
    for (int i = a.digits.size() - 1; i >= 0; i--)
    {
        if (a.digits[i] != 0)
        {
            break;
        }
        cnt++;
    }
    while (cnt > 0)
    {
        a.digits.pop_back();
        cnt--;
    }
}

Float mul(Float &a, Float &b)
{

    Float ori;
    ori.leading_zeros = a.leading_zeros + b.leading_zeros;
    ori.digits = a.digits;
    if (b.digits.size() == 0)
    {
        return b;
    }

    Float res;
    res.digits = a.digits;
    res.leading_zeros = ori.leading_zeros + 1;
    mul(res, b.digits[0]);

    for (int i = 1; i < b.digits.size(); i++)
    {
        Float t;
        if (b.digits[i] == 0)
        {
            continue;
        }

        t.digits = a.digits;
        t.leading_zeros = ori.leading_zeros + i + 1;
        mul(t, b.digits[i]);
        res = add(res, t);
    }
    return res;
}

Float froms2(int a)
{
    Float res;
    if (a == 0)
    {
        return res;
    }
    res.leading_zeros = 0;
    res.digits.push_back(IMAX / N * a);

    return res;
}

int cmp(Float &a, Float &b)
{
    int sa = a.leading_zeros + a.digits.size();
    int sb = b.leading_zeros + b.digits.size();

    if (sa < sb)
    {
        sa = sb;
    }
    for (int i = 0; i < sa; i++)
    {
        uint64_t ba = get_bit(a, i);
        uint64_t bb = get_bit(b, i);
        if (ba < bb)
        {
            return -1;
        }
        if (ba > bb)
        {
            return 1;
        }
    }
    return 0;
}

int check(vector<RNG> &rng, int idx, Float &start, Float &len, Float &rr)
{
    Float sstart = mul(len, rng[idx].start);
    Float mlen = mul(len, rng[idx].length);

    Float cs = add(start, sstart);
    Float ce = add(cs, mlen);

    if (cmp(rr, cs) < 0)
    {
        return -1;
    }
    if (cmp(rr, ce) >= 0)
    {
        return 1;
    }
    start = cs;
    len = mlen;
    return 0;
}

Float get_rr(Float &start, Float &end)
{
    int ss = start.digits.size();
    if (end.digits.size() < ss)
    {
        ss = end.digits.size();
    }

    Float rr;

    rr.leading_zeros = start.leading_zeros;

    for (int i = 0; i < ss; i++)
    {
        if (start.digits[i] != end.digits[i])
        {
            if (i != end.digits.size() - 1)
            {
                rr.digits.push_back(end.digits[i]);
                break;
            }
            else
            {
                if (end.digits[i] - start.digits[i] == 1)
                {
                    if (i == start.digits.size() - 1)
                    {
                        rr.digits.push_back(start.digits[i]);
                        break;
                    }
                    else
                    {
                        rr.digits.push_back(start.digits[i]);
                        i++;
                        while (i < start.digits.size())
                        {
                            if (start.digits[i] != (IMAX - 1))
                            {
                                rr.digits.push_back(start.digits[i] + 1);
                                return rr;
                            }
                            rr.digits.push_back(start.digits[i]);
                            i++;
                        }
                        return rr;
                    }
                }
                else
                {
                    rr.digits.push_back(start.digits[i] + 1);
                    break;
                }
            }
        }
        else
        {
            rr.digits.push_back(start.digits[i]);
        }
    }

    return rr;
}

Float encode(unsigned char data[N], RNG rng[])
{
    unsigned int dict[256] = {0};

    int length = 0;
    for (int i = 0; i < N; i++)
    {
        dict[data[i]]++;
    }
    for (int i = 0; i < 256; i++)
    {
        int ll = dict[i];
        int start = length;

        rng[i].start = froms2(start);
        rng[i].length = froms2(ll);
        length += ll;
    }

    Float start;
    Float len;

    start.digits.push_back(IMAX / 2);
    len.digits.push_back(IMAX / 4);

    for (int i = 0; i < N; i++)
    {
        int k = data[i];

        Float sstart = mul(len, rng[k].start);

        start = add(start, sstart);

        len = mul(len, rng[k].length);
    }

    Float end = add(start, len);

    return get_rr(start, end);
}

void decode(RNG rng[], Float &rr, unsigned char res[N])
{
    Float sstart;
    Float len;
    sstart.digits.push_back(IMAX / 2);
    len.digits.push_back(IMAX / 4);

    vector<RNG> rn;
    vector<int> idxs;

    for (int i = 0; i < 256; i++)
    {
        if (rng[i].length.digits.size() != 0)
        {
            rn.push_back(rng[i]);
            idxs.push_back(i);
        }
    }

    for (int i = 0; i < N; i++)
    {
        int start = 0;
        int end = rn.size() - 1;
        int mid;
        while (start <= end)
        {
            mid = (start + end) / 2;
            int ret = check(rn, mid, sstart, len, rr);
           
            if (ret < 0)
            {
                end = mid - 1;
            }
            else if (ret > 0)
            {
                start = mid + 1;
            }
            else
            {
                
                break;
            }
        }
        res[i] = idxs[mid];
    }
}

#include <time.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    FILE *fp = fopen(argv[1], "rb");
    unsigned char dt[N] = {0};
    unsigned char res[N] = {0};

    RNG rng[256];
    Float rr;

    while (1)
    {
        int ret = fread(dt, 1, N, fp);
        if (ret != N)
        {
            break;
        }
        clock_t t1 = clock();
        rr = encode(dt, rng);
        printf("enc time %d  compress ratio %d%%\n", (int)(clock() - t1), (int)(100*rr.digits.size()*32 /(N*8)));
        break;
    }
    
    clock_t t1 = clock();
    decode(rng, rr, res);
    printf("dec time %d\n", (int)(clock() - t1));
    printf("\n\n");
    if (memcmp(dt, res, N) == 0)
    {
        printf("decode ok\n");
    }
    else
    {
        printf("decode error\n");
    }

    return 0;
}