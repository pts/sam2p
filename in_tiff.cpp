/*
 * in_tiff.cpp -- read TIFF (Tag Image File Format) files with tif22pnm
 * by pts@fazekas.hu at Sun Apr 14 14:50:30 CEST 2002
 */
/* Imp: get docs about the TIFF format, and rewrite this from scratch */
/* Imp: use xviff.c */

#ifdef __GNUC__
#pragma implementation
#endif

#include "image.hpp"

#if USE_IN_TIFF

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

static Image::Sampled *in_tiff_reader(Image::filep_t file_, SimBuffer::Flat const&) {
  // Error::sev(Error::ERROR) << "Cannot load TIFF images yet." << (Error*)0;
  // HelperE helper("tifftopnm %S"); /* Cannot extract alpha channel */
  // HelperE helper("tif22pnm -rgba %S"); /* tif22pnm <= 0.07 */
  HelperE helper("tif22pnm -rgba %S pnm:"); /* Wants to seek in the file. */
  Encoder::writeFrom(*(Filter::PipeE*)&helper, (FILE*)file_);
  ((Filter::PipeE*)&helper)->vi_write(0,0); /* Signal EOF */
  return helper.getImg();
}

static Image::Loader::reader_t in_tiff_checker(char buf[Image::Loader::MAGIC_LEN], char [Image::Loader::MAGIC_LEN], SimBuffer::Flat const&) {
  /* MM\x00\x2a: TIFF image data, big-endian
   * II\x2a\x00: TIFF image data, little-endian
   * The second word of TIFF files is the TIFF version number, 42, which has 
   * never changed.  The TIFF specification recommends testing for it.
   */
  return (0==memcmp(buf,"MM\x00\x2a",4) || 0==memcmp(buf,"II\x2a\x00",4)) ? in_tiff_reader : 0;
}

#else
#define in_tiff_checker (Image::Loader::checker_t)NULLP
#endif /* USE_IN_XPM */

Image::Loader in_tiff_loader = { "TIFF", in_tiff_checker, 0 };
