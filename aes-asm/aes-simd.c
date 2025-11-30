#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

void aes_key(uint8_t *in, uint8_t *out);

void aes_e(uint8_t *in, uint8_t *out, uint8_t *rk);

int main(){

    uint8_t key[] =  {0x00, 0x01, 0x02, 0x03,
                      0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b,
                      0x0c, 0x0d, 0x0e, 0x0f};

    uint8_t res[176];
    aes_key(key, res);

    uint8_t data[] = {0x00, 0x11, 0x22, 0x33,
                      0x44, 0x55, 0x66, 0x77,
                      0x88, 0x99, 0xaa, 0xbb,
                      0xcc, 0xdd, 0xee, 0xff};
    
    uint8_t in[32];
    
    memcpy(in, data, 16);
    memcpy(in+16, data, 16);

    uint8_t out[32];
    
    uint64_t ctr = 0;
    clock_t p1 = clock();
    while(!((clock() - p1)/CLOCKS_PER_SEC)){
        aes_e(data, out, res);
        ctr++;
    }

    for(int i = 0; i < 32; ++i) printf("%02x", out[i]);
    printf("\n");
    printf("%ld\n", 2*ctr);


    return 0;

}