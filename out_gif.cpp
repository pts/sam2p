/* out_gif.cpp
 * by pts@fazekas.hu at Sat Mar 23 11:02:36 CET 2002
 */

#include "image.hpp"

#if USE_OUT_GIF
#include <string.h>
#include <stdio.h>

/**** pts ****/
#ifdef __cplusplus
#  define AALLOC(var,len,itemtype) var=new itemtype[len]
#  define AFREE(expr) delete [] (expr)
#else
#  include <stdlib.h> /* malloc(), free() */
#  define AALLOC(var,len,itemtype) var=(itemtype*)malloc(len*sizeof(itemtype));
#  define AFREE(expr) free((expr))
#endif
#define HasLZW 1
#define True true
#define False false

/* The code of GIFEncodeImage is based on an early version of ImageMagick. */
static unsigned int GIFEncodeImage(GenBuffer::Writable& out, char const*ppbeg, register char const*ppend, const unsigned int data_size)
{
#define MaxCode(number_bits)  ((1 << (number_bits))-1)
#define MaxHashTable  5003
#define MaxGIFBits  12
#if defined(HasLZW)
#define MaxGIFTable  (1 << MaxGIFBits)
#else
#define MaxGIFTable  max_code
#endif
#define GIFOutputCode(code) \
{ \
  /*  \
    Emit a code. \
  */ \
  if (bits > 0) \
    datum|=((long) code << bits); \
  else \
    datum=(long) code; \
  bits+=number_bits; \
  while (bits >= 8) \
  { \
    /*  \
      Add a character to current packet. \
    */ \
    packet[byte_count++]=(unsigned char) (datum & 0xff); \
    if (byte_count >= 254) \
      { \
        packet[-1]=byte_count; \
        out.vi_write((char*)packet-1, byte_count+1); \
        byte_count=0; \
      } \
    datum>>=8; \
    bits-=8; \
  } \
  if (free_code > max_code)  \
    { \
      number_bits++; \
      if (number_bits == MaxGIFBits) \
        max_code=MaxGIFTable; \
      else \
        max_code=MaxCode(number_bits); \
    } \
}

  int
    bits,
    byte_count,
    i,
    next_pixel,
    number_bits;

  long
    datum;

  register int
    displacement,
    k;
    
  register char const*pp;

  short
    clear_code,
    end_of_information_code,
    free_code,
    *hash_code,
    *hash_prefix,
    index,
    max_code,
    waiting_code;

  unsigned char
    *packet,
    *hash_suffix;

  /*
    Allocate encoder tables.
  */
  AALLOC(packet,257,unsigned char);
  AALLOC(hash_code,MaxHashTable,short);
  AALLOC(hash_prefix,MaxHashTable,short);
  AALLOC(hash_suffix,MaxHashTable,unsigned char);
  if ((packet == (unsigned char *) NULL) || (hash_code == (short *) NULL) ||
      (hash_prefix == (short *) NULL) ||
      (hash_suffix == (unsigned char *) NULL))
    return(False);
  packet++;
  /* Now: packet-1 == place for byte_count */
  /*
    Initialize GIF encoder.
  */
  number_bits=data_size;
  max_code=MaxCode(number_bits);
  clear_code=((short) 1 << (data_size-1));
  end_of_information_code=clear_code+1;
  free_code=clear_code+2;
  byte_count=0;
  datum=0;
  bits=0;
  for (i=0; i < MaxHashTable; i++)
    hash_code[i]=0;
  GIFOutputCode(clear_code);
  /*
    Encode pixels.
  */
  /**** pts ****/
  pp=ppbeg;
  waiting_code=*pp++;

  while (pp!=ppend) {
      /*
        Probe hash table.
      */
      index=*(unsigned char const*)pp++;
      k=(int) ((int) index << (MaxGIFBits-8))+waiting_code;
      if (k >= MaxHashTable)
        k-=MaxHashTable;
#if defined(HasLZW)
      if (hash_code[k] > 0)
        {
          if ((hash_prefix[k] == waiting_code) && (hash_suffix[k] == index))
            {
              waiting_code=hash_code[k];
              continue;
            }
          if (k == 0)
            displacement=1;
          else
            displacement=MaxHashTable-k;
          next_pixel=False;
          for ( ; ; )
          {
            k-=displacement;
            if (k < 0)
              k+=MaxHashTable;
            if (hash_code[k] == 0)
              break;
            if ((hash_prefix[k] == waiting_code) && (hash_suffix[k] == index))
              {
                waiting_code=hash_code[k];
                next_pixel=True;
                break;
              }
          }
          if (next_pixel != False) /* pacify VC6.0 */
            continue;
        }
#endif
      GIFOutputCode(waiting_code);
      if (free_code < MaxGIFTable)
        {
          hash_code[k]=free_code++;
          hash_prefix[k]=waiting_code;
          hash_suffix[k]=index;
        }
      else
        {
          /*
            Fill the hash table with empty entries.
          */
          for (k=0; k < MaxHashTable; k++)
            hash_code[k]=0;
          /*
            Reset compressor and issue a clear code.
          */
          free_code=clear_code+2;
          GIFOutputCode(clear_code);
          number_bits=data_size;
          max_code=MaxCode(number_bits);
        }
      waiting_code=index;
#if 0 /**** pts ****/
      if (QuantumTick(i,image) && (image->previous == (Image2 *) NULL))
        ProgressMonitor(SaveImageText,i,image->packets);
#endif
  }
  /*
    Flush out the buffered code.
  */
  GIFOutputCode(waiting_code);
  GIFOutputCode(end_of_information_code);
  if (bits > 0)
    {
      /*
        Add a character to current packet.
      */
      packet[byte_count++]=(unsigned char) (datum & 0xff);
      if (byte_count >= 254)
        {
          packet[-1]=byte_count;
          out.vi_write((char*)packet-1, byte_count+1);
          byte_count=0;
        }
    }
  /*
    Flush accumulated data.
  */
  if (byte_count > 0)
    {
      packet[-1]=byte_count;
      out.vi_write((char*)packet-1, byte_count+1);
    }
  /*
    Free encoder memory.
  */
  AFREE(hash_suffix);
  AFREE(hash_prefix);
  AFREE(hash_code);
  AFREE(packet-1);
  return pp==ppend;
}

