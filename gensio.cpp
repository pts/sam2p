/*
 * gensio.cpp -- IO-specific methods
 * by pts@fazekas.hu at Tue Feb 26 13:28:12 CET 2002
 */

#ifdef __GNUC__
#pragma implementation
#endif

#if 0
extern "C" int errno;
/* Imp: autodetect with autoconf */
extern "C" int lstat(const char *file_name, struct stat *buf);
/* vvv Imp: not in ANSI C, but we cannot emulate it! */
extern "C" int _v_s_n_printf ( char *str, size_t n, const char *format, va_list ap );
#else
#undef __STRICT_ANSI__
#define _BSD_SOURCE 1 /* vsnprintf(); may be emulated with fixup_vsnprintf() */
#define _POSIX_SOURCE 1 /* also popen() */
#define _POSIX_C_SOURCE 2 /* also popen() */
#define _XOPEN_SOURCE_EXTENDED 1 /* Digital UNIX lstat */
#define _XPG4_2 1 /* SunOS 5.7 lstat() */
#undef  _XOPEN_SOURCE /* pacify gcc-3.1 */
#define _XOPEN_SOURCE 1 /* popen() on Digital UNIX */
#endif

#include "gensio.hpp"
#include "error.hpp"
#include <string.h> /* strlen() */
#include <stdarg.h> /* va_list */
#include <unistd.h> /* getpid() */
#include <sys/stat.h> /* struct stat */
#include <stdlib.h> /* getenv() */
#include <errno.h>
#include <signal.h> /* signal() */ /* Imp: use sigaction */

#define USGE(a,b) ((unsigned char)(a))>=((unsigned char)(b))

#if HAVE_PTS_VSNPRINTF /* Both old and c99 work OK */
#  define VSNPRINTF vsnprintf
#else
#  if OBJDEP
#    warning REQUIRES: snprintf.o
#  endif
#  include "snprintf.h"
#  define VSNPRINTF fixup_vsnprintf /* Tested, C99. */
#endif

#if _MSC_VER > 1000
extern "C" int getpid(void);
#endif

static void cleanup(int) {
  Error::cexit(126);
}

void Files::doSignalCleanup() {
  signal(SIGINT, cleanup);
  signal(SIGTERM, cleanup);
#ifdef SIGHUP
  signal(SIGHUP, SIG_IGN);
#endif
  /* Dat: don't do cleanup for SIGQUIT */
}

