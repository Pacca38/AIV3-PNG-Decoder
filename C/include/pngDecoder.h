#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <linkedLists.h>

typedef struct chunk
{
    unsigned int data_length;
    unsigned char *chunk_type;
    unsigned char *chunk_data;
    unsigned int crc;
} chunk_t;

typedef struct ihdr_info
{
    unsigned int width;
    unsigned int height;
    unsigned int bit_depth;
    unsigned int color_type;
    unsigned int compression_method;
    unsigned int filter_method;
    unsigned int interlace_method;
} ihdr_info_t;

typedef struct chunk_item
{
    struct list_node node;
    chunk_t *chunk;
} chunk_list_t;

static chunk_list_t *chunk_item_new(chunk_t *chunk)
{
    chunk_list_t *item = (chunk_list_t *)malloc(sizeof(chunk_list_t));
    if (!item)
    {
        return NULL;
    }
    item->chunk = chunk;
    return item;
}

static size_t get_file_length(FILE **file)
{
    if (!file || !*file) return 0;

    fseek(*file, 0, SEEK_END);
    size_t size = ftell(*file);
    rewind(*file);
    return size;
}

static void print_byte_stream(unsigned char *data, size_t file_size)
{
    if (!data) return;
    for (size_t i = 0; i < file_size; i++)
    {
        printf("%x ", data[i]);
        if (i % 16 == 15)
        {
            printf("\n");
        }
    }
    printf("\n");
}

static unsigned int data_to_int(unsigned char *data, size_t data_length)
{
    if (data_length > 4) return 0;

    unsigned int value = 0;
    unsigned int multiplier = 1;
    for (int i = data_length - 1; i >= 0; i--)
    {
        unsigned char nibble = data[i];
        value += nibble * multiplier;
        multiplier *= 256;
    }
    return value;
}

static chunk_t *read_chunk(FILE **file)
{
    //Allocate chunk struct memory
    chunk_t *ret = (chunk_t *)malloc(sizeof(chunk_t));
    if (!ret) return NULL;

    //Get data_length
    unsigned char *four_byte_data = (unsigned char *)malloc(4);
    if (!four_byte_data)
    {
        free(ret);
        return NULL;
    }
    fread(four_byte_data, 4, 1, *file);
    ret->data_length = data_to_int(four_byte_data, 4);

    //Get chunk_type
    ret->chunk_type = (unsigned char *)malloc(4);
    if (!ret->chunk_type)
    {
        free(ret);
        free(four_byte_data);
        return NULL;
    }
    fread(four_byte_data, 4, 1, *file);
    memcpy(ret->chunk_type, four_byte_data, 4);

    //Get chunk_data
    unsigned char *chunk_data_from_file = (unsigned char *)malloc(ret->data_length);
    if (!chunk_data_from_file)
    {
        free(ret);
        free(four_byte_data);
        return NULL;
    }
    ret->chunk_data = (unsigned char *)malloc(ret->data_length);
    if (!ret->chunk_data)
    {
        free(ret);
        free(four_byte_data);
        free(chunk_data_from_file);
        return NULL;
    }
    fread(chunk_data_from_file, ret->data_length, 1, *file);
    memcpy(ret->chunk_data, chunk_data_from_file, ret->data_length);

    //Get and check CRC
    fread(four_byte_data, 4, 1, *file);
    unsigned int expected_crc = data_to_int(four_byte_data, 4);
    unsigned int exported_crc = crc32(crc32(0, ret->chunk_type, 4), ret->chunk_data, ret->data_length);
    if (expected_crc != exported_crc)
    {
        printf("Chunk checksum failed {%u} != {%u}\n", expected_crc, exported_crc);
        free(ret);
        free(four_byte_data);
        free(chunk_data_from_file);
        return NULL;
    }
    ret->crc = expected_crc;

    free(four_byte_data);
    free(chunk_data_from_file);
    return ret;
}

static void print_chunk_info(chunk_t *chunk)
{
    printf("Chunk name: %s\n", chunk->chunk_type);
    printf("Chunk data size: %u\n", chunk->data_length);
    printf("Chunk CRC: %u\n", chunk->crc);
    print_byte_stream(chunk->chunk_data, chunk->data_length);
}