/** This isn't a complete GIF writer. For example, it doesn't support
 * animation or multiple sub-images. But it supports transparency and
 * compression. Only works when . The user should call
 * packPal() first to ensure img->getBpc()==8, and to get a minimal palette.
 */
void out_gif_write(GenBuffer::Writable& out, Image::Indexed *img) {
  /* Tested and proven to work at Sat Mar 23 13:11:41 CET 2002 */
  unsigned i, c, bits_per_pixel;
  signed transp;
  char hd[19];
  
  assert(img->getBpc()==8); /* 1 palette entry == 8 bits */
  
  memcpy(hd, "GIF89a", 6);
  i=img->getWd(); hd[6]=i; hd[7]=i>>8;
  i=img->getHt(); hd[8]=i; hd[9]=i>>8;
  
  transp=img->getTransp();
  // transp=-1; /* With this, transparency will be ignored */
  c=img->getNcols();
  // fprintf(stderr, "GIF89 write transp=%d ncols=%d\n", transp, c);
  bits_per_pixel=1; while ((c>>bits_per_pixel)!=0) bits_per_pixel++;
  assert(bits_per_pixel<=8);
  c=3*((1<<bits_per_pixel)-c);
  /* Now: c is the number of padding bytes */
  
  hd[10]= 0x80 /* have global colormap */
        | ((8-1) << 4) /* color resolution: bpc==8 */
        | (bits_per_pixel-1); /* size of global colormap */
  hd[11]=0; /* background color: currently unused */
  hd[12]=0; /* reversed */
  out.vi_write(hd, 13);

  // out.vi_write("\xFF\x00\x00" "\x00\xFF\x00" "\x00\x00\xFF", 9); // !!
  
  out.vi_write(img->getHeadp(), img->getRowbeg()-img->getHeadp()); /* write colormap */
  if (c!=0) {
    char *padding=new char[c];
    memset(padding, '\0', c); /* Not automatic! */
    out.vi_write(padding, c);
    delete [] padding;
  }

  /* Write Graphics Control extension. Only GIF89a */
  hd[0]=0x21; hd[1]=(char)0xf9; hd[2]=0x04;
  hd[3]=transp!=-1; /* dispose==0 */
  hd[4]=hd[5]=0; /* delay==0 */
  hd[6]=transp; /* transparent color index -- or 255 */
  hd[7]=0;
  
  /* Write image header */
  hd[8]=',';
  hd[ 9]=hd[10]=0;   /* left */
  hd[11]=hd[12]=0; /* top  */
  i=img->getWd(); hd[13]=i; hd[14]=i>>8;
  i=img->getHt(); hd[15]=i; hd[16]=i>>8;
  hd[17]=0; /* no interlace, no local colormap, no bits in local colormap */
  
  if ((c=bits_per_pixel)<2) c=4;
  hd[18]=c; /* compression bits_per_pixel */
  out.vi_write(hd, 19);

  i=GIFEncodeImage(out, img->getRowbeg(), img->getRowbeg()+img->getRlen()*img->getHt(), c+1);
  assert(i!=0);
  
  /* Write trailer */
  hd[0]=0; hd[1]=';';
  out.vi_write(hd, 2);
}
#else
#include <stdlib.h>
void out_gif_write(GenBuffer::Writable&, Image::Indexed *) {
  assert(0);
  abort();
}
#endif
