/*
 * in_ps.cpp -- read PS files using GS
 * by pts@fazekas.hu at Tue Sep 30 12:33:11 CEST 2003
 */

#ifdef __GNUC__
#pragma implementation
#endif

#include "image.hpp"

#if USE_IN_PS

#include "error.hpp"
#include "gensio.hpp"
#include <string.h> /* memchr() */
#include <stdio.h> /* printf() */

/* float DPI ?? */
/* !! PDF -r... */
/* !! -r144 and scale back..., also for PDF */

/** Ugly multiple inheritance. !! unify with PNG, TIFF etc. */
class HelperE: public Filter::NullE, public Filter::PipeE {
 public:
  HelperE(char const*filter_cmd, char const*mainfn): Filter::NullE(), Filter::PipeE(*(Filter::NullE*)this, filter_cmd, (slendiff_t)mainfn) {
    /* ^^^ (slendiff_t) is unsafe cast */
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

static char const cmd[]=GS " -r00072 -q -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -dLastPage=1 -sDEVICE=pnmraw -dDELAYSAFER -dBATCH -dNOPAUSE -sOutputFile=%D -s_IFN=%S -- %*";

static Image::Sampled *in_ps_reader_low(Image::Loader::UFD* ufd, char const*bboxline, SimBuffer::Flat const& hints) {
  SimBuffer::B mainfn;
  if (!Files::find_tmpnam(mainfn)) Error::sev(Error::EERROR) << "in_ps_reader" << ": tmpnam() failed" << (Error*)0;
  mainfn.term0(); Files::tmpRemoveCleanup(mainfn());
  FILE *f=fopen(mainfn(),"w");
  fprintf(f, "%s/setpagedevice/pop load def\n", bboxline); /* Imp: /a4/letter etc. */
  fprintf(f, "_IFN (r) file cvx exec\n"); /* Dat: doesn't rely on GS to
    recognise EPSF-x.y comment, so works with both old and new gs */
  // ^^^ !! DOS EPSF etc. instead of exec/run
  fclose(f);
  // Error::sev(Error::EERROR) << "Cannot load PS images yet." << (Error*)0;
  /* Dat: -dLastPage=1 has no effect, but we keep it for PDF compatibility */
  /* !! keep only 1st page, without setpagedevice for PS files */
  /* Dat: -dSAFER won't let me open the file with `/' under ESP Ghostscript 7.05.6 (2003-02-05) */
  /* Imp: win9X command line too long? */
  SimBuffer::B cmd(GS " -r72 -q -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -dLastPage=1 -sDEVICE=pnmraw -dDELAYSAFER -dBATCH -dNOPAUSE -sOutputFile=%D -s_IFN=%S ");
  { char const *p=hints(), *r;
    /* Dat: hints ends by ',' '\0' */
    // Files::FILEW(stdout) << hints << ";;\n";
    while (*p!=',') p++; /* Dat: safe, because hints is assumed to start and end by ',' */
    while (1) {
      assert(*p==',');
      if (*++p=='\0') break;
      if (p[0]=='g' && p[1]=='s' && p[2]=='=') {
        r=p+=3;
        while (*p!=',') p++;
        cmd.vi_write(r, p-r);
        cmd << ' ';
      } else {
        while (*p!=',') p++;
      }
    }
  }
  cmd << "-- %*";
  HelperE helper(cmd.term0()(), mainfn()); /* Run external process GS */
  Filter::UngetFILED* ufdd=(Filter::UngetFILED*)ufd;
  int i=ufdd->vi_getcc();
  if (i<0) Error::sev(Error::EERROR) << "in_ps_reader: Empty PostScript file." << (Error*)0; /* should never happen */
  ((Filter::PipeE*)&helper)->vi_putcc(i);
  Encoder::writeFrom(*(Filter::PipeE*)&helper, *ufdd);
  ((Filter::PipeE*)&helper)->vi_write(0,0); /* Signal EOF */
  remove(mainfn());
  return helper.getImg();
}

static Image::Sampled *in_ps_reader(Image::Loader::UFD* ufd, SimBuffer::Flat const& hints) {
  return in_ps_reader_low(ufd, "", hints);
}

static Image::Sampled *in_eps_reader(Image::Loader::UFD* ufd, SimBuffer::Flat const& hints) {
  double llx=0.0, lly=0.0, urx=0.0, ury=0.0;
  Filter::UngetFILED* ufdd=(Filter::UngetFILED*)ufd;
  /* ^^^ SUXX: no warning for ufdd=ufdd */
  /* SUXX: valgrind, checkergcc: no indication of segfault due to stack overflow inside fgetc() */
  SimBuffer::B line; /* Imp: limit for max line length etc. */
  #if 0
  while ((line.clearFree(), ufdd->appendLine(line), line)) {
    line.term0();
    printf("line: %s", line());
  }
  #endif
  slen_t line0ofs;
  int had=0;
  while ((line0ofs=line.getLength(), ufdd->appendLine(line), line0ofs!=line.getLength())) {
    char const *thisline=line()+line0ofs;
    line.term0();
    // printf("line: %s", thisline);
    if (thisline[0]=='\n' || thisline[0]=='\r') continue; /* empty line */
    if (thisline[0]=='%' && thisline[1]=='!') continue; /* %!PS-... */
    if (thisline[0]!='%' || thisline[1]!='%') break; /* non-ADSC comment */
         if (had<3 && 4==sscanf(thisline+2, "ExactBoundingBox:%lg%lg%lg%lg", &llx, &lly, &urx, &ury)) had=3;
    else if (had<2 && 4==sscanf(thisline+2, "HiResBoundingBox:%lg%lg%lg%lg", &llx, &lly, &urx, &ury)) had=2;
    else if (had<1 && 4==sscanf(thisline+2, "BoundingBox:%lg%lg%lg%lg", &llx, &lly, &urx, &ury)) had=1;
    /* Dat: finds MetaPost hiresbbox after %%EndComments */
    // printf("line: %s", line()+line0ofs);
  }
  ufdd->unread(line(), line.getLength()); line.clearFree();
  char bboxline[300];
  if (had!=0) {
    // fprintf(stderr, "bbox=[%"PTS_CFG_PRINTFGLEN"g %"PTS_CFG_PRINTFGLEN"g %"PTS_CFG_PRINTFGLEN"g %"PTS_CFG_PRINTFGLEN"g]\n", llx, lly, urx, ury);
    sprintf(bboxline, "%"PTS_CFG_PRINTFGLEN"g %"PTS_CFG_PRINTFGLEN"g translate\n"
      "<</PageSize[%"PTS_CFG_PRINTFGLEN"g %"PTS_CFG_PRINTFGLEN"g]>>setpagedevice\n", -llx, -lly, urx-llx, ury-lly);
  } else {
    Error::sev(Error::WARNING) << "in_eps_reader: missing EPS bbox" << (Error*)0;
    bboxline[0]='\0';
  }
  
  return in_ps_reader_low(ufd, bboxline, hints);
}

static Image::Loader::reader_t in_ps_checker(char buf[Image::Loader::MAGIC_LEN], char [Image::Loader::MAGIC_LEN], SimBuffer::Flat const&, Image::Loader::UFD*) {
  if (0!=memcmp(buf,"%!PS-Adobe-",11)) return 0;
  char const *p=buf+11, *pend=buf+Image::Loader::MAGIC_LEN;
  while (p!=pend && *p!=' ' && *p!='\t') p++;
  while (p!=pend && (*p==' ' || *p=='\t')) p++;
  /* Imp: option to accept BoundingBox for non-EPS PS */
  if (0!=strcmp(p,"EPSF-"),5) return in_eps_reader;
  return in_ps_reader;
}

#else
#define in_ps_checker (Image::Loader::checker_t)NULLP
#endif /* USE_IN_PS */

Image::Loader in_ps_loader = { "PS", in_ps_checker, 0 };
