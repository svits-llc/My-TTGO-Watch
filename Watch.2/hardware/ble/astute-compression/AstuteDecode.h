// https://picheta.me/obfuscator
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "aw_profile.h"

#ifndef AW_SHOW
    #define AW_SHOW(prefix, ...)
#endif

#ifndef AW_PROFILE
    #define AW_PROFILE()
#endif

enum EType
{
    etKEY,
    etDIFF,
    etRMB
};

typedef uint8_t aw_counter_t;
typedef uint16_t aw_rgb565_t;

#define AW_CAP 0b101101
typedef struct
{
    uint32_t cap : 6; //AW_CAP
    uint32_t bpp : 3; //1, 2, 4, 8, 16 as power of 2, so real values are 0 to 4
    uint32_t type : 3; //EType
    uint32_t size : 20;
} AWHeader;


typedef void (*receiver_t)(void* cookie, size_t offset, size_t count, aw_rgb565_t* pix);

#ifndef AW_BUFF_SIZE
    #define AW_BUFF_SIZE (1024+512)
#endif

enum EOperation {
    eoFillSkip,
    eoCopy,
    eoHeader,
    eoPalette,
    eoLongSkipFill //uint16_t
};

typedef struct {
    receiver_t callback;
    void* cookie;
    AWHeader header;
    size_t filled;
    char* ptr;
    enum EOperation operation;
    size_t offset; //in pixels, where are we
    size_t expect;
    aw_rgb565_t long_fill;
//persistent part for reinit
    char* buff;
    aw_rgb565_t* palette;
    unsigned char bpp;
    char* mem;
} AWDecoder;


inline void* aw_get_tail(AWDecoder* decoder, size_t& size)
{
    size_t palette_size = decoder->buff - decoder->mem;
    size = AW_BUFF_SIZE - palette_size - decoder->filled;
    return decoder->buff + decoder->filled;
}


inline int aw_decoder_init(AWDecoder* decoder, receiver_t callback, void* cookie)
{
    AW_PROFILE();
    char* buff = 0, * mem = 0;
    unsigned char bpp = 0;
    aw_rgb565_t* palette = 0;
    int reinit = AW_CAP == decoder->header.cap;
    if (reinit) {
        buff = decoder->buff;
        mem = decoder->mem;
        palette = decoder->palette;
        bpp = decoder->bpp;
    }
    else
    {
        mem = (char*)malloc(AW_BUFF_SIZE);
        if (!mem)
            return -1;
        buff = mem;
    }
    memset(decoder, 0, sizeof(AWDecoder));

    decoder->bpp = bpp;
    decoder->mem = mem;
    decoder->buff = buff;
    decoder->palette = palette;
    decoder->callback = callback;
    decoder->cookie = cookie;
    decoder->ptr = decoder->buff;

    decoder->operation = eoHeader;
    decoder->expect = sizeof(AWHeader);

    return 0;
}

inline double aw_ceil(double num) {
    int integerPart = (int)num;
    double fractionalPart = num - integerPart;

    if (fractionalPart > 0)
        return integerPart + 1;
    else
        return integerPart;
}

inline int aw_decoder_qr(AWDecoder* decoder)
{
    AW_PROFILE();
    unsigned char qr[] = {0, 0, 0, 254, 144, 63, 130, 186, 32, 186, 180, 46, 186, 182, 46, 186, 136, 46, 130, 190, 32, 254, 170, 63, 0, 8, 0, 190, 87, 21, 118, 67, 58, 140, 253, 20, 4, 87, 31, 158, 154, 6, 0, 6, 47, 254, 106, 25, 130, 104, 12, 186, 194, 4, 186, 70, 31, 186, 58, 12, 130, 82, 13, 254, 170, 20, 0, 0, 0, 0, 0, 0 };
    if (sizeof(qr) + 240 * sizeof(aw_rgb565_t) > AW_BUFF_SIZE) return -1;
    unsigned char* ptr = qr;
    unsigned char* end = ptr + sizeof(qr);
    size_t offset = 0;
    while (ptr < end) {
        aw_rgb565_t* line = (aw_rgb565_t*)decoder->mem;
        int count = 3;
        while(count--)
        {
            unsigned char byte = *ptr++;
            for (int i = 0; i < 8; ++i)
            {
                aw_rgb565_t color = (byte & 1) ? 0xFFFF : 0x0000;
                printf((byte & 1) ? "@" : " ");
                for (int j = 0; j < 10; ++j)
                {
                    *line++ = color;
                }
                byte >>= 1;
            }
        }
        printf("\n");
        for (int i = 0; i < 10; ++i)
        {
            decoder->callback(decoder->cookie, offset, 240, (aw_rgb565_t*)decoder->mem);
            offset += 240;
        }
    }
    return offset;
}

