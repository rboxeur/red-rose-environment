/*
 * Microsoft Video-1 Decoder unit tests
 *
 * Copyright 2024 Anton Baskanov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>

#include "wine/test.h"

struct bitmap_info
{
    BITMAPINFOHEADER header;
    union
    {
        RGBQUAD colors[256];
        DWORD color_masks[3];
    };
};

typedef void check_pixel_t(int line, int frame, int row, int col, BYTE *data, BYTE *expected_data);

static const struct bitmap_info format_cram8 =
{
    .header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 4,
        .biHeight = 16,
        .biPlanes = 1,
        .biBitCount = 8,
        .biCompression = mmioFOURCC('C','R','A','M'),
    },
};

static const struct bitmap_info format_cram16 =
{
    .header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 4,
        .biHeight = 16,
        .biPlanes = 1,
        .biBitCount = 16,
        .biCompression = mmioFOURCC('C','R','A','M'),
    },
};

static const struct bitmap_info format_rgb8 =
{
    .header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 4,
        .biHeight = 16,
        .biPlanes = 1,
        .biBitCount = 8,
        .biCompression = BI_RGB,
        .biSizeImage = 16 * 4,
    },
};

static const struct bitmap_info format_rgb555 =
{
    .header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 4,
        .biHeight = 16,
        .biPlanes = 1,
        .biBitCount = 16,
        .biCompression = BI_RGB,
        .biSizeImage = 16 * 4 * 2,
    },
};

static const struct bitmap_info format_rgb565 =
{
    .header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 4,
        .biHeight = 16,
        .biPlanes = 1,
        .biBitCount = 16,
        .biCompression = BI_BITFIELDS,
        .biSizeImage = 16 * 4 * 2,
    },
    .color_masks =
    {
        0xf800,
        0x07e0,
        0x001f,
    },
};

static const struct bitmap_info format_rgb24 =
{
    .header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 4,
        .biHeight = 16,
        .biPlanes = 1,
        .biBitCount = 24,
        .biCompression = BI_RGB,
        .biSizeImage = 16 * 4 * 3,
    },
};

static const struct bitmap_info format_yvuv =
{
    .header =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = 4,
        .biHeight = 16,
        .biPlanes = 1,
        .biBitCount = 16,
        .biCompression = mmioFOURCC('Y','V','U','V'),
        .biSizeImage = 16 * 4 * 2,
    },
};

static void check_image(int line, int frame, check_pixel_t *check_pixel,
        const struct bitmap_info *out_format, BYTE *image, BYTE *expected_image)
{
    int stride = ((out_format->header.biWidth * out_format->header.biBitCount + 31) >> 3) & ~3;
    int step = out_format->header.biBitCount >> 3;
    int row, col;

    for (row = 0; row < out_format->header.biHeight; ++row)
    {
        for (col = 0; col < out_format->header.biWidth; ++col)
        {
            BYTE *pixel = image + row * stride + col * step;
            BYTE *expected_pixel = expected_image + row * stride + col * step;
            check_pixel(line, frame, row, col, pixel, expected_pixel);
        }
    }
}

static void check_pixel_rgb8(int line, int frame, int row, int col, BYTE *pixel,
        BYTE *expected_pixel)
{
    ok_(__FILE__, line)(*pixel == *expected_pixel, "got frame %d image[%d][%d] 0x%02x.\n", frame,
            row, col, *pixel);
}

static void check_pixel_rgb555(int line, int frame, int row, int col, BYTE *pixel,
        BYTE *expected_pixel)
{
    /* Native sets the MSB inconsistently so ignore it. */
    ok_(__FILE__, line)((*(WORD *)pixel & 0x7fff) == (*(WORD *)expected_pixel & 0x7fff),
            "got frame %d image[%d][%d] 0x%02x.\n", frame, row, col, *pixel);
}

static void check_pixel_rgb565(int line, int frame, int row, int col, BYTE *pixel,
        BYTE *expected_pixel)
{
    todo_wine_if(*(WORD *)pixel != *(WORD *)expected_pixel)
        ok_(__FILE__, line)(*(WORD *)pixel == *(WORD *)expected_pixel,
                "got frame %d image[%d][%d] 0x%02x.\n", frame, row, col, *pixel);
}

