/*
 * in_jpeg.cpp -- read JPEG (JFIF and other) files with djpeg
 * by pts@fazekas.hu at Sun Apr 14 14:50:30 CEST 2002
 */

#ifdef __GNUC__
#pragma implementation
#endif

#include "image.hpp"

#if USE_IN_JPEG

#include "error.hpp"
#include "gensio.hpp"
#include <string.h> /* memchr() */
#include <stdio.h> /* printf() */

/** Ugly multiple inheritance. */
class HelperE: public Filter::NullE, public Filter::PipeE {
 public:
  HelperE(char const*djpeg_cmd): Filter::NullE(), Filter::PipeE(*(Filter::NullE*)this, djpeg_cmd) {
    /* Dat: VC6.0 warning: used in base member initializer list */
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

static Image::Sampled *in_jpeg_reader(Image::filep_t file_, SimBuffer::Flat const&) {
  // Error::sev(Error::ERROR) << "Cannot load JPEG images yet." << (Error*)0;
  HelperE helper("djpeg"); /* Run external process `djpeg' to convert JPEG -> PNM */
  Encoder::writeFrom(*(Filter::PipeE*)&helper, (FILE*)file_);
  ((Filter::PipeE*)&helper)->vi_write(0,0); /* Signal EOF */
  return helper.getImg();
}

static Image::Loader::reader_t in_jpeg_checker(char buf[Image::Loader::MAGIC_LEN], char [Image::Loader::MAGIC_LEN], SimBuffer::Flat const& loadHints) {
  return (0==memcmp(buf, "\xff\xd8", 2)) && loadHints.findFirst((char const*)",asis,",6)==loadHints.getLength()
         ? in_jpeg_reader : 0;
}

#else
#define in_jpeg_checker (Image::Loader::checker_t)NULLP
#endif /* USE_IN_XPM */

Image::Loader in_jpeg_loader = { "JPEG", in_jpeg_checker, 0 };
