

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstdarg>
#include <time.h>
#include <iostream>

using namespace std;

// a^0 = 1
// a^1 = 2
// a^2 = 4
// a^8 = 29
int vtable[] = {1, 2, 4, 8, 16, 32, 64, 128, 29, 58, 116, 232, 205, 135, 19, 38, 76, 152,
                45, 90, 180, 117, 234, 201, 143, 3, 6, 12, 24, 48, 96, 192, 157, 39, 78, 156,
                37, 74, 148, 53, 106, 212, 181, 119, 238, 193, 159, 35, 70, 140, 5, 10, 20, 40, 80, 160, 93, 186, 105, 210,
                185, 111, 222, 161, 95, 190, 97, 194, 153, 47, 94, 188, 101, 202, 137, 15, 30, 60, 120, 240, 253, 231, 211,
                187, 107, 214, 177, 127, 254, 225, 223, 163, 91, 182, 113, 226, 217, 175, 67, 134, 17, 34, 68, 136, 13, 26, 52, 104, 208,
                189, 103, 206, 129, 31, 62, 124, 248, 237, 199, 147, 59, 118, 236, 197, 151, 51, 102, 204, 133, 23, 46, 92, 184, 109, 218, 169, 79,
                158, 33, 66, 132, 21, 42, 84, 168, 77, 154, 41, 82, 164, 85, 170, 73, 146, 57, 114, 228, 213, 183, 115, 230, 209, 191, 99, 198, 145, 63,
                126, 252, 229, 215, 179, 123, 246, 241, 255, 227, 219, 171, 75, 150, 49, 98, 196, 149, 55, 110, 220, 165, 87, 174, 65, 130, 25, 50, 100,
                200, 141, 7, 14, 28, 56, 112, 224, 221, 167, 83, 166, 81, 162, 89, 178, 121, 242, 249, 239, 195, 155, 43, 86, 172, 69, 138, 9, 18, 36, 72, 144, 61, 122,
                244, 245, 247, 243, 251, 235, 203, 139, 11, 22, 44, 88, 176, 125, 250, 233, 207, 131, 27, 54, 108, 216, 173, 71, 142, 1};

// value to idx
int ftable[] = {0, 0, 1, 25, 2, 50, 26, 198, 3, 223, 51, 238, 27, 104, 199, 75, 4, 100, 224, 14, 52, 141, 239, 129, 28, 193, 105, 248, 200, 8,
                76, 113, 5, 138, 101, 47, 225, 36, 15, 33, 53, 147, 142, 218, 240, 18, 130, 69, 29, 181, 194, 125, 106, 39, 249, 185, 201, 154, 9, 120, 77,
                228, 114, 166, 6, 191, 139, 98, 102, 221, 48, 253, 226, 152, 37, 179, 16, 145, 34, 136, 54, 208, 148, 206, 143, 150, 219, 189, 241, 210, 19,
                92, 131, 56, 70, 64, 30, 66, 182, 163, 195, 72, 126, 110, 107, 58, 40, 84, 250, 133, 186, 61, 202, 94, 155, 159, 10, 21, 121, 43, 78, 212, 229,
                172, 115, 243, 167, 87, 7, 112, 192, 247, 140, 128, 99, 13, 103, 74, 222, 237, 49, 197, 254, 24, 227, 165, 153, 119, 38, 184, 180, 124, 17, 68,
                146, 217, 35, 32, 137, 46, 55, 63, 209, 91, 149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190, 97, 242, 86, 211, 171, 20, 42, 93, 158, 132,
                60, 57, 83, 71, 109, 65, 162, 31, 45, 67, 216, 183, 123, 164, 118, 196, 23, 73, 236, 127, 12, 111, 246, 108, 161, 59, 82, 41, 157, 85, 170, 251,
                96, 134, 177, 187, 204, 62, 90, 203, 89, 95, 176, 156, 169, 160, 81, 11, 245, 22, 235, 122, 117, 44, 215, 79, 174, 213, 233, 230, 231, 173, 232,
                116, 214, 244, 234, 168, 80, 88, 175};

class node
{
public:
    unsigned char value;

    node(unsigned char c)
    {
        value = c;
    }

    node succ()
    {
        node c(0);
        if ((value & 0x80) == 0)
        {
            c.value = (value << 1);
        }
        else
        {
            c.value = (value << 1) ^ (0b11101);
        }
        return c;
    }

    node &operator+=(const node &b)
    {
        value = (b.value ^ value);
        return *this;
    }

    node &operator-=(const node &b)
    {
        value = (b.value ^ value);
        return *this;
    }