static void check_pixel_rgb24(int line, int frame, int row, int col, BYTE *pixel,
        BYTE *expected_pixel)
{
    int i;

    for (i = 0; i < 3; ++i)
    {
        todo_wine_if(pixel[i] != expected_pixel[i])
            ok_(__FILE__, line)(pixel[i] == expected_pixel[i],
                    "got frame %d image[%d][%d][%d] 0x%02x.\n", frame, row, col, i, pixel[i]);
    }
}

static void test_ICDecompressQuery(void)
{
    LRESULT lr;
    HIC hic;

    hic = ICOpen(mmioFOURCC('V','I','D','C'), mmioFOURCC('M','S','V','C'), ICMODE_DECOMPRESS);
    ok(!!hic, "got hic %p.\n", hic);

    lr = ICDecompressQuery(hic, &format_cram8, &format_rgb8);
    ok(lr == ICERR_OK, "got lr %Id.\n", lr);

    lr = ICDecompressQuery(hic, &format_cram16, &format_rgb555);
    ok(lr == ICERR_OK, "got lr %Id.\n", lr);

    lr = ICDecompressQuery(hic, &format_cram16, &format_rgb565);
    ok(lr == ICERR_OK, "got lr %Id.\n", lr);

    lr = ICDecompressQuery(hic, &format_cram16, &format_rgb24);
    ok(lr == ICERR_OK, "got lr %Id.\n", lr);

    lr = ICDecompressQuery(hic, &format_cram16, &format_yvuv);
    ok(lr == ICERR_BADFORMAT, "got lr %Id.\n", lr);

    ICClose(hic);
}

#define check_ICDecompress(check_pixel, in_format, out_format, data0, data0_size, data1, data1_size, expected_data) \
        check_ICDecompress_(__LINE__, check_pixel, in_format, out_format, data0, data0_size, data1, data1_size, expected_data)
static void check_ICDecompress_(int line, check_pixel_t *check_pixel,
        const struct bitmap_info *in_format, const struct bitmap_info *out_format, BYTE *data0,
        DWORD data0_size, BYTE *data1, DWORD data1_size, BYTE *expected_data)
{
    struct bitmap_info format;
    BYTE data[1024];
    LRESULT lr;
    HIC hic;

    hic = ICOpen(mmioFOURCC('V','I','D','C'), mmioFOURCC('M','S','V','C'), ICMODE_DECOMPRESS);
    ok_(__FILE__, line)(!!hic, "got hic %p.\n", hic);

    format = *in_format;
    format.header.biSizeImage = data0_size;
    lr = ICDecompressBegin(hic, &format, out_format);
    ok_(__FILE__, line)(lr == ICERR_OK, "got lr %Id.\n", lr);

    format = *in_format;
    format.header.biSizeImage = data0_size;
    memset(data, 0, sizeof(data));
    lr = ICDecompress(hic, 0, &format.header, data0, (BITMAPINFOHEADER *)&out_format->header, data);
    ok_(__FILE__, line)(lr == ICERR_OK, "got lr %Id.\n", lr);

    check_image(line, 0, check_pixel, out_format, data, expected_data);

    format = *in_format;
    format.header.biSizeImage = data1_size;
    lr = ICDecompress(hic, 0, &format.header, data1, (BITMAPINFOHEADER *)&out_format->header, data);
    ok_(__FILE__, line)(lr == ICERR_OK, "got lr %Id.\n", lr);

    check_image(line, 1, check_pixel, out_format, data, expected_data);

    ICDecompressEnd(hic);
    ICClose(hic);
}

