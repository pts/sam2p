/*
 * in_png.cpp -- read PNG (Portable Network Graphics, PNG is Not GIF) files with pngtopnm
 * by pts@fazekas.hu at Sun Apr 14 14:50:30 CEST 2002
 */

#ifdef __GNUC__
#pragma implementation
#endif

#include "image.hpp"

#if USE_IN_PNG

#include "error.hpp"
#include "gensio.hpp"
#include <string.h> /* memchr() */
#include <stdio.h> /* printf() */

/** Ugly multiple inheritance. */
class HelperE: public Filter::NullE, public Filter::PipeE {
 public:
  HelperE(char const*djpeg_cmd): Filter::NullE(), Filter::PipeE(*(Filter::NullE*)this, djpeg_cmd) {
    // GenBuffer::Writable &out_, char *pipe_tmpl, slendiff_t i=0)
  }
  virtual void vi_copy(FILE *f) {
    img=Image::load("PNM", (Image::filep_t)f, SimBuffer::B());
    fclose(f);
  }
  inline Image::Sampled *getImg() const { return img; }
 protected:
  Image::Sampled *img;
};

static Image::Sampled *in_png_reader(Image::filep_t file_, SimBuffer::Flat const&) {
  // Error::sev(Error::ERROR) << "Cannot load PNG images yet." << (Error*)0;
  HelperE helper("(pngtopnm <%S && pngtopnm -alpha <%S) >%D"); /* Run external process `djpeg' to convert JPEG -> PNM */
  Encoder::writeFrom(*(Filter::PipeE*)&helper, (FILE*)file_);
  ((Filter::PipeE*)&helper)->vi_write(0,0); /* Signal EOF */
  return helper.getImg();
}

static Image::Loader::reader_t in_png_checker(char buf[Image::Loader::MAGIC_LEN], char [Image::Loader::MAGIC_LEN], SimBuffer::Flat const&) {
  return 0==memcmp(buf,"\211PNG\r\n\032\n",8) ? in_png_reader : 0;
}

#else
#define in_png_checker (Image::Loader::checker_t)NULLP
#endif /* USE_IN_XPM */

Image::Loader in_png_loader = { "PNG", in_png_checker, 0 };