    node operator+(const node &b)
    {
        node c(0);
        c.value = (b.value ^ value);
        return c;
    }
    node operator*(const node &b)
    {
        node c(0);
        if (b.value == 0 || value == 0)
            return c;

        c.value = vtable[(ftable[value] + ftable[b.value]) % 255];

        return c;
    }
    node operator-(const node &b)
    {
        node c(0);
        c.value = (value ^ b.value);
        return c;
    }
    node inv()
    {
        node c(0);

        c.value = vtable[255 - ftable[value]];

        return c;
    }
    node &operator/=(const node &b)
    {
        if (value == 0)
        {
            return *this;
        }
        value = vtable[(ftable[value] + (255 - ftable[b.value])) % 255];
        return *this;
    }
    node operator/(const node &b)
    {
        node c(0);
        if (value == 0)
            return c;

        c.value = vtable[(ftable[value] + (255 - ftable[b.value])) % 255];
        return c;
    }
    node exp(unsigned int b)
    {
        node c(0);
        if (b == 0)
        {
            c.value = 1;
            return c;
        }

        c.value = vtable[((ftable[value] * (b % 255)) % 255)];

        return c;
    }
    node &operator=(const unsigned char &a)
    {
        value = a;
        return *this;
    }
};

class Vector
{
public:
    int length;
    vector<node> val;
    int zeros;

    Vector(int c)
    {
        length = c;
        for (int i = 0; i < c; i++)
        {
            val.push_back(node(0));
        }
        zeros = 0;
    }

    int is_zero()
    {
        for (int i = 0; i < length; i++)
        {
            if (val[i].value != 0)
            {
                return 0;
            }
        }
        return 1;
    }

    void dump()
    {
        for (int i = 0; i < length; i++)
        {
            printf("%d ", val[i].value);
        }
        printf("\n");
    }

    void set_value(int n, ...)
    {
        va_list args;
        va_start(args, n);
        for (int i = 0; i < n; i++)
        {
            val[i] = va_arg(args, unsigned int);
        }
        va_end(args);
    }

    Vector &operator+=(const Vector &b)
    {
        for (int i = b.zeros; i < length; i++)
        {
            val[i] += b.val[i];
        }
        return *this;
    }
    Vector &operator-=(const Vector &b)
    {
        for (int i = b.zeros; i < length; i++)
        {
            val[i] -= b.val[i];
        }
        return *this;
    }

    Vector operator+(const Vector &b)
    {
        Vector res(length);
        int n = length;
        res.length = n;
        for (int i = 0; i < n; i++)
        {
            res[i] = (val[i] + b.val[i]);
        }
        return res;
    }
    Vector operator-(const Vector &b)
    {
        Vector res(length);
        int n = length;
        res.length = n;
        for (int i = 0; i < n; i++)
        {
            res[i] = (val[i] - b.val[i]);
        }
        return res;
    }
    Vector operator*(const node &b)
    {
        Vector res(length);
        int n = length;
        res.length = n;
        for (int i = zeros; i < n; i++)
        {
            res[i] = (val[i] * b);
        }
        return res;
    }

    Vector &operator/=(const node &b)
    {
        for (int i = zeros; i < length; i++)
        {
            val[i] /= b;
        }
        return *this;
    }
    Vector operator/(const node &b)
    {
        Vector res(length);
        int n = length;
        res.length = n;
        for (int i = 0; i < n; i++)
        {
            res[i] = (val[i] / b);
        }
        return res;
    }
    node &operator[](int index)
    {
        return val[index];
    }
    Vector &operator=(const Vector &b)
    {
        for (int i = 0; i < length; i++)
        {
            val[i] = b.val[i];
        }
    }
    node dot(Vector b)
    {
        node c(0);
        for (int i = 0; i < length; i++)
        {
            c = c + val[i] * b[i];
        }
        return c;
    }
    Vector &sub_mul(Vector &b, node &c) // a = a - b*c
    {
        for (int i = b.zeros; i < length; i++)
        {
            val[i] -= b.val[i] * c;
        }
    }
};

class Matric
{
public:
    int m, n;
    vector<Vector> row;
    Matric(int c, int d)
    {
        m = c;
        n = d;
        for (int i = 0; i < m; i++)
        {
            row.push_back(Vector(n));
        }
    }
    Vector colum(int index)
    {
        Vector a(m);
        for (int i = 0; i < m; i++)
        {
            a[i] = row[i][index];
        }
        return a;
    }
    Vector &operator[](int index)
    {
        return row[index];
    }
    Matric operator*(Matric &b)
    {
        Matric c(this->m, b.n);
        for (int i = 0; i < this->m; i++)
        {
            for (int j = 0; j < b.n; j++)
            {
                c[i][j] = row[i].dot(b.colum(j));
            }
        }
        return c;
    }
    Matric operator+(Matric &b)
    {
        Matric c(m, n);
        for (int i = 0; i < m; i++)
        {
            for (int j = 0; j < n; j++)
            {
                c[i][j] = row[i][j] + b[i][j];
            }
        }
        return c;
    }
    Vector operator*(Vector &b)
    {
        Vector c(m);
        for (int i = 0; i < m; i++)
        {
            c[i] = row[i].dot(b);
        }
        return c;
    }

