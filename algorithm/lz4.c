
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define WINDOW_SIZE 65535

#define HASH_BITS 17
#define HASH_SIZE (1 << HASH_BITS)

typedef struct _list
{
    uint8_t *v;
    struct _list *next;
} list;

typedef struct
{
    list *head;
    list *tail;
} array_hash;

typedef struct
{
    array_hash hash[HASH_SIZE];
    int size;
}hash_table;

uint32_t hash_32(uint8_t *p)
{
    return (((*((uint32_t *)p)) * 0x9E3779BC) >> (32 - HASH_BITS));
}

int hash_insert(hash_table*table,uint8_t *p)
{
    list *t = (list *)malloc(sizeof(list));
    uint32_t idx = hash_32(p);
    t->v = p;
    t->next = NULL;
    if (table->hash[idx].head == NULL)
    {
        table->hash[idx].tail = t;
        table->hash[idx].head = t;
    }
    else
    {

        table->hash[idx].tail->next = t;
        table->hash[idx].tail = t;
    }
    table->size++;
    return 0;
}
int hash_remove(hash_table*table,uint8_t *p)
{
    uint32_t idx = hash_32(p);
    list *hd = table->hash[idx].head;
    if(hd==NULL)
    {
        return 0;
    }
    //it must be top
    if(hd->v==p)
    {
        list *n = hd->next;
        table->hash[idx].head = hd->next;
        if(table->hash[idx].tail==hd)
        {
            table->hash[idx].tail = hd->next;
        }
        free(hd);
        table->size--;
        return 0;
    }
    return 0;
}

list *hash_read(hash_table*table,uint8_t *p)
{
    return table->hash[hash_32(p)].head;
}

int lz4_dec(uint8_t *dst, uint32_t dstlen, uint8_t *input, uint32_t ilen)
{
    uint8_t *dst_start = dst;
    uint8_t *dst_end = dst + dstlen;
    uint8_t *end = input + ilen;

    while (input < end)
    {
        int match_len = (*input) & 15;
        int literal_len = (*input) >> 4;
        uint8_t *match_start;
        input++;
        if (literal_len > 0)
        {
            if (literal_len == 15)
            {
                while (input < end)
                {
                    int c = *input;
                    literal_len += c;
                    input++;
                    if (c != 255)
                    {
                        break;
                    }
                }
            }
            if ((input + literal_len > end) || (dst + literal_len) > dst_end)
            {
                return -1;
            }
            memcpy(dst, input, literal_len);
            dst += literal_len;
            input += literal_len;
        }
        if (input == end)
        {
            return dst - dst_start;
        }
        if (input + 1 >= end)
        {
            return -2;
        }

        match_start = dst - *((unsigned short *)input); // little ending
        input += 2;

        if (match_start < dst_start)
        {
            return -3;
        }
        if (match_len == 15)
        {
            while (input < end)
            {
                int c = *input;
                match_len += c;
                input++;
                if (c != 255)
                {
                    break;
                }
            }
        }
        match_len += 4;
        if ((dst + match_len) > dst_end)
        {
            return -4;
        }
        while (match_len > 0)
        {
            *dst = *match_start;
            dst++;
            match_start++;
            match_len--;
        }
    }
    return (dst - dst_start);
}

int max_match(uint8_t *start, uint8_t *p, uint8_t *end)
{
    int len = 0;
    while (p < end)
    {
        if (*start != *p)
        {
            break;
        }
        len++;
        start++;
        p++;
    }
    return len;
}

int find_max_match(hash_table*t,uint8_t *start, uint8_t *current, uint8_t *end, int *poffset)
{
    list *head = hash_read(t,current + 1);
    int maxlen = 0;

    if (current - start + 1 > WINDOW_SIZE)
    {
        start = current - WINDOW_SIZE + 1;
    }

    while (head != NULL)
    {
        if (head->v > current)
            break;

        if (head->v < start)
        {
            head = head->next;
            continue;
        }
        int len = max_match(head->v, current + 1, end);
        if (len > maxlen)
        {
            maxlen = len;
            *poffset = current - head->v + 1;
        }
        head = head->next;
    }

    return maxlen;
}

