#include <stdio.h>

#define KMAP(k,f) k
#define KEND 0

int table[128] = {
    /* 00 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 04 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 08 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 0c */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 10 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 14 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 18 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 1c */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 20 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 24 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 28 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 2c */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 30 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 34 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 38 */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 3c */ KMAP(1, 0), KMAP(1, 0), KMAP(1, 0), KMAP(1, 0),
    /* 40 */ KMAP(40, 0), 
    /* 41 */ KMAP(41, 0), KMAP(42, 0), KMAP(43, 0),
    /* 44 */ KMAP(44, 0), 
    /* 45 */ KMAP(45, 0), 
    /* 46 */ KMAP(46, 0),
    /* 47 */ KMAP(47, 0),
    /* 48 */ KMAP(48, 0),
    /* 49 */ KMAP(49, 0), KMAP(0x4a, 0), KMAP(0x4b, 0), KMAP(0x4c, 0), KMAP(0x4d, 0),
    /* 4e */ KMAP(0x4e, 0), KMAP(0x4f, 0),
};

int main() {
    printf("4f is at index: 0x%02x\n", 0);
    for(int i=0; i<128; i++) {
        if (table[i] == 0x4f) printf("Actual 0x4f is at index: 0x%02x\n", i);
    }
}
