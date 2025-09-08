#include "Md5.hh"

#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#pragma GCC diagnostic ignored "-Wshift-count-negative"

#include <stdexcept>
extern "C" {
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
}

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define G(x, y, z) (y ^ (z & (x ^ y)))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

#define ROTATE_LEFT(x, s) (x<<s | x>>(32-s))

#define STEP(f, a, b, c, d, x, t, s) ( \
    a += f(b, c, d) + x + t, \
    a = ROTATE_LEFT(a, s), \
    a += b \
)

static void
_md5_init(struct pekwm::md5_context& ctx)
{
    ctx.a = 0x67452301;
    ctx.b = 0xefcdab89;
    ctx.c = 0x98badcfe;
    ctx.d = 0x10325476;
    ctx.count[0] = 0;
    ctx.count[1] = 0;
}

static uint8_t*
_md5_transform(struct pekwm::md5_context& ctx, const void* data, uintmax_t size)
{
    uint8_t* ptr = (uint8_t*) data;
    uint32_t a, b, c, d, aa, bb, cc, dd;

    #define GET(n) (ctx.block[(n)])
    #define SET(n) (ctx.block[(n)] =        \
          ((uint32_t)ptr[(n)*4 + 0] << 0 )   \
        | ((uint32_t)ptr[(n)*4 + 1] << 8 )   \
        | ((uint32_t)ptr[(n)*4 + 2] << 16)   \
        | ((uint32_t)ptr[(n)*4 + 3] << 24) )

    a = ctx.a;
    b = ctx.b;
    c = ctx.c;
    d = ctx.d;

    do {
        aa = a;
        bb = b;
        cc = c;
        dd = d;

        STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7);
        STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12);
        STEP(F, c, d, a, b, SET(2), 0x242070db, 17);
        STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22);
        STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7);
        STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12);
        STEP(F, c, d, a, b, SET(6), 0xa8304613, 17);
        STEP(F, b, c, d, a, SET(7), 0xfd469501, 22);
        STEP(F, a, b, c, d, SET(8), 0x698098d8, 7);
        STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12);
        STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17);
        STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22);
        STEP(F, a, b, c, d, SET(12), 0x6b901122, 7);
        STEP(F, d, a, b, c, SET(13), 0xfd987193, 12);
        STEP(F, c, d, a, b, SET(14), 0xa679438e, 17);
        STEP(F, b, c, d, a, SET(15), 0x49b40821, 22);

        STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5);
        STEP(G, d, a, b, c, GET(6), 0xc040b340, 9);
        STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14);
        STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20);
        STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5);
        STEP(G, d, a, b, c, GET(10), 0x02441453, 9);
        STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14);
        STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20);
        STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5);
        STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9);
        STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14);
        STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20);
        STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5);
        STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9);
        STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14);
        STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20);

        STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4);
        STEP(H, d, a, b, c, GET(8), 0x8771f681, 11);
        STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16);
        STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23);
        STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4);
        STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11);
        STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16);
        STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23);
        STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4);
        STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11);
        STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16);
        STEP(H, b, c, d, a, GET(6), 0x04881d05, 23);
        STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4);
        STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11);
        STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16);
        STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23);

        STEP(I, a, b, c, d, GET(0), 0xf4292244, 6);
        STEP(I, d, a, b, c, GET(7), 0x432aff97, 10);
        STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15);
        STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21);
        STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6);
        STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10);
        STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15);
        STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21);
        STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6);
        STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10);
        STEP(I, c, d, a, b, GET(6), 0xa3014314, 15);
        STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21);
        STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6);
        STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10);
        STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15);
        STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21);

        a += aa;
        b += bb;
        c += cc;
        d += dd;

        ptr += 64;
    } while (size -= 64);

    ctx.a = a;
    ctx.b = b;
    ctx.c = c;
    ctx.d = d;

    #undef GET
    #undef SET

    return ptr;
}