uint8_t* rewind_hash( hash_table*t,uint8_t*hash_start)
{
    while(t->size>WINDOW_SIZE+10)
    {
        hash_remove(t,hash_start);
        hash_start++;
    }
    return hash_start;
}

int lz4_compress(uint8_t *dst, uint32_t dstlen, uint8_t *input, uint32_t ilen)
{
    int offset;
    int i;

    int max_literal=0;

    uint8_t *dst_start = dst;
    uint8_t *start = input;
    uint8_t *end = input + ilen;
   
    int literal_size = 0;
    uint8_t *current = input ;
    uint8_t*hash_start = input;

    hash_table *table = (hash_table *)calloc(1,sizeof(hash_table));

    while (current + 8 < end)
    {
        int match = find_max_match(table,start, current, end, &offset);
        uint8_t tag;
        if (match < 4)
        {
            literal_size++;
            if(literal_size>max_literal)
            {
                max_literal = literal_size;
            }
            current++;
            if(current+4<end)
            {
                hash_insert(table,current);
                hash_start = rewind_hash(table,hash_start);
            }
            continue;
        }

        if (literal_size >= 15)
        {
            tag = (15 << 4);
        }
        else
        {
            tag = (literal_size << 4);
        }
        if (match >= 4 + 15)
        {
            tag = (tag | 15);
        }
        else
        {
            tag = (tag | (match - 4));
        }
        *dst = tag;
        dst++;
        if (literal_size >= 15)
        {
            int len = literal_size - 15;
            while (len >= 255)
            {
                *dst = 255;
                dst++;
                len -= 255;
            }
            *dst = len;
            dst++;
        }
        if (literal_size > 0)
        {
            memcpy(dst, current - literal_size + 1, literal_size);
        }
        dst += literal_size;
        *dst = offset & 0xff;
        *(dst + 1) = offset >> 8;
        dst += 2;
        if (match >= 15 + 4)
        {
            int len = match - 4 - 15;
            while (len >= 255)
            {
                *dst = 255;
                dst++;
                len -= 255;
            }
            *dst = len;
            dst++;
        }
        if(literal_size>1000)
        {
            printf("%d %d\n",literal_size,match);
        }
        literal_size = 0;
        for(int i=1;i<=match;i++)
        {
            if(current+i+4<end)
            {
                hash_insert(table,current+i);
            }
        }
        hash_start = rewind_hash(table,hash_start);
        
        current += match;
    }

    if (current != end - 1)
    {
        literal_size += (end - 1 - current);
        current = end - 1;
    }

    if (literal_size > 0)
    {
        uint8_t tag;
        if (literal_size >= 15)
        {
            tag = (15 << 4);
        }
        else
        {
            tag = (literal_size << 4);
        }
        *dst = tag;
        dst++;
        if (literal_size >= 15)
        {
            int len = literal_size - 15;
            while (len >= 255)
            {
                *dst = 255;
                dst++;
                len -= 255;
            }
            *dst = len;
            dst++;
        }
        memcpy(dst, current - literal_size + 1, literal_size);
        dst += literal_size;
    }

    printf("max literal %d\n",max_literal);

    return (dst - dst_start);
}

#define MAX_LEN 0x1000000

char buf[MAX_LEN];
char dst_buf[MAX_LEN];

char k_buf[MAX_LEN];

int main(int agc,char **argv)
{
    FILE *fp = fopen(argv[1], "r");
    int ret = fread(buf, 1, MAX_LEN, fp);
    int clen = lz4_compress(dst_buf,MAX_LEN,buf,ret);

    printf("compress len %d\n",clen);

    

    
    printf("\n");
    return 0;
}