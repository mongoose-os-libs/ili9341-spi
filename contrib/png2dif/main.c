/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos.h"
#include "mgos_ili9341.h"
#include "upng.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

int png2dif(char *png_filename, char *dif_filename) {
  upng_t *       upng;
  uint16_t *     data;
  uint16_t *     dif_buf_p;
  const uint8_t *png_buf;
  const uint8_t *png_buf_p;
  uint16_t       width, height;
  uint32_t       out;
  uint8_t        out_byte;
  int            ret = -1;
  int            fd;
  int            xx, yy;

  if (!(upng = upng_new_from_file(png_filename))) {
    LOG(LL_ERROR, ("Can't read %s", png_filename));
    goto exit;
  }

  upng_decode(upng);
  if (upng_get_error(upng) != UPNG_EOK) {
    LOG(LL_ERROR, ("PNG decode error"));
    goto exit;
  }
  if (upng_get_format(upng) != UPNG_RGB8) {
    LOG(LL_ERROR, ("PNG is not in RGB8 format"));
    goto exit;
  }
  // Do stuff with upng data
  width  = upng_get_width(upng);
  height = upng_get_height(upng);
  LOG(LL_INFO, ("%s: w=%d h=%d size=%d bpp=%d bitdepth=%d pixelsize=%d", png_filename, width, height, upng_get_size(upng), upng_get_bpp(upng), upng_get_bitdepth(upng), upng_get_pixelsize(upng)));
  png_buf = upng_get_buffer(upng);

  data = (uint16_t *)calloc(width * height, sizeof(uint16_t));
  memset(data, 0, width * height * sizeof(uint16_t));
  dif_buf_p = data;

  if (!data) {
    LOG(LL_ERROR, ("Could not create DIF data struct (%d bytes)", height * width));
    goto exit;
  }
  png_buf_p = png_buf;
  for (yy = 0; yy < height; yy++) {
    for (xx = 0; xx < width; xx++) {
      int      r, g, b;
      uint16_t output_rgb565;
      switch (upng_get_format(upng)) {
      case UPNG_RGB8: r = *png_buf_p++; g = *png_buf_p++; b = *png_buf_p++;
        output_rgb565   = htons(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
        break;

      case UPNG_RGBA8: r = *png_buf_p++; g = *png_buf_p++; b = *png_buf_p++; png_buf_p++;
        output_rgb565    = htons(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
        break;

      default:
        LOG(LL_ERROR, ("Unknown PNG format (%d)", upng_get_format(upng)));
        goto exit;
      }
      *dif_buf_p++ = output_rgb565;
    }
  }

  fd = open(dif_filename, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (!fd) {
    LOG(LL_ERROR, ("Could not open %s for writing", dif_filename));
    goto exit;
  }
  write(fd, "DIF", 3);
  out_byte = 1;
  write(fd, (void *)&out_byte, 1);
  out = htonl(width);
  write(fd, (void *)&out, 4);
  out = htonl(height);
  write(fd, (void *)&out, 4);
  out = 0;
  write(fd, (void *)&out, 4);
  write(fd, (void *)data, width * height * 2);
  close(fd);
  LOG(LL_INFO, ("%s: w=%d h=%d datasize=%lu", dif_filename, width, height, width * height * sizeof(uint16_t)));

  ret = 0;
exit:
  if (data) {
    free(data);
  }
  if (upng) {
    upng_free(upng);
  }

  return ret;
}

int main(int argc, char **argv, char **env) {
  char *i_value = NULL;
  char *o_value = NULL;
  int   c;

  opterr = 0;

  while ((c = getopt(argc, argv, "i:o:")) != -1) {
    switch (c) {
    case 'i':
      i_value = optarg;
      break;

    case 'o':
      o_value = optarg;
      break;

    default:
      abort();
    }
  }
  if (!i_value || !o_value) {
    printf("Usage: -i <input.png> -o <output.dif>\r\n");
    return -1;
  }

  return png2dif(i_value, o_value);
}
