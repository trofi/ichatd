/*
 * An implementation of the ARC4 algorithm,
 * by Christophe Devine <devine@cr0.net>;
 * this program is licensed under the GPL.
 */

#include "rc4.h"

void
rc4_setup (
    struct rc4_state * s,
    unsigned char    * key,
    int                length)
{
    int i, j, k, *m, a;

    s->x = s->y = 0;
    m = s->m;

    for (i = 0; i < 256; i++)
        m[i] = i;
    
    j = k = 0;

    for (i = 0; i < 256; i++)
    {
        a = m[i];
        j = (j + a + key[k]) & 0xFF;
        m[i] = m[j]; m[j] = a;
        if (++k >= length)
            k = 0;
    }
}

void
rc4_crypt (struct rc4_state * s,
           unsigned char    * data,
           int                length)
{ 
    int i, x, y, *m, a, b;

    x = s->x;
    y = s->y;
    m = s->m;

    for (i = 0; i < length; i++)
    {
        x = (x + 1) & 0xFF;
        a = m[x];
        y = (y + a) & 0xFF;
        m[x] = b = m[y];
        m[y] = a;
        data[i] ^= m[(a + b) & 0xFF];
    }

    s->x = x;
    s->y = y;
}