GenBuffer::Writable& SimBuffer::B::vformat(slen_t n, char const *fmt, va_list ap) {
  /* Imp: test this code in various systems and architectures. */
  /* Dat: vsnprintf semantics are verified in configure AC_PTS_HAVE_VSNPRINTF,
   * and a replacement vsnprintf is provided in case of problems. We don't
   * depend on HAVE_PTS_VSNPRINTF_C99, because C99-vsnprintf is a run-time
   * property.
   *
   * C99 vsnprintf semantics: vsnprintf() always returns a non-negative value:
   *   the number of characters (trailing \0 not included) that would have been
   *   printed if there were enough space. Only the first maxlen (@param)
   *   characters may be modified. The output is terminated by \0 iff maxlen!=0.
   *   NULL @param dststr is OK iff maxlen==0. Since glibc 2.1.
   * old vsnprintf semantics: vsnprintf() returns a non-negative value or -1:
   *   0 if maxlen==0, otherwise: -1 if maxlen is shorter than all the characters
   *   plus the trailing \0, otherwise: the number of characters (trailing \0 not
   *   included) that were printed. Only the first maxlen (@param)
   *   characters may be modified. The output is terminated by \0 iff maxlen!=0.
   *   NULL @param dststr is OK iff maxlen==0.
   */
  if (n>0) { /* avoid problems with old-style vsnprintf */
    char *s; vi_grow2(0, n+1, 0, &s); len-=n+1; /* +1: sprintf appends '\0' */
    const_cast<char*>(beg)[len]='\0'; /* failsafe sentinel */
    slen_t did=VSNPRINTF(s, n+1, fmt, ap);
    if (did>n) did=n;
    /* ^^^ Dat: always true: (unsigned)-1>n, so this works with both old and c99 */
    /* Now: did contains the # chars to append, without trailing '\0' */
    /* Dat: we cannot check for -1, because `did' is unsigned */
    /* Dat: trailer '\0' doesn't count into `did' */
    len+=did;
  }
  return *this;
}
GenBuffer::Writable& SimBuffer::B::vformat(char const *fmt, va_list ap) {
  char dummy, *s;
  slen_t did=VSNPRINTF(&dummy, 1, fmt, ap), n;
  if (did>0) { /* skip if nothing to be appended */
    /* vvv Dat: we cannot check for -1, because `did' is unsigned */
    if ((did+1)!=(slen_t)0) { /* C99 semantics; quick shortcut */
      vi_grow2(0, (n=did)+1, 0, &s); len-=n+1;
      ASSERT_SIDE2(VSNPRINTF(s, n+1, fmt, ap), * 1U==did);
    } else { /* old semantics: grow the buffer incrementally */
      if ((n=strlen(fmt))<16) n=16; /* initial guess */
      while (1) {
        vi_grow2(0, n+1, 0, &s); len-=n+1; /* +1: sprintf appends '\0' */
        const_cast<char*>(beg)[len]='\0'; /* failsafe sentinel */
        did=VSNPRINTF(s, n+1, fmt, ap);
        if ((did+1)!=(slen_t)0) {
          assert(did!=0); /* 0 is caught early in this function */
          assert(did<=n); /* non-C99 semantics */
          break;
        }
        n<<=1;
      }
    }
    len+=did;
  }
  return *this;
}

GenBuffer::Writable& SimBuffer::B::format(slen_t n, char const *fmt, ...) {
  va_list ap;
  PTS_va_start(ap, fmt);
  vformat(n, fmt, ap);
  va_end(ap);
  return *this;
}
GenBuffer::Writable& SimBuffer::B::format(char const *fmt, ...) {
  va_list ap;
  PTS_va_start(ap, fmt);
  vformat(fmt, ap);
  va_end(ap);
  return *this;
}

/* --- */

GenBuffer::Writable& Files::FILEW::vformat(slen_t n, char const *fmt, va_list ap) {
  /* Dat: no vfnprintf :-( */
  SimBuffer::B buf;
  buf.vformat(n, fmt, ap);
  fwrite(buf(), 1, buf.getLength(), f);
  return*this;
}
GenBuffer::Writable& Files::FILEW::vformat(char const *fmt, va_list ap) {
  vfprintf(f, fmt, ap);
  return*this;
}

/* --- */

/** Must be <=32767. Should be a power of two. */
static const slen_t BUFLEN=4096;

void Encoder::vi_putcc(char c) { vi_write(&c, 1); }
int Decoder::vi_getcc() { char ret; return vi_read(&ret, 1)==1 ? (unsigned char)ret : -1; }
void Encoder::writeFrom(GenBuffer::Writable& out, FILE *f) {
  char *buf=new char[BUFLEN];
  int wr;
  while (1) {
    if ((wr=fread(buf, 1, BUFLEN, f))<1) break;
    out.vi_write(buf, wr);
  }
  delete [] buf;
}

/* --- */

Filter::FILEE::FILEE(char const* filename) {
  if (NULLP==(f=fopen(filename,"wb"))) Error::sev(Error::ERROR) << "Filter::FILEE: error open4write: " << FNQ2(filename,strlen(filename)) << (Error*)0;
  closep=true;
}
void Filter::FILEE::vi_write(char const*buf, slen_t len) {
  if (len==0) close(); else fwrite(buf, 1, len, f);
}
void Filter::FILEE::close() {
  if (closep) { fclose(f); f=(FILE*)NULLP; closep=false; }
}