static void test_ICDecompress(void)
{
    BYTE frame0_data8[] =
    {
        /* Skip 1 block. */
        0x01, 0x84,
        /* 8-color block. */
        0x55, 0xaa,
        0x01,
        0x02,
        0x04,
        0x08,
        0x10,
        0x20,
        0x40,
        0x80,
        /* 2-color block. */
        0xaa, 0x55,
        0x00,
        0xff,
        /* 1-color block. */
        0xff, 0x80,
    };
    BYTE frame1_data8[] =
    {
        /* Skip 4 blocks. */
        0x04, 0x84,
    };
    BYTE frame0_data16[] =
    {
        /* Skip 1 block. */
        0x01, 0x84,
        /* 8-color block. */
        0xaa, 0x55,
        0x01, 0x80,
        0x10, 0x00,
        0x20, 0x00,
        0x00, 0x02,
        0x00, 0x04,
        0x00, 0x40,
        0x00, 0x00,
        0xff, 0x7f,
        /* 2-color block. */
        0xaa, 0x55,
        0x00, 0x00,
        0xff, 0x7f,
        /* 1-color block. */
        0xff, 0xff,
    };
    BYTE frame1_data16[] =
    {
        /* Skip 4 blocks. */
        0x04, 0x84,
    };
    BYTE expected_data8[] =
    {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,

        0x01, 0x02, 0x04, 0x08,
        0x01, 0x02, 0x04, 0x08,
        0x20, 0x10, 0x80, 0x40,
        0x20, 0x10, 0x80, 0x40,

        0xff, 0x00, 0xff, 0x00,
        0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff,

        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
    };
    BYTE expected_data555[] =
    {
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,

        0x10, 0x00,  0x01, 0x80,  0x00, 0x02,  0x20, 0x00,
        0x10, 0x00,  0x01, 0x80,  0x00, 0x02,  0x20, 0x00,
        0x00, 0x04,  0x00, 0x40,  0x00, 0x00,  0xff, 0x7f,
        0x00, 0x04,  0x00, 0x40,  0x00, 0x00,  0xff, 0x7f,

        0xff, 0x7f,  0x00, 0x00,  0xff, 0x7f,  0x00, 0x00,
        0xff, 0x7f,  0x00, 0x00,  0xff, 0x7f,  0x00, 0x00,
        0x00, 0x00,  0xff, 0x7f,  0x00, 0x00,  0xff, 0x7f,
        0x00, 0x00,  0xff, 0x7f,  0x00, 0x00,  0xff, 0x7f,

        0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,
        0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,
        0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,
        0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,  0xff, 0x7f,
    };
    BYTE expected_data565[] =
    {
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,
        0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,

        0x10, 0x00,  0x01, 0x00,  0x00, 0x04,  0x40, 0x00,
        0x10, 0x00,  0x01, 0x00,  0x00, 0x04,  0x40, 0x00,
        0x00, 0x08,  0x00, 0x80,  0x00, 0x00,  0xdf, 0xff,
        0x00, 0x08,  0x00, 0x80,  0x00, 0x00,  0xdf, 0xff,

        0xdf, 0xff,  0x00, 0x00,  0xdf, 0xff,  0x00, 0x00,
        0xdf, 0xff,  0x00, 0x00,  0xdf, 0xff,  0x00, 0x00,
        0x00, 0x00,  0xdf, 0xff,  0x00, 0x00,  0xdf, 0xff,
        0x00, 0x00,  0xdf, 0xff,  0x00, 0x00,  0xdf, 0xff,

        0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,
        0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,
        0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,
        0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,  0xdf, 0xff,
    };
    BYTE expected_data24[] =
    {
        0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,
        0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,
        0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,
        0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,  0x00, 0x00, 0x00,

        0x80, 0x00, 0x00,  0x08, 0x00, 0x00,  0x00, 0x80, 0x00,  0x00, 0x08, 0x00,
        0x80, 0x00, 0x00,  0x08, 0x00, 0x00,  0x00, 0x80, 0x00,  0x00, 0x08, 0x00,
        0x00, 0x00, 0x08,  0x00, 0x00, 0x80,  0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,
        0x00, 0x00, 0x08,  0x00, 0x00, 0x80,  0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,

        0xf8, 0xf8, 0xf8,  0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,  0x00, 0x00, 0x00,
        0xf8, 0xf8, 0xf8,  0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,  0x00, 0x00, 0x00,
        0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,  0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,
        0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,  0x00, 0x00, 0x00,  0xf8, 0xf8, 0xf8,

        0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,
        0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,
        0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,
        0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,  0xf8, 0xf8, 0xf8,
    };

    check_ICDecompress(check_pixel_rgb8, &format_cram8, &format_rgb8, frame0_data8,
            sizeof(frame0_data8), frame1_data8, sizeof(frame1_data8), expected_data8);
    check_ICDecompress(check_pixel_rgb555, &format_cram16, &format_rgb555, frame0_data16,
            sizeof(frame0_data16), frame1_data16, sizeof(frame1_data16), expected_data555);
    check_ICDecompress(check_pixel_rgb565, &format_cram16, &format_rgb565, frame0_data16,
            sizeof(frame0_data16), frame1_data16, sizeof(frame1_data16), expected_data565);
    check_ICDecompress(check_pixel_rgb24, &format_cram16, &format_rgb24, frame0_data16,
            sizeof(frame0_data16), frame1_data16, sizeof(frame1_data16), expected_data24);
}

START_TEST(msvidc32)
{
    test_ICDecompressQuery();
    test_ICDecompress();
}