static ihdr_info_t *parse_ihdr(chunk_t *ihdr)
{
    unsigned char *four_byte_data = (unsigned char *)malloc(4);
    if (!four_byte_data)
    {
        return NULL;
    }
    ihdr_info_t *infos = (ihdr_info_t *)malloc(sizeof(ihdr_info_t));
    if (!infos)
    {
        return NULL;
    }

    int count = 0;

    //Parsing data
    for (int i = 0; i < 4; i++)
    {
        four_byte_data[i] = ihdr->chunk_data[count++];
    }
    infos->width = data_to_int(four_byte_data, 4);
    for (int i = 0; i < 4; i++)
    {
        four_byte_data[i] = ihdr->chunk_data[count++];
    }
    infos->height = data_to_int(four_byte_data, 4);

    infos->bit_depth = ihdr->chunk_data[count++];

    infos->color_type = ihdr->chunk_data[count++];

    infos->compression_method = ihdr->chunk_data[count++];

    infos->filter_method = ihdr->chunk_data[count++];

    infos->interlace_method = ihdr->chunk_data[count++];

    //Sanity checks
    if (infos->compression_method != 0)
    {
        printf("Invalid compression method [%u]", infos->compression_method);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->filter_method != 0)
    {
        printf("Invalid filter method [%u]", infos->filter_method);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->color_type != 6)
    {
        printf("Only true color alpha is supported [%u]", infos->color_type);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->bit_depth != 8)
    {
        printf("Only bit depth of 8 is supported [%u]", infos->bit_depth);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->interlace_method != 0)
    {
        printf("Adam7 interlace not supported [%u]", infos->interlace_method);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->width == 0 || infos->height == 0)
    {
        printf("Invalid image size [%u, %u]", infos->width, infos->height);
        free(infos);
        free(four_byte_data);
        return NULL;
    }

    free(four_byte_data);
    return infos;
}

static int paeth_predictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc)
        return a;
    else if (pb <= pc)
        return b;
    else
        return c;
}

static int recon_a(unsigned char *pixels, unsigned int r, unsigned int c, int stride, int bytes_per_pixels)
{
    if (c >= bytes_per_pixels)
        return pixels[r * stride + c - bytes_per_pixels];
    else
        return 0;
}

static int recon_b(unsigned char *pixels, unsigned int r, unsigned int c, int stride, int bytes_per_pixels)
{
    if (r > 0)
        return pixels[(r - 1) * stride + c];
    else
        return 0;
}

static int recon_c(unsigned char *pixels, unsigned int r, unsigned int c, int stride, int bytes_per_pixels)
{
    if (c >= bytes_per_pixels && r > 0)
        return pixels[(r - 1) * stride + c - bytes_per_pixels];
    else
        return 0;
}

static unsigned char *parse_idat(chunk_t *idat, ihdr_info_t *ihdr_infos)
{
    int bytes_per_pixels = 4;
    int stride = ihdr_infos->width * bytes_per_pixels;

    //Decompress pixels data
    unsigned long uncompressed_size = ihdr_infos->height * (1 + stride);
    unsigned long uncompressed_max_size = compressBound(idat->data_length);
    unsigned char *uncompressed_idat = (unsigned char *)malloc(uncompressed_size);
    if (!uncompressed_idat)
    {
        return NULL;
    }
    int result = uncompress(uncompressed_idat, &uncompressed_size, idat->chunk_data, uncompressed_max_size);
    if (result != Z_OK)
    {
        printf("Unable to uncompress IDAT: error %d\n", result);
        free(uncompressed_idat);
        return NULL;
    }

    //Assign pixels memory
    unsigned char *pixels = (unsigned char * )malloc(ihdr_infos->height * stride);
    if (!pixels)
    {
        return NULL;
    }
    int count = 0;

    //De-filtering of uncompressed pixels data
    int i = 0;
    int filter_type = 0;
    int filt_x = 0;
    int recon_x = 0;
    for (unsigned int r = 0; r < ihdr_infos->height; r++)
    {
        filter_type = uncompressed_idat[i++];
        for (unsigned int c = 0; c < stride; c++)
        {
            filt_x = uncompressed_idat[i++];
            switch (filter_type)
            {
                case 0:
                    recon_x = filt_x;
                    break;
                case 1:
                    recon_x = filt_x + recon_a(pixels, r, c, stride, bytes_per_pixels);
                    break;
                case 2:
                    recon_x = filt_x + recon_b(pixels, r, c, stride, bytes_per_pixels);
                    break;
                case 3:
                    recon_x = filt_x + (recon_a(pixels, r, c, stride, bytes_per_pixels) + recon_b(pixels, r, c, stride, bytes_per_pixels)) / 2;
                    break;
                case 4:
                    recon_x = filt_x + paeth_predictor(recon_a(pixels, r, c, stride, bytes_per_pixels), recon_b(pixels, r, c, stride, bytes_per_pixels), recon_c(pixels, r, c, stride, bytes_per_pixels));
                    break;
                default:
                    recon_x = filt_x;
                    break;
            }
            pixels[count++] = recon_x;
        }
    }
    return pixels;
}