    int inv(Matric &dst)
    {
        Matric &src = *this;
        int n = src.n;
        int start = 0;
        node tmp(0);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (i == j)
                {
                    dst[i][j] = 1;
                }
                else
                {
                    dst[i][j] = 0;
                }
            }
        }

        for (start = 0; start < n; start++)
        {
            int idx = -1;
            for (int i = start; i < n; i++)
            {
                if (src[i][start].value != 0)
                {
                    idx = i;
                    break;
                }
            }
            if (idx < 0)
            {
                return -1;
            }
            if (idx != start)
            {
                src[start] += src[idx];
                dst[start] += dst[idx];
            }

            tmp = src[start][start];
            dst[start] /= (tmp);
            src[start] /= (tmp);
            for (int j = start + 1; j < n; j++)
            {
                if (src[j][start].value != 0)
                {
                    tmp = src[j][start];
                    dst[j] /= (tmp);
                    src[j] /= (tmp);
                    src[j] -= src[start];
                    dst[j] -= dst[start];
                }
                src[j].zeros = start + 1;
            }
        }

        for (start = n - 2; start >= 0; start--)
        {
            for (int i = start + 1; i < n; i++)
            {
                if (src[start][i].value != 0)
                {
                    dst[start].sub_mul(dst[i], src[start][i]);
                    src[start].sub_mul(src[i], src[start][i]);
                }
            }
        }
        return 0;
    }

    int solve_equation(Vector &dst) // A*X = Y
    {
        Matric &src = *this;
        int n = src.n;
        int start = 0;
        node tmp(0);

        for (start = 0; start < n; start++)
        {
            int idx = -1;
            for (int i = start; i < n; i++)
            {
                if (src[i][start].value != 0)
                {
                    idx = i;
                    break;
                }
            }
            if (idx < 0)
            {
                return -1;
            }
            if (idx != start)
            {
                src[start] += src[idx];
                dst[start] += dst[idx];
            }

            tmp = src[start][start];
            dst[start] /= (tmp);
            src[start] /= (tmp);
            for (int j = start + 1; j < n; j++)
            {
                if (src[j][start].value != 0)
                {
                    tmp = src[j][start];
                    dst[j] /= (tmp);
                    src[j] /= (tmp);
                    src[j] -= src[start];
                    dst[j] -= dst[start];
                }
                src[j].zeros = start + 1;
            }
        }

        for (start = n - 2; start >= 0; start--)
        {
            for (int i = start + 1; i < n; i++)
            {
                if (src[start][i].value != 0)
                {
                    dst[start] -= dst[i] * src[start][i];
                    src[start].sub_mul(src[i], src[start][i]);
                }
            }
        }
        return 0;
    }

    void dump()
    {
        for (int i = 0; i < m; i++)
        {
            for (int j = 0; j < n; j++)
            {
                printf("%03d ", row[i][j].value);
            }
            printf("\n");
        }
        printf("\n");
        printf("#######\n");
    }
};

int get_time()
{
    struct timespec time1 = {0, 0};

    clock_gettime(CLOCK_REALTIME, &time1);

    return (time1.tv_sec) * 1000 + (time1.tv_nsec) / 1000000;
}

class Generator
{
public:
    Matric *G; // generator matric
    int n;
    int k;
    Generator(int r1, int r2)
    {

        n = r1;
        k = r2;
        Matric info_matric(n - k, k);
        Matric check_matric(n - k, n - k);

        G = new Matric(n - k, k);
        for (int i = 0; i < n - k; i++)
        {
            for (int j = 0; j < k; j++)
            {
                if (i == 0)
                    info_matric[i][j] = 1;
                else
                {
                    node t(j + 1);
                    info_matric[i][j] = t.exp(i);
                }
            }
        }
        for (int i = 0; i < n - k; i++)
        {
            for (int j = 0; j < n - k; j++)
            {
                if (i == 0)
                    check_matric[i][j] = 1;
                else
                {
                    node t(j + 1 + k);
                    check_matric[i][j] = t.exp(i);
                }
            }
        }

        Matric cinv(n - k, n - k);
        check_matric.inv(cinv);
        *G = cinv * info_matric;
    }