/* --- */

Filter::FILED::FILED(char const* filename) {
  if (NULLP==(f=fopen(filename,"rb"))) Error::sev(Error::ERROR) << "Filter::FILED: error open4read: " << FNQ2(filename,strlen(filename)) << (Error*)0;
  closep=true;
}
slen_t Filter::FILED::vi_read(char *buf, slen_t len) {
  if (len==0) { close(); return 0; }
  return fread(buf, 1, len, f);
}
void Filter::FILED::close() {
  if (closep) { fclose(f); f=(FILE*)NULLP; closep=false; }
}

/* --- */

Filter::PipeE::PipeE(GenBuffer::Writable &out_, char const*pipe_tmpl, slendiff_t i): tmpname(), out(out_), tmpename() {
  /* <code similarity: Filter::PipeE::PipeE and Filter::PipeD::PipeD> */
  param_assert(pipe_tmpl!=(char const*)NULLP);
  SimBuffer::B *pp;
  char const*s=pipe_tmpl;
  lex: while (s[0]!='\0') { /* Interate throuh the template, substitute temporary filenames */
    if (*s++=='%') switch (*s++) {
     case '\0': case '%':
      redir_cmd << '%';
      break;
     case 'i': /* the optional integer passed in param `i' */
      redir_cmd << i;
      break;
     case 'd': case 'D': /* temporary file for encoded data output */
      pp=&tmpname;
     put:
      // if (*pp) Error::sev(Error::ERROR) << "Filter::PipeE" << ": multiple %escape" << (Error*)0;
      /* ^^^ multiple %escape is now a supported feature */
      if (!*pp && !Files::find_tmpnam(*pp)) Error::sev(Error::ERROR) << "Filter::PipeE" << ": tmpnam() failed" << (Error*)0;
      assert(!!*pp); /* pacify VC6.0 */
      pp->term0();
      if ((unsigned char)(s[-1]-'A')<(unsigned char)('Z'-'A'))
        redir_cmd.appendFnq(*pp); /* Capital letter: quote from the shell */
        else redir_cmd << *pp;
      break;
     case 'e': case 'E': /* temporary file for error messages */
      pp=&tmpename;
      goto put;
     case 's': case 'S': /* temporary source file */
      pp=&tmpsname;
      goto put;
     default:
      Error::sev(Error::ERROR) << "Filter::PipeE" << ": invalid %escape in pipe_tmpl" << (Error*)0;
    } else redir_cmd << s[-1];
  }
  #if 0
    if (!tmpname) Error::sev(Error::ERROR) << "Filter::PipeE" << ": no outname (%D) in cmd: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
  #else
    /* Append quoted file redirect to command, if missing */
    if (!tmpname) { s=" >%D"; goto lex; }
  #endif
  #if !HAVE_PTS_POPEN
    if (!tmpsname) { s=" <%S"; goto lex; }
  #endif
  // tmpname="tmp.name";
  redir_cmd.term0();
  if (tmpname)  Files::tmpRemoveCleanup(tmpname ()); /* already term0() */
  if (tmpename) Files::tmpRemoveCleanup(tmpename()); /* already term0() */
  if (tmpsname) Files::tmpRemoveCleanup(tmpsname()); /* already term0() */
  /* </code similarity: Filter::PipeE::PipeE and Filter::PipeD::PipeD> */
  
  // fprintf(stderr, "rc: (%s)\n", redir_cmd());

 #if HAVE_PTS_POPEN
  if (!tmpsname) {
    if (NULLP==(p=popen(redir_cmd(), "w"CFG_PTS_POPEN_B))) Error::sev(Error::ERROR) << "Filter::PipeE" << ": popen() failed: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
    signal(SIGPIPE, SIG_IGN); /* Don't abort process with SIGPIPE signals if child cannot read our data */
  } else {
 #else
  if (1) {
 #endif
   #if !HAVE_system_in_stdlib
    Error::sev(Error::ERROR) << "Filter::PipeE" << ": no system() on this system" << (Error*)0;
   #else
    if (NULLP==(p=fopen(tmpsname(), "wb"))) Error::sev(Error::ERROR) << "Filter::PipeD" << ": fopen(w) failed: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
   #endif
  }
}
void Filter::PipeE::vi_copy(FILE *f) {
  writeFrom(out, f);
  if (ferror(f)) Error::sev(Error::ERROR) << "Filter::PipeE: vi_copy() failed" << (Error*)0;
  fclose(f);
}
void Filter::PipeE::vi_write(char const*buf, slen_t len) {
  assert(p!=NULLP);
  int wr;
  if (len==0) { /* EOF */
    if (tmpsname) {
     #if HAVE_system_in_stdlib
      fclose(p);
      if (0!=(system(redir_cmd()))) Error::sev(Error::ERROR) << "Filter::PipeE" << ": system() failed: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
      remove(tmpsname());
     #endif /* Dat: else is not required; would be unreachable code. */
    } else {
     #if HAVE_PTS_POPEN
      if (0!=pclose(p)) Error::sev(Error::ERROR) << "Filter::PipeE" << ": pclose() failed; error in external prg" << (Error*)0;
     #endif
    }
    vi_check();
    p=(FILE*)NULLP;
    FILE *f=fopen(tmpname(),"rb");
    if (NULLP==f) Error::sev(Error::ERROR) << "Filter::PipeE" <<": fopen() after pclose() failed: " << tmpname << (Error*)0;
    vi_copy(f);
    // if (ferror(f)) Error::sev(Error::ERROR) << "Filter::Pipe: fread() tmpfile failed" << (Error*)0;
    // fclose(f);
    /* ^^^ interacts badly when Image::load() is called inside vi_copy(),
     * Image::load() calls fclose()
     */
    if (tmpname ) remove(tmpname ());
    if (tmpename) remove(tmpename());
    if (tmpsname) remove(tmpsname());
    out.vi_write(0,0); /* Signal EOF to subsequent filters. */
  } else {
    while (len!=0) {
      wr=fwrite(buf, 1, len>0x4000?0x4000:len, p);
//      assert(!ferror(p));
      if (ferror(p)) {
        vi_check(); /* Give a chance to report a better error message when Broken File. */
        Error::sev(Error::ERROR) << "Filter::PipeE" << ": pipe write failed" << (Error*)0;
      }
      buf+=wr; len-=wr;
    }
  }
}
Filter::PipeE::~PipeE() {}
void Filter::PipeE::vi_check() {}

/* --- */

Filter::PipeD::PipeD(GenBuffer::Readable &in_, char const*pipe_tmpl, slendiff_t i): state(0), in(in_) {
  /* <code similarity: Filter::PipeE::PipeE and Filter::PipeD::PipeD> */
  param_assert(pipe_tmpl!=(char const*)NULLP);
  SimBuffer::B *pp=(SimBuffer::B*)NULLP;
  char const*s=pipe_tmpl;
  lex: while (s[0]!='\0') { /* Interate throuh the template, substitute temporary filenames */
    if (*s++=='%') switch (*s++) {
     case '\0': case '%':
      redir_cmd << '%';
      break;
     case 'i': /* the optional integer passed in param `i' */
      redir_cmd << i;
      break;
     case 'd': case 'D': /* temporary file for encoded data output */
      pp=&tmpname;
     put:
      // if (*pp) Error::sev(Error::ERROR) << "Filter::PipeD: multiple %escape" << (Error*)0;
      /* ^^^ multiple %escape is now a supported feature */
      if (!*pp && !Files::find_tmpnam(*pp)) Error::sev(Error::ERROR) << "Filter::PipeD" << ": tmpnam() failed" << (Error*)0;
      assert(*pp);
      pp->term0();
      if ((unsigned char)(s[-1]-'A')<(unsigned char)('Z'-'A'))
        redir_cmd.appendFnq(*pp); /* Capital letter: quote from the shell */
        else redir_cmd << *pp;
      break;
     case 'e': case 'E': /* temporary file for error messages */
      pp=&tmpename;
      goto put;
     case 's': case 'S': /* temporary source file */
      pp=&tmpsname;
      goto put;
     /* OK: implement temporary file for input, option to suppress popen() */
     default:
      Error::sev(Error::ERROR) << "Filter::PipeD: invalid %escape in pipe_tmpl" << (Error*)0;
    } else redir_cmd << s[-1];
  }
  #if 0
    if (!tmpname) Error::sev(Error::ERROR) << "Filter::PipeD" << ": no outname (%D) in cmd: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
  #else
    /* Append quoted file redirect to command, if missing */
    if (!tmpname) { s=" >%D"; goto lex; }
  #endif
  #if !HAVE_PTS_POPEN
    if (!tmpsname) { s=" <%S"; goto lex; }
  #endif
  // tmpname="tmp.name";
  redir_cmd.term0();
  if (tmpname)  Files::tmpRemoveCleanup(tmpname ()); /* already term0() */
  if (tmpename) Files::tmpRemoveCleanup(tmpename()); /* already term0() */
  if (tmpsname) Files::tmpRemoveCleanup(tmpsname()); /* already term0() */
  /* </code similarity: Filter::PipeE::PipeE and Filter::PipeD::PipeD> */
}
slen_t Filter::PipeD::vi_read(char *tobuf, slen_t tolen) {
  assert(!(tolen!=0 && state==2));
  if (state==2) return 0; /* Should really never happen. */
  /* Normal read operation with tolen>0; OR tolen==0 */
  if (state==0) { /* Read the whole stream from `in', write it to `tmpname' */
   #if HAVE_PTS_POPEN
    if (!tmpsname) {
      if (NULLP==(p=popen(redir_cmd(), "w"CFG_PTS_POPEN_B))) Error::sev(Error::ERROR) << "Filter::PipeD" << ": popen() failed: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
      signal(SIGPIPE, SIG_IGN); /* Don't abort process with SIGPIPE signals if child cannot read our data */
      vi_precopy();
      in.vi_read(0,0);
      if (0!=pclose(p)) Error::sev(Error::ERROR) << "Filter::PipeD" << ": pclose() failed; error in external prg" << (Error*)0;
    } else {
   #else
    if (1) {
   #endif
     #if !HAVE_system_in_stdlib
      Error::sev(Error::ERROR) << "Filter::PipeD" << ": no system() on this system" << (Error*)0;
     #else
      if (NULLP==(p=fopen(tmpsname(), "wb"))) Error::sev(Error::ERROR) << "Filter::PipeD" << ": fopen(w) failed: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
      vi_precopy();
      in.vi_read(0,0);
      fclose(p);
      if (0!=(system(redir_cmd()))) Error::sev(Error::ERROR) << "Filter::PipeD" << ": system() failed: " << (SimBuffer::B().appendDumpC(redir_cmd)) << (Error*)0;
      remove(tmpsname());
     #endif
    }
    vi_check();
    if (NULLP==(p=fopen(tmpname(),"rb"))) Error::sev(Error::ERROR) << "Filter::PipeD" << ": fopen() after pclose() failed: " << tmpname << (Error*)0;
    state=1;
  } /* IF state==0 */
  assert(state==1);
  if (tolen==0 || 0==(tolen=fread(tobuf, 1, tolen, p))) do_close();
  // putchar('{'); fwrite(tobuf, 1, tolen, stdout); putchar('}');
  return tolen;
}
void Filter::PipeD::do_close() {
  fclose(p); p=(FILE*)NULLP;
  if (tmpname ) remove(tmpname ());
  if (tmpename) remove(tmpename());
  if (tmpsname) remove(tmpsname());
  state=2;
}
void Filter::PipeD::vi_precopy() {
  char *buf0=new char[BUFLEN], *buf;
  slen_t len, wr;
  while (0!=(len=in.vi_read(buf0, BUFLEN))) {
    // printf("[%s]\n", buf0);
    for (buf=buf0; len!=0; buf+=wr, len-=wr) {
      wr=fwrite(buf, 1, len>0x4000?0x4000:len, p);
      if (ferror(p)) {
        vi_check(); /* Give a chance to report a better error message when Broken File. */
        Error::sev(Error::ERROR) << "Filter::PipeD" << ": pipe write failed" << (Error*)0;
      }
    }
  }
  delete [] buf0;
}
int Filter::PipeD::vi_getcc() {
  char ret; int i;
  // fprintf(stderr,"state=%u\n", state);
  switch (state) {
   case 0: return vi_read(&ret, 1)==1 ? (unsigned char)ret : -1;
   case 1: if (-1==(i=MACRO_GETC(p))) do_close(); return i;
   /* case: 2: fall-through */
  }
  return -1;
}
void Filter::PipeD::vi_check() {}
Filter::PipeD::~PipeD() { if (state!=2) vi_read(0,0); }

Filter::BufR::BufR(GenBuffer const& buf_): bufp(&buf_) {
  buf_.first_sub(sub);
}
int Filter::BufR::vi_getcc() {
  if (bufp==(GenBuffer const*)NULLP) return -1; /* cast: pacify VC6.0 */
  if (sub.len==0) {
    bufp->next_sub(sub);
    if (sub.len==0) { bufp=(GenBuffer const*)NULLP; return -1; }
  }
  sub.len--; return *sub.beg++;
}
slen_t Filter::BufR::vi_read(char *to_buf, slen_t max) {
  if (max==0 || bufp==(GenBuffer const*)NULLP) return 0;
  if (sub.len==0) {
    bufp->next_sub(sub);
    if (sub.len==0) { bufp=(GenBuffer const*)NULLP; return 0; }
  }
  if (max<sub.len) {
    memcpy(to_buf, sub.beg, max);
    sub.len-=max; sub.beg+=max;
    return max;
  }
  max=sub.len; sub.len=0;
  memcpy(to_buf, sub.beg, max);
  return max;
}
void Filter::BufR::vi_rewind() { bufp->first_sub(sub); }

Filter::FlatR::FlatR(char const* s_, slen_t slen_): s(s_), sbeg(s_), slen(slen_) {}
Filter::FlatR::FlatR(char const* s_): s(s_), sbeg(s_), slen(strlen(s_)) {}
void Filter::FlatR::vi_rewind() { s=sbeg; }
int Filter::FlatR::vi_getcc() {
  if (slen==0) return -1;
  slen--; return *(unsigned char const*)s++;
}
slen_t Filter::FlatR::vi_read(char *to_buf, slen_t max) {
  if (max>slen) max=slen;
  memcpy(to_buf, s, max);
  s+=max; slen-=max;
  return max;
}

/* --- */

/** @param fname must start with '/' (dir separator)
 * @return true if file successfully created
 */
FILE *Files::try_dir(SimBuffer::B &dir, SimBuffer::B const&fname, char const*s1, char const*s2) {
  if (dir.isEmpty() && s1==(char const*)NULLP) return (FILE*)NULLP;
  SimBuffer::B full(s1!=(char const*)NULLP?s1:dir(),
                    s1!=(char const*)NULLP?strlen(s1):dir.getLength(),
		    s2!=(char const*)NULLP?s2:"",
		    s2!=(char const*)NULLP?strlen(s2):0, fname(), fname.getLength());
  full.term0();
  struct stat st;
  FILE *f;
  /* Imp: avoid race conditions with other processes pretending to be us... */
#define lstat stat /*!! !!*/
  if (-1!=lstat(full(), &st)
   || (0==(f=fopen(full(), "wb")))
   || ferror(f)
     ) return (FILE*)NULLP;
  dir=full;
  return f;
}

/* @param dir `dir' is empty: appends a unique filename for a temporary
 *        file. Otherwise: returns a unique filename in the specified directory.
 *        Creates the new file with 0 size.
 * @return FILE* opened for writing for success, NULLP on failure
 * --return true on success, false on failure
 */
FILE *Files::open_tmpnam(SimBuffer::B &dir) {
  /* Imp: verify / on Win32... */
  /* Imp: ensure uniqueness on NFS */
  /* Imp: short file names */
  static unsigned PTS_INT32_T counter=0;
  assert(Error::tmpargv0!=(char const*)NULLP);
  SimBuffer::B fname("/tmp_", 5, Error::tmpargv0,strlen(Error::tmpargv0));
  fname << '_' << getpid() << '_' << counter++;
  fname.term0();
  FILE *f=(FILE*)NULLP;
  (void)( ((FILE*)NULLP!=(f=try_dir(dir, fname, 0, 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, PTS_CFG_P_TMPDIR, 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, getenv("TMPDIR"), 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, getenv("TMP"), 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, getenv("TEMP"), 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, "/tmp", 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, getenv("WINBOOTDIR"), "//temp"))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, getenv("WINDIR"), "//temp"))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, "c:/temp", 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, "c:/windows/temp", 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, "c:/winnt/temp", 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, "c:/tmp", 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, ".", 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, "..", 0))) ||
          ((FILE*)NULLP!=(f=try_dir(dir, fname, "../..", 0)) ));
  return f;
};

