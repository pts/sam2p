#define DUMMY \
set -ex; \
${CC:-gcc} -DNDEBUG=1 -g -ansi \
  -Wall -W -Wstrict-prototypes -Wtraditional -Wnested-externs -Winline \
  -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wmissing-prototypes \
  -Wmissing-declarations flateenc.c pts_defl.c -o flateenc; \
exit
/*
 * Usage: flateenc [-<level>] < <inputfile> > <outputfile>
 * <level> is one of: 0: no compression; 1: low & fast; 9: high & slow
 */

#include "pts_defl.h"
#include <unistd.h> /* read(), write() */
#include <stdio.h>
#include <stdlib.h> /* abort() */

int main(int argc, char **argv) {
  char ibuf[4096], obuf[6000]; /* Dat: 4096->6000 should be enough */
  char workspace[ZLIB_DEFLATE_WORKSPACESIZE_MIN]; /* Dat: as returned by zlib_deflate_workspacesize in ZLIB 1.1.3 */
  int got, zgot;
  /** Compression level: 0..9 or Z_DEFAULT_COMPRESSION */
  int level=Z_DEFAULT_COMPRESSION;
  z_stream zs;
  (void)argc;
  if (argv && argv[0] && argv[1] && argv[1][0]=='-' && argv[1][1]>='0' && argv[1][1]<='9')
    level=argv[1][1]-'0';
  /* printf("ws=%d\n", zlib_deflate_workspacesize()); */
  if (zlib_deflate_workspacesize()+(unsigned)0<sizeof(workspace)) abort();
  zs.total_in=0;
  zs.total_out=0;
  zs.workspace=workspace;
  zs.msg=(char*)0;
  zs.state=(struct zlib_internal_state*)0;
  zs.data_type=Z_UNKNOWN; /* Imp: do we have to initialize it? */
  if (Z_OK!=zlib_deflateInit(&zs, level)) abort();
  while (0<(got=read(0, ibuf, sizeof(ibuf)))) {
    zs.next_in=ibuf;   zs.avail_in=got;
    zs.next_out=obuf;  zs.avail_out=sizeof(obuf);
    if (Z_OK!=zlib_deflate(&zs, 0)) abort();
#ifdef DEBUG_PTS_DEFL
    fprintf(stderr, "ai=%d ao=%d no=%d\n", zs.avail_in, zs.avail_out, (char*)zs.next_out-obuf);
#endif
    if (0!=zs.avail_in) abort();
    got=sizeof(obuf)-zs.avail_out;
    if (got>0 && got!=write(1, zs.next_out-got, got)) abort();
  }
  if (0!=got) abort();
  do { /* flush all output */
    zs.next_in=NULL; zs.avail_in=0;
    zs.next_out=obuf; zs.avail_out=sizeof(obuf);
    if (Z_STREAM_END!=(zgot=zlib_deflate(&zs, Z_FINISH)) && Z_OK!=zgot) abort();
#ifdef DEBUG_PTS_DEFL
    fprintf(stderr, "ai=%d ao=%d flush\n", zs.avail_in, zs.avail_out);
#endif
    got=sizeof(obuf)-zs.avail_out;
    if (got>0 && got!=write(1, zs.next_out-got, got)) abort();
  } while (zgot==Z_OK);
  if (Z_OK!=zlib_deflateEnd(&zs)) abort();
  return 0;
}