    // input  k length
    // output n length  check_data append to tail
    int generate(Vector &info)
    {
        Vector check(n - k);
        if (info.length != k)
        {
            return -1;
        }
        check = (*G) * info;

        for (int i = 0; i < n - k; i++)
        {
            info.val.push_back(check[i]);
        }
        info.length = n;
        return 0;
    }
};

class Checker
{
public:
    Matric *H; // check matric
    int n;
    int k;
    Checker(int r1, int r2)
    {
        n = r1;
        k = r2;
        H = new Matric(n - k, n);
        for (int i = 0; i < n - k; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (i == 0)
                    (*H)[i][j] = 1;
                else
                {
                    node t(j + 1);
                    (*H)[i][j] = t.exp(i);
                }
            }
        }
    }
    int error_locate(int err_cnt, Vector &S, Vector &dst) // err_locate_matric *A = Scross
    {
        Matric err_locate_matric(err_cnt, err_cnt);
        for (int i = 0; i < err_cnt; i++)
        {
            for (int j = 0; j < err_cnt; j++)
            {
                err_locate_matric[i][j] = S[i + j];
            }
            dst[i] = S[err_cnt + i];
        }

        if (err_locate_matric.solve_equation(dst) == 0)
        {
            return 0;
        }
        return -1;
    }

    int ecc(Vector &info)
    {
        Vector S(n - k);
        S = (*H) * info;

        if (S.is_zero())
        {
            return 0;
        }

        for (int i = (n - k) / 2; i >= 1; i--)
        {
            Vector A(i);
            if (error_locate(i, S, A) == 0)
            {
                Matric X(i, i);

                Vector error_positon(i);

                int cnt = 0;
                for (unsigned int j = 1; j <= n; j++)
                {
                    node xj = node(j).inv();
                    node position_check(1);
                    for (int t = 1; t <= i; t++)
                    {
                        position_check += A[i - t] * (xj.exp(t));
                    }
                    if (position_check.value == 0)
                    {
                        if (cnt >= i)
                        {
                            return -1;
                        }

                        error_positon[cnt] = j - 1;
                        for (unsigned int q = 0; q < i; q++)
                        {
                            X[q][cnt] = (*H)[q][j - 1]; // pick the error position line
                        }
                        cnt++;
                    }
                }
                if (cnt != i)
                {

                    return -2;
                }

                if (X.solve_equation(S) < 0) // X*Y = S Y= Xinv *S
                {
                    return -3;
                }

                for (int j = 0; j < i; j++)
                {
                    info[error_positon[j].value] -= S[j];
                }

                S = (*H) * info;

                if (!S.is_zero())
                {
                    return -5;
                }

                return i;
            }
        }
        return -4;
    }
};

void shuffle(vector<int> &a)
{
    int n = a.size();
    for (int i = n; i >= 2; i--)
    {
        int idx = rand() % i;
        int front = n - i;
        if (idx != 0)
        {
            int tmp = a[front + idx];
            a[front + idx] = a[front];
            a[front] = tmp;
        }
    }
}



#include <unistd.h>

#define n 255
#define k 223

Generator gen(n, k);
Checker checker(n, k);

void generate_code(char *name)
{
    FILE *fp = fopen(name, "r");
    FILE *fw = fopen("code", "w");
    char buf[k];

    while (1)
    {
        int ret = fread(buf, 1, k, fp);
        if (ret > 0)
        {
            Vector info(k);
            for (int i = 0; i < k; i++)
            {
                info[i] = buf[i];
            }
            gen.generate(info);
        }
    }
}

int main()
{
    
}

int xmain()
{

    int tick;

    int tt = 0;

    while (1)
    {
        Vector info(k);

        srand((unsigned)time(0));

        for (int i = 0; i < k; i++)
        {
            info[i] = rand() % 256;
        }
        gen.generate(info);

        int err_cnt = rand() % ((n - k) / 2 - 2) + 1;
        vector<int> a;
        for (int i = 0; i < n; i++)
        {
            a.push_back(i);
        }
        shuffle(a);

        for (int i = 0; i < err_cnt; i++)
        {
            int idx = a[i];

            info[idx].value = (info[idx].value + rand() % 10 + 1) % 256;
        }
        tick = get_time();

        int ret = checker.ecc(info);
        if (ret < 0)
        {
            printf("%d err ecc error %d\n", err_cnt, ret);
            continue;
        }
        printf("%d\n", info.length);
        if (get_time() - tick > tt)
        {
            tt = get_time() - tick;
            printf("%d  :%d\n", err_cnt, tt);
        }
        // usleep(500*1000);
    }

    return 0;
}