inline int aw_decoder_chunk(AWDecoder* decoder)
{
    AW_PROFILE();
    size_t bytes_left = 0;
    while (1)
    {
        bytes_left = decoder->filled - (decoder->ptr - decoder->buff);
        if (bytes_left < decoder->expect)
            break;

        switch (decoder->operation) //might be expencive to access bits
        {
        case eoHeader:
        {
            decoder->header = *(AWHeader*)decoder->ptr;
            if (decoder->header.cap != AW_CAP)
                return -1;
            decoder->ptr += sizeof(AWHeader);
            if (decoder->header.size)
            {
                decoder->operation = eoFillSkip;
                decoder->expect = sizeof(aw_counter_t) + aw_ceil(decoder->bpp / 8.);
                if (decoder->header.bpp)
                {
                    decoder->bpp = 1 << (decoder->header.bpp - 1);
                    if (decoder->bpp < 16) //decoder->header.bpp is zero when old palette is used
                    {
                        decoder->operation = eoPalette;
                        decoder->expect = sizeof(aw_counter_t) + (2 << decoder->bpp);
                    }
                    else
                    {
                        decoder->palette = 0;
                        decoder->buff = decoder->mem;
                        bytes_left -= sizeof(AWHeader);
                        memcpy(decoder->buff, decoder->ptr, bytes_left);
                        decoder->ptr = decoder->buff;
                        decoder->filled = bytes_left;
                    }
                } //otherwice we keep old palette
                AW_SHOW("decoder:header", decoder->bpp);
            } else {
                //empty diff
                decoder->operation = eoHeader;
                decoder->expect = sizeof(AWHeader);
            }
            break;
        }
        case eoPalette:
        {
            decoder->palette = (aw_rgb565_t*)decoder->mem;
            size_t size = *(aw_counter_t*)decoder->ptr * sizeof(aw_rgb565_t);
            decoder->ptr += sizeof(aw_counter_t);
            bytes_left -= sizeof(aw_counter_t);
            AW_SHOW("decoder:palette", size);
            if (!size) //full palette size is 256 and doesn't fit in 0..255 range
                size = sizeof(aw_rgb565_t) * (1 << decoder->bpp);
            size_t to_copy = bytes_left < size ? bytes_left : size;
            memcpy(decoder->palette, decoder->ptr, to_copy); //memmove?
            decoder->ptr += to_copy;
            bytes_left -= to_copy;
            decoder->buff = decoder->mem + size;
            memcpy(decoder->buff, decoder->ptr, bytes_left);
            decoder->ptr = decoder->buff;
            decoder->filled = bytes_left;
            decoder->operation = eoFillSkip;
            decoder->expect = sizeof(aw_counter_t) + aw_ceil(decoder->bpp / 8.);
            break;
        }
        case eoCopy:
        {
            aw_counter_t count = *(aw_counter_t*)decoder->ptr;
            AW_SHOW("decoder:copy", count);

            size_t pixels_left = (bytes_left - sizeof(aw_counter_t)) * 8 / decoder->bpp; //buffer can have less pixels we need to copy
            if (pixels_left < count) {
                decoder->expect = sizeof(aw_counter_t) + aw_ceil(count * decoder->bpp / 8.);
                return 0;
            }
            decoder->ptr += sizeof(aw_counter_t);
            bytes_left -= sizeof(aw_counter_t);
            if (count) { // can be zero to ensure tic-tok switch on every pace
                AW_SHOW("decoder:write", decoder->offset);

                //TODO: 255 == count for repeated copies?
                if (decoder->bpp < 16)
                {
                    size_t pixels_left = bytes_left * 8 / decoder->bpp; //buffer can have less pixels we need to copy
                    pixels_left = (pixels_left < count) ? pixels_left : count;

                    size_t tail_size = 0;
                    aw_rgb565_t* tail = (aw_rgb565_t*)aw_get_tail(decoder, tail_size);
                    tail_size /= sizeof(aw_rgb565_t);
                    if (tail_size < count)
                        tail = 0;
                    size_t written_tail = 0;
                    if (decoder->bpp < 8)
                    {
                        for (size_t i = 0; i < aw_ceil(pixels_left * decoder->bpp / 8.); ++i)
                        {
                            uint8_t index = *(uint8_t*)decoder->ptr;
                            decoder->ptr += sizeof(uint8_t);
                            for (size_t j = 0; j < (8 / decoder->bpp) && count; ++j)
                            {
                                uint8_t val = index & ((1 << decoder->bpp) - 1);
                                index >>= decoder->bpp;
                                aw_rgb565_t pix = decoder->palette[val];
                                AW_SHOW("decoder:index", pix, val);
                                if (tail)
                                    tail[written_tail++] = pix;
                                else { //FIXME: write in cycles when not enough tail
                                    decoder->callback(decoder->cookie, decoder->offset, 1, &pix);
                                    decoder->offset += 1;
                                }
                                --count;
                            }
                            --bytes_left;
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < pixels_left; ++i) {
                            uint8_t index = *(uint8_t*)decoder->ptr;
                            decoder->ptr += sizeof(uint8_t);
                            aw_rgb565_t pix = decoder->palette[index];
                            if (tail)
                                tail[written_tail++] = pix;
                            else { //FIXME: write in cycles when not enough tail
                                decoder->callback(decoder->cookie, decoder->offset, 1, &pix);
                                decoder->offset += 1;
                            }
                        }
                        count -= pixels_left;
                        bytes_left -= pixels_left;
                    }
                    if (tail)
                    {
                        decoder->callback(decoder->cookie, decoder->offset, written_tail, tail);
                        decoder->offset += written_tail;
                    }
                } else {
                    size_t pixels_left = bytes_left / sizeof(aw_rgb565_t); //buffer can have less pixels we need to copy
                    pixels_left = (pixels_left < count) ? pixels_left : count;
                    decoder->callback(decoder->cookie, decoder->offset, pixels_left, (aw_rgb565_t*)decoder->ptr);
                    decoder->offset += pixels_left;
                    decoder->ptr += pixels_left * sizeof(aw_rgb565_t);
                    count -= pixels_left;
                    bytes_left -= pixels_left * sizeof(aw_rgb565_t);
                }
            }
            //FIXME: switch to expect more instead!

            decoder->operation = eoFillSkip;
            decoder->expect = sizeof(aw_counter_t) + aw_ceil(decoder->bpp / 8.);
            break;
        }; 
        case eoFillSkip:
        {
            aw_counter_t count = *(aw_counter_t*)decoder->ptr;
            AW_SHOW("decoder:FillSkip", decoder->offset, count);
            decoder->ptr += sizeof(aw_counter_t);
            if (count)
            {
                if (etKEY == decoder->header.type)
                {
                    aw_rgb565_t pix;
                    if (decoder->bpp < 16)
                    {
                        uint8_t index = *(uint8_t*)decoder->ptr;
                        decoder->ptr += sizeof(uint8_t);
                        pix = decoder->palette[index];
                    }
                    else
                    {
                        pix = *(aw_rgb565_t*)decoder->ptr;
                        decoder->ptr += sizeof(aw_rgb565_t);
                    }
                    if (255 == count) //special case for fills that don't fit into byte
                    {
                        decoder->long_fill = pix;
                        decoder->operation = eoLongSkipFill;
                        decoder->expect = sizeof(aw_rgb565_t);
                        continue;
                    }

                    size_t tail_size = 0;
                    aw_rgb565_t* tail = (aw_rgb565_t*)aw_get_tail(decoder, tail_size);
                    tail_size /= sizeof(aw_rgb565_t);
                    if (tail_size < count)
                        tail = 0;

                    size_t written_tail = 0;

                    if (tail) {
                        while (count--)
                            tail[written_tail++] = pix;
                        if (written_tail)
                        {
                            decoder->callback(decoder->cookie, decoder->offset, written_tail, tail);
                            decoder->offset += written_tail;
                        }
                    }
                    else {
                        while (count--) {
                            decoder->callback(decoder->cookie, decoder->offset, 1, &pix);
                            decoder->offset += 1;
                        }
                    }
                }
                else if (etDIFF == decoder->header.type)
                {
                    if (255 == count) //special case for jumps that don't fit into byte
                    {
                        decoder->operation = eoLongSkipFill;
                        decoder->expect = sizeof(aw_rgb565_t);
                        continue;
                    }
                    else
                        decoder->offset += count;
                }
            }
            decoder->operation = eoCopy;
            decoder->expect = sizeof(aw_counter_t) + aw_ceil(decoder->bpp / 8.);
            break;
        }
        case eoLongSkipFill:
        {
            aw_rgb565_t count = *(aw_rgb565_t*)decoder->ptr;
            AW_SHOW("decoder:LongSkipFill", count);

            decoder->ptr += sizeof(aw_rgb565_t);
            if (etKEY == decoder->header.type)
            {
                size_t tail_size = 0;
                aw_rgb565_t* tail = (aw_rgb565_t*)aw_get_tail(decoder, tail_size);
                tail_size /= sizeof(aw_rgb565_t);

                if (tail_size > 1) {

                    while (count)
                    {
                        size_t written_tail = 0;
                        size_t to_fill = count < tail_size ? count : tail_size;
                        while (to_fill--)
                            tail[written_tail++] = decoder->long_fill;
                        decoder->callback(decoder->cookie, decoder->offset, written_tail, tail);
                        decoder->offset += written_tail;
                        count -= written_tail;
                    }
                }
                else {
                    while (count--) {
                        decoder->callback(decoder->cookie, decoder->offset, 1, &decoder->long_fill);
                        decoder->offset += 1;
                    }
                }
            } else
                decoder->offset += count;
            decoder->operation = eoFillSkip;
            decoder->expect = sizeof(aw_counter_t) + aw_ceil(decoder->bpp / 8.);
        }
        }
    };
    if (decoder->buff != decoder->ptr)
    {
        memcpy(decoder->buff, decoder->ptr, bytes_left);
        decoder->ptr = decoder->buff;
    }
    decoder->filled = bytes_left;
    return 0;
}


inline int aw_decoder_fini(AWDecoder* decoder, int free_mem)
{
    AW_PROFILE();
    int res = 0;
    if (decoder->offset) {
        size_t bytes_left = AW_BUFF_SIZE - decoder->filled;
        const size_t noop = sizeof(aw_counter_t) + sizeof(aw_rgb565_t);
        if (bytes_left > noop)
        {
            aw_counter_t* ptr = (aw_counter_t*)(decoder->buff + decoder->filled);
            decoder->filled += noop;
            memset(ptr, 0, noop);
            decoder->expect = decoder->filled - (decoder->ptr - decoder->buff);
        }
        res = aw_decoder_chunk(decoder);
    } //else empty run, maybe QR
    if (free_mem)
    {
        free(decoder->mem);
        memset(decoder, 0, sizeof(AWDecoder));
    }
    return res;
}
