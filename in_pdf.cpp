/*
 * in_pdf.cpp -- read PDF files using GS
 * by pts@fazekas.hu at Mon Sep 29 17:19:39 CEST 2003
 */

#ifdef __GNUC__
#pragma implementation
#endif

#include "image.hpp"

#if USE_IN_PDF

#include "error.hpp"
#include "gensio.hpp"
#include <string.h> /* memchr() */
#include <stdio.h> /* printf() */

/** Ugly multiple inheritance. !! unify with PNG, TIFF etc. */
class HelperE: public Filter::NullE, public Filter::PipeE {
 public:
  HelperE(char const*filter_cmd): Filter::NullE(), Filter::PipeE(*(Filter::NullE*)this, filter_cmd) {
    // GenBuffer::Writable &out_, char *pipe_tmpl, slendiff_t i=0)
  }
  virtual void vi_copy(FILE *f) {
    // img=Image::load("-", SimBuffer::B(), (Image::filep_t)f, (char const*)"PNM");
    /* fclose(f); */
    Filter::UngetFILED ufd((char const*)NULLP, f, Filter::UngetFILED::CM_closep|Filter::UngetFILED::CM_keep_stdinp);
    img=Image::load((Image::Loader::UFD*)&ufd, SimBuffer::B(), (char const*)"PNM");
  }
  inline Image::Sampled *getImg() const { return img; }
 protected:
  Image::Sampled *img;
};

#if OS_COTY==COTY_WIN9X || OS_COTY==COTY_WINNT
#  define GS "gswin32c"
#else
#  define GS "gs"
#endif

static Image::Sampled *in_pdf_reader(Image::Loader::UFD* ufd, SimBuffer::Flat const&) {
  // Error::sev(Error::EERROR) << "Cannot load PDF images yet." << (Error*)0;
  char const* cmd=GS " -q -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -dLastPage=1 -sDEVICE=pnmraw -r72 -dDELAYSAFER -dBATCH -dNOPAUSE -sOutputFile=%D -- %S";
  /* Dat: -dSAFER won't let me open the file with `/' under ESP Ghostscript 7.05.6 (2003-02-05) */
  /* Imp: win9X command line too long? */
  HelperE helper(cmd); /* Run external process GS */
  Encoder::writeFrom(*(Filter::PipeE*)&helper, *(Filter::UngetFILED*)ufd);
  ((Filter::PipeE*)&helper)->vi_write(0,0); /* Signal EOF */
  return helper.getImg();
}

static Image::Loader::reader_t in_pdf_checker(char buf[Image::Loader::MAGIC_LEN], char [Image::Loader::MAGIC_LEN], SimBuffer::Flat const&, Image::Loader::UFD*) {
  return 0==memcmp(buf,"%PDF-",5) ? in_pdf_reader : 0;
}

#else
#define in_pdf_checker (Image::Loader::checker_t)NULLP
#endif /* USE_IN_PDF */

Image::Loader in_pdf_loader = { "PDF", in_pdf_checker, 0 };
