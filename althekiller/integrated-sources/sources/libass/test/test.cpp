#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>

extern "C" {
#include <ass.h>
}

#include <png.h>

struct image_t {
  int width, height, stride;
  unsigned char* buffer; // RGB24
};

ass_library_t* ass_library;
ass_renderer_t* ass_renderer;

static void write_png(char *fname, image_t* img)
{
    FILE * fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte **row_pointers;
    int k;

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    fp = NULL;

    if (setjmp(png_ptr->jmpbuf)) {
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	return;
    }

    fp = fopen (fname, "wb");
    if (fp == NULL) {
	printf("PNG Error opening %s for writing!\n", fname);
	return;
    }
        
    png_init_io(png_ptr, fp);
    png_set_compression_level(png_ptr, 0);

    png_set_IHDR(png_ptr, info_ptr, img->width, img->height,
		 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);
        
    png_set_bgr(png_ptr);

    row_pointers = (png_byte **)malloc(img->height * sizeof(png_byte*));
    for (k = 0; k < img->height; k++)
	row_pointers[k] = img->buffer + img->stride * k;

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(row_pointers);

    fclose (fp);
}

static void init(int frame_w, int frame_h) {
  ass_library = ass_library_init();
  if (!ass_library) throw std::runtime_error("ass_library_init failed");
  
  ass_set_fonts_dir(ass_library, "");
  ass_set_extract_fonts(ass_library, 0);
  ass_set_style_overrides(ass_library, NULL);

  ass_renderer = ass_renderer_init(ass_library);
  if (!ass_renderer) throw std::runtime_error("ass_renderer_init failed");
  
  ass_set_frame_size(ass_renderer, frame_w, frame_h);
  ass_set_margins(ass_renderer, 0, 0, 0, 0);
  ass_set_use_margins(ass_renderer, 0);
  ass_set_font_scale(ass_renderer, 1.);
  ass_set_fonts(ass_renderer, NULL, "Sans");
}

static image_t* gen_image(int width, int height) {
  image_t* img = new image_t;
  img->width = width;
  img->height = height;
  img->stride = width * 3;
  img->buffer = (unsigned char*)calloc(1, height * width * 3);
  for (int i = 0; i < height * width * 3; ++i)
   img->buffer[i] = (i/3/50) % 100;
  return img;
}

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

static void blend_single(image_t* frame, ass_image_t* img) {
  unsigned char opacity = 255 - _a(img->color);
  unsigned char r = _r(img->color);
  unsigned char g = _g(img->color);
  unsigned char b = _b(img->color);
  
  unsigned char* src;
  unsigned char* dst;

  src = img->bitmap;
  dst = frame->buffer + img->dst_y * frame->stride + img->dst_x * 3;
  for (int y = 0; y < img->h; ++y) {
    for (int x = 0; x < img->w; ++x) {
      unsigned k = ((unsigned)src[x]) * opacity / 255;
      // possible endianness problems
      dst[x*3]   = (k*b + (255-k)*dst[x*3])   / 255;
      dst[x*3+1] = (k*g + (255-k)*dst[x*3+1]) / 255;
      dst[x*3+2] = (k*r + (255-k)*dst[x*3+2]) / 255;
    }
    src += img->stride;
    dst += frame->stride;
  }
}

static void blend(image_t* frame, ass_image_t* img) {
  int cnt = 0;
  while (img) {
    blend_single(frame, img);
    ++cnt;
    img = img->next;
  }
  printf("%d images blended\n", cnt);
}

int main(int argc, char* argv[]) {
  const int frame_w = 640;
  const int frame_h = 480;
  
  if (argc < 4) {
    printf("usage: %s <image file> <subtitle file> <time>\n", argv[0]);
    exit(1);
  }
  char* imgfile = argv[1];
  char* subfile = argv[2];
  double tm = strtod(argv[3], 0);

  init(frame_w, frame_h);
  ass_track_t* track = ass_read_file(ass_library, subfile, NULL);
  
  ass_image_t* img = ass_render_frame(ass_renderer, track, (int)(tm * 1000), NULL);
  image_t* frame = gen_image(frame_w, frame_h);
  blend(frame, img);
  write_png(imgfile, frame);

  return 0;
}