static void
_md5_update(struct pekwm::md5_context& ctx, const void* buffer,
	    uint32_t buffer_size)
{
    uint32_t saved_low = ctx.count[0];
    uint32_t used;
    uint32_t free;

    if ((ctx.count[0] = ((saved_low+buffer_size) & 0x1fffffff)) < saved_low) {
        ctx.count[1]++;
    }
    ctx.count[1] += (uint32_t)(buffer_size>>29);

    used = saved_low & 0x3f;

    if (used) {
        free = 64 - used;

        if (buffer_size < free) {
            memcpy(&ctx.input[used], buffer, buffer_size);
            return;
        }

        memcpy(&ctx.input[used], buffer, free);
        buffer = (uint8_t*) buffer + free;
        buffer_size -= free;
        _md5_transform(ctx, ctx.input, 64);
    }

    if (buffer_size >= 64) {
        buffer = _md5_transform(ctx, buffer,
				buffer_size & ~(unsigned long)0x3f);
        buffer_size = buffer_size % 64;
    }

    memcpy(ctx.input, buffer, buffer_size);
}

static void
_md5_finalize(struct pekwm::md5_context& ctx, uint8_t *digest)
{
    uint32_t used = ctx.count[0] & 0x3f;
    ctx.input[used++] = 0x80;
    uint32_t free = 64 - used;

    if (free < 8) {
        memset(&ctx.input[used], 0, free);
        _md5_transform(ctx, ctx.input, 64);
        used = 0;
        free = 64;
    }

    memset(&ctx.input[used], 0, free - 8);

    ctx.count[0] <<= 3;
    ctx.input[56] = (uint8_t)(ctx.count[0]);
    ctx.input[57] = (uint8_t)(ctx.count[0] >> 8);
    ctx.input[58] = (uint8_t)(ctx.count[0] >> 16);
    ctx.input[59] = (uint8_t)(ctx.count[0] >> 24);
    ctx.input[60] = (uint8_t)(ctx.count[1]);
    ctx.input[61] = (uint8_t)(ctx.count[1] >> 8);
    ctx.input[62] = (uint8_t)(ctx.count[1] >> 16);
    ctx.input[63] = (uint8_t)(ctx.count[1] >> 24);

    _md5_transform(ctx, ctx.input, 64);

    digest[0] = (uint8_t)(ctx.a);
    digest[1] = (uint8_t)(ctx.a >> 8);
    digest[2] = (uint8_t)(ctx.a >> 16);
    digest[3] = (uint8_t)(ctx.a >> 24);
    digest[4] = (uint8_t)(ctx.b);
    digest[5] = (uint8_t)(ctx.b >> 8);
    digest[6] = (uint8_t)(ctx.b >> 16);
    digest[7] = (uint8_t)(ctx.b >> 24);
    digest[8] = (uint8_t)(ctx.c);
    digest[9] = (uint8_t)(ctx.c >> 8);
    digest[10] = (uint8_t)(ctx.c >> 16);
    digest[11] = (uint8_t)(ctx.c >> 24);
    digest[12] = (uint8_t)(ctx.d);
    digest[13] = (uint8_t)(ctx.d >> 8);
    digest[14] = (uint8_t)(ctx.d >> 16);
    digest[15] = (uint8_t)(ctx.d >> 24);
}

Md5::Md5()
{
	_md5_init(_ctx);
}

Md5::Md5(const std::string &data)
{
	oneGo(data.c_str(), data.size());
}

Md5::Md5(const StringView &data)
{
	oneGo(*data, data.size());
}

void
Md5::update(const std::string &data, bool do_finalize)
{
	_md5_update(_ctx, data.c_str(), data.size());
	if (do_finalize) {
		finalize();
	}
}

void
Md5::finalize()
{
	_md5_finalize(_ctx, _digest);
}

void
Md5::oneGo(const char *data, size_t len)
{
	if (len > UINT32_MAX) {
		throw std::invalid_argument("data size limit reached");
	}
	_md5_init(_ctx);
	_md5_update(_ctx, data, len);
	_md5_finalize(_ctx, _digest);
}
