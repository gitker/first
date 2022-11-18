
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>

using namespace std;

const uint32_t kTopMask = (1 << 22);

struct Encoder
{

    uint32_t Low;
    uint32_t Range;
    uint32_t _cacheSize;
    uint32_t _cache;

    vector<uint32_t> res;

    void Init()
    {

        Low = 0;
        Range = -1;
        _cacheSize = 1;
        _cache = 0;
    }

    void FlushData()
    {
        for (int i = 0; i < 5; i++)
            ShiftLow();
    }

    void ShiftLow()
    {

        if (Low < 0xFF000000L)
        {

            uint8_t temp = _cache;

            do
            {
                res.push_back(temp);
                temp = 0xFF;
            } while (--_cacheSize != 0);
            _cache = Low>> 24;
        }
        _cacheSize++;
        Low = Low << 8;
    }

    void Encode(uint32_t prob, int symbol)
    {
        uint32_t newBound = Range / 256 * prob;
        int carry = 0;
        if (symbol == 0)
        {
            Range = newBound;
        }
        else
        {
            carry = (Low + newBound) < Low;
            Low += newBound;
            Range -= newBound;
        }
        if (carry)
        {
            _cache++;
            while (--_cacheSize)
            { 
                res.push_back(_cache);
               _cache = 0x00;
            }
            ++_cacheSize;
        }

        if (Range < kTopMask)
        {
            Range <<= 8;
            ShiftLow();
        }
    }
};

struct Decoder
{

    uint32_t Range;
    uint32_t Code;
    vector<uint32_t> *res;
    int idx;

    uint32_t read_char()
    {
        uint32_t v = (*res)[idx];
        idx++;
        return v;
    }

    void Init(vector<uint32_t> *t)
    {
        Code = 0;
        Range = -1;
        idx = 0;
        res = t;

        for (int i = 0; i < 5; i++)
            Code = (Code << 8) | read_char();
    }

    int DecodeBit(uint32_t prob)
    {

        int newBound = Range / 256 * prob;
        int ret = 0;
        if (Code < newBound)
        {
            Range = newBound;
        }
        else
        {
            Range -= newBound;
            Code -= newBound;
            ret = 1;
        }
        if (Range < kTopMask)
        {
            Code = (Code << 8) | read_char();
            Range <<= 8;
        }
        return ret;
    }
};

#define MAX 100000

int xx[MAX];

int main()
{
    int notmatch = 0;
    for (int i = 0; i < MAX; i++)
    {
        if ((rand() % 20) != 0)
        {
            xx[i] = 0;
        }
        else
        {
            xx[i] = 1;
        }
    }
    Encoder enc;
    enc.Init();

    for (int i = 0; i < MAX; i++)
    {
        enc.Encode(240, xx[i]);
    }
    enc.FlushData();

    printf("enc size %d\n", enc.res.size());

    Decoder dec;
    dec.Init(&enc.res);

    for (int i = 0; i < MAX; i++)
    {
        int ret = dec.DecodeBit(240);
        if (ret != xx[i])
        {
            notmatch++;
        }
    }
    printf("not match %d\n", notmatch);
}