unsigned char *parse_png(const char *string, int *width, int *height, int *channels)
{
    unsigned char *pixels = NULL;

    //Open .png
    FILE *png;
    png = fopen(string, "rb");
    if (!png)
    {
        printf("Error opening file");
        return NULL;
    }
    size_t file_size = get_file_length(&png);

    //Check if first 8 byte are the PNG signature
    unsigned char *signature_data = (unsigned char *)malloc(8);
    if (!signature_data)
    {
        printf("Could not load memory");
        goto end;
    }
    fread(signature_data, 8, 1, png);
    unsigned char png_signature[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    if (memcmp(signature_data, png_signature, 8) != 0)
    {
        printf("Invalid PNG signature");
        goto end;
    }
    free(signature_data);

    //Reading chunks
    chunk_list_t *chunk_list = NULL;
    while (1)
    {
        chunk_t *chunk = read_chunk(&png);
        if (!chunk)
        {
            break;
        }
        list_append((struct list_node **)&chunk_list, (struct list_node *)chunk_item_new(chunk));
        unsigned char iend_signature[] = {0x49, 0x45, 0x4e, 0x44};
        if (memcmp(iend_signature, chunk->chunk_type, 4) == 0)
        {
            break;
        }
    }

    /*
    chunk_list_t *chunk_list_cpy = chunk_list;
    while (chunk_list_cpy)
    {
        print_chunk_info(chunk_list_cpy->chunk);
        chunk_list_cpy = (chunk_list_t *)chunk_list_cpy->node.next;
    }
    */

    //Parse IHDR chunk
    ihdr_info_t *ihdr_infos = parse_ihdr(((chunk_list_t *)list_pop((struct list_node **)&chunk_list))->chunk);
    if (!ihdr_infos)
    {
        goto end;
    }

    //Look for IDAT chunk
    chunk_list_t *node = (chunk_list_t *)list_pop((struct list_node **)&chunk_list);
    chunk_t *idat = NULL;
    unsigned char idat_signature[] = {0x49, 0x44, 0x41, 0x54};
    while (node)
    {
        chunk_t *chunk = node->chunk;
        if (memcmp(idat_signature, chunk->chunk_type, 4) == 0)
        {
            idat = chunk;
            break;
        }
        node = (chunk_list_t *)list_pop((struct list_node **)&chunk_list);
    }
    if (!idat)
    {
        printf("Could not find pixels data");
        goto end;
    }

    //Parse IDAT chunk
    pixels = parse_idat(idat, ihdr_infos);
    if (!pixels)
    {
        goto end;
    }

    *width = ihdr_infos->width;
    *height = ihdr_infos->height;
    *channels = 4;

    /*
    for (size_t i = 0; i < 1000; i = i + 4)
    {
        printf("%u %u %u %u\n", pixels[i], pixels[i+1], pixels[i+2], pixels[i+3]);
    }
    */
    free(chunk_list);
end:
    fclose(png);
    return pixels;
}