bool Files::find_tmpnam(SimBuffer::B &dir)  {
  FILE *f=open_tmpnam(dir);
  if (f!=NULL) { fclose(f); return true; }
  return false;
}

bool Files::tmpRemove=true;

static int cleanup_remove(Error::Cleanup *cleanup) {
  if (Files::tmpRemove) return Files::removeIf(cleanup->getBuf());
  Error::sev(Error::WARNING) << "keeping tmp file: " << cleanup->getBuf() << (Error*)0;
  return 0;
}

void Files::tmpRemoveCleanup(char const* filename) {
  Error::newCleanup(cleanup_remove, 0, filename);
}

static int cleanup_remove_cond(Error::Cleanup *cleanup) {
  if (*(FILE**)cleanup->data!=NULLP) {
    fclose(*(FILE**)cleanup->data);
    if (Files::tmpRemove) return Files::removeIf(cleanup->getBuf());
    Error::sev(Error::WARNING) << "keeping tmp2 file: " << cleanup->getBuf() << (Error*)0;
  }
  return 0;
}

void Files::tmpRemoveCleanup(char const* filename, FILE**p) {
  param_assert(p!=NULLP);
  Error::newCleanup(cleanup_remove_cond, (void*)p, filename);
}

int Files::removeIf(char const* filename) {
  if (0==remove(filename) || errno==ENOENT) return 0;
  return 1;
}

slen_t Files::statSize(char const* filename) {
  struct stat st;
  if (-1==lstat(filename, &st)) return (slen_t)-1;
  return st.st_size;
}

/* Tue Jul  2 10:57:21 CEST 2002 */
char const* Files::only_fext(char const*filename) {
  char const *ret;
  if (OS_COTY==COTY_WINNT || OS_COTY==COTY_WIN9X) {
    if ((USGE('z'-'a',filename[0]-'a') || USGE('Z'-'A',filename[0]-'A'))
     && filename[1]==':'
       ) filename+=2; /* strip drive letter */
    ret=filename;
    while (*filename!='\0') {
      if (*filename=='/' || *filename=='\\') ret=++filename;
                                        else filename++;
    }
  } else { /* Everything else is treated as UNIX */
    ret=filename;
    while (filename[0]!='\0') if (*filename++=='/') ret=filename;
  }
  return ret;
}

/* __END__ */
