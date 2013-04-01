void sigFunc( short num, void *msg );
/*
 *      IMPLEMENTATION NOTES
 *
 *      Converting is achieved by interpolating the input samples in
 *      order to evaluate the represented continuous input slope at
 *      sample instances of the new rate (resampling). It is implemented
 *      as a polyphase FIR-filtering process (see reference). The rate
 *      conversion factor is determined by a rational factor. Its
 *      nominator and denominator are integers of almost arbitrary
 *      value, limited only by coefficient memory size.
 *
 *      General rate conversion formula:
 *
 *          out(n*Tout) = SUM in(m*Tin) * g((n*d/u-m)*Tin) * Tin
 *                    over all m
 *
 *      FIR-based rate conversion formula for polyphase processing:
 *
 *                        L-1
 *          out(n*Tout) = SUM in(A(i,n)*Tin) * g(B(i,n)*Tin) * Tin
 *                        i=0
 *
 *          A(i,n) = i - (L-1)/2 + [n*d/u]
 *                 = i - (L-1)/2 + [(n%u)*d/u] + [n/u]*d
 *          B(i,n) = n*d/u - [n*d/u] + (L-1)/2 - i
 *                 =  ((n%u)*d/u)%1  + (L-1)/2 - i
 *          Tout   = Tin * d/u
 *
 *        where:
 *          n,i         running integers
 *          out(t)      output signal sampled at t=n*Tout
 *          in(t)       input signal sampled in intervalls Tin
 *          u,d         up- and downsampling factor, integers
 *          g(t)        interpolating function
 *          L           FIR-length of realized g(t), integer
 *          /           float-division-operator
 *          %           float-modulo-operator
 *          []          integer-operator
 *
 *        note:
 *          (L-1)/2     in A(i,n) can be omitted as pure time shift yielding
 *                      a causal design with a delay of ((L-1)/2)*Tin.
 *          n%u         is a cyclic modulo-u counter clocked by out-rate
 *          [n/u]*d     is a d-increment counter, advanced when n%u resets
 *          B(i,n)*Tin  can take on L*u differnt values, at which g(t)
 *                      has to be sampled as a coefficient array
 *
 *      Interpolation function design:
 *
 *          The interpolation function design is based on a sinc-function
 *          windowed by a gaussian function. The former determines the
 *          cutoff frequency, the latter limits the necessary FIR-length by
 *          pushing the outer skirts of the resulting impulse response below
 *          a certain threshold fast enough. The drawback is a smoothed
 *          cutoff inducing some aliasing. Due to the symmetry of g(t) the
 *          group delay of the filtering process is contant (linear phase).
 *
 *          g(t) = 2*fgK*sinc(pi*2*fgK*t) * exp(-pi*(2*fgG*t)**2)
 *
 *        where:
 *          fgK         cutoff frequency of sinc function in f-domain
 *          fgG         key frequency of gaussian window in f-domain
 *                      reflecting the 6.82dB-down point
 *
 *        note:
 *          Taking fsin=1/Tin as the input sampling frequncy, it turns out
 *          that in conjunction with L, u and d only the ratios fgK/(fsin/2)
 *          and fgG/(fsin/2) specify the whole proces. Requiring fsin, fgK
 *          and fgG as input purposely keeps the notion of absolute
 *          frequencies.
 *
 *      Numerical design:
 *
 *          Samples are expected to be 16bit-signed integers, alternating
 *          left and right channel in case of stereo mode- The byte order
 *          per sample is selectable. FIR-filtering is implemented using
 *          32bit-signed integer arithmetic. Coefficients are scaled to
 *          find the output sample in the high word of accumulated FIR-sum.
 *
 *          Interpolation can lead to sample magnitudes exceeding the
 *          input maximum. Worst case is a full scale step function on the
 *          input. In this case the sinc-function exhibits an overshoot of
 *          2*9=18percent (Gibb's phaenomenon). In any case sample overflow
 *          can be avoided by a gain of 0.8.
 *
 *          If u=d=1 and if the input stream contains only a single sample,
 *          the whole length of the FIR-filter will be written to the output.
 *          In general the resulting output signal will be (L-1)*fsout/fsin
 *          samples longer than the input signal. The effect is that a
 *          finite input sequence is viewed as padded with zeros before the
 *          `beginning' and after the `end'.
 *
 *          The output lags ((L-1)/2)*Tin behind to implement g(t) as a
 *          causal system corresponding to a causal relationship of the
 *          discrete-time sequences in(m/fsin) and out(n/fsout) with
 *          resepect to a sequence time origin at t=n*Tin=m*Tout=0.
 *
 *
 *      REFERENCES
 *
 *          Crochiere, R. E., Rabiner, L. R.: "Multirate Digital Signal
 *          Processing", Prentice-Hall, Englewood Cliffs, New Jersey, 1983
 *
 *          Zwicker, E., Fastl, H.: "Psychoacoustics - Facts and Models",
 *          Springer-Verlag, Berlin, Heidelberg, New-York, Tokyo, 1990
 */

#ifdef WIN32
#include <windows.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#ifndef UNIX
#include <io.h>
#include <process.h>
#endif

#include "../../../units/common.src/bastypes.h"

#include "../../../units/common.src/util.h"
#include "../../../units/common.src/cfg_file.h"
#include "../../../units/common.src/num2nam.h"
#include "../../common/source/global.h"
#include "../../common/source/texte.h"
#include "../../../units/common.src/os.h"

/*
 *      adaptable defines and globals
 */

#define M_PI 3.14159265358979323846

#ifndef MAXUP
#define MAXUP           0x400           /* MAXUP*MAXLENGTH worst case malloc */
#endif

#ifndef MAXLENGTH
#define MAXLENGTH       0x400           /* max FIR length */
#endif

#define OUTBUFFSIZE     MAXLENGTH       /* fit >=MAXLENGHT samples */
#define INBUFFSIZE      (2*MAXLENGTH)   /* fit >=2*MAXLENGTH samples */
#define sqr(a)  ((a)*(a))

/*
 *      other globals
 */

FILE *g_outfilehandle = NULL;            /*  */

double  g_ampli = 0.8;                  /* default gain, don't change */
int
        g_firlen  = 0,                  /* FIR-length */
        g_up = 0,                       /* upsampling factor */
        g_down = 0                      /* downsampling factor */
;

//LONG
long
        g_sin[INBUFFSIZE],              /* input buffer */
        g_sout[OUTBUFFSIZE],            /* output buffer */
        *g_coep                         /* coefficient array pointer */
;
double
        g_fsi =0.0,                          /* input sampling frequency */
        g_fgk =0.0,                          /* sinc-filter cutoff frequency */
        g_fgg =0.0                          /* gaussian window key frequency */
;                                       /* (6.8dB down freq. in f-domain) */

unsigned char /*UCHAR*/ reverse_byte[256]={
0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};


char str[100];
unsigned char  bytevar;
unsigned short wordvar;
unsigned long  dwordvar;

unsigned short Channels;
unsigned long  SamplesPerSecond;
unsigned short BitsPerSample;

unsigned long  BytePerSecond;
unsigned short BytePerSample;

/*
 * linear2alaw() - Convert a 16-bit linear alw value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *              Linear Input Code       Compressed Code
 *      ------------------------        ---------------
 *      0000000wxyza                    000wxyz
 *      0000001wxyza                    001wxyz
 *      000001wxyzab                    010wxyz
 *      00001wxyzabc                    011wxyz
 *      0001wxyzabcd                    100wxyz
 *      001wxyzabcde                    101wxyz
 *      01wxyzabcdef                    110wxyz
 *      1wxyzabcdefg                    111wxyz
 *
 */
unsigned char linear2alaw(long alw_val)  /* 2's complement (16-bit range) */
{
  long          mask = 0;
  long          seg = 0;
  unsigned char aval = 0;
  static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
                            0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

  if (alw_val >= 0) {
    mask = 0xD5;                /* sign (7th) bit = 1 */
  } else {
    mask = 0x55;                /* sign bit = 0 */
    alw_val = -alw_val;
  }

  if(alw_val<0x7ff8)
    alw_val+=8;

  if(alw_val<0x10)
    mask = 0xD5;

        /* Convert the scaled magnitude to segment number. */

  for (seg = 0; seg < 8; seg++) {
    if (alw_val <= seg_end[seg])
      break;
    }

        /* Combine the sign, segment, and quantization bits. */

  if (seg >= 8)         /* out of range, return maximum value. */
    return ((unsigned char)(0x7F ^ mask));
  else {
    aval = (unsigned char)(seg << 4);
    if (seg < 2)
      aval |= (alw_val >> 4) & 0xf;
    else
      aval |= (alw_val >> (seg + 3)) & 0xf;
    return ((unsigned char)(aval ^ mask));
  }
}

/*
 *      evaluate sinc(x) = sin(x)/x safely
 */
double sinc(double x) {
  return(fabs(x) < 1E-50 ? 1. : sin(fmod(x,2*M_PI))/x);
}

/*
 *      evaluate interpolation function g(t) at t
 *      integral of g(t) over all times is expected to be one
 */
double interpol_func(double t, double fgk, double fgg) {
  return (2*fgk*sinc(M_PI*2*fgk*t)*exp(-M_PI*sqr(2*fgg*t)));
}

/*
 *      evaluate coefficient from i, q=n%u by sampling interpolation function
 *      and scale it for integer multiplication used by FIR-filtering
 */
long coefficient(long i, long q, long firlen, double fgk, double fgg, double fsi,
                 long up, long down, double amp) {
  return(
    (int)(0x10000 * amp *
       interpol_func(
         (fmod(q*down/(double)up,1.) + (firlen-1)/2 - i) / fsi,
         fgk,
         fgg
       ) / fsi
    )
  );
}

/*
 *      transfer n LONGs from  s to d
 */
void transfer_int(long *s, long *d, long n) {
  long *e;

  if (n < 1)
    return;
  e = d + n;
  while (d != e)
    *d++ = *s++;
}

/*
 *      zerofill n longs from s
 */
void zerofill(long *s, long n) {
  long *e;

  if (n < 1)
    return;
  e = s + n;
  while (s != e)
    *s++ = 0;
}

/*
 *      convert buffer of n samples to longs
 */
void bsample_to_int(void *buff, long n) {
  unsigned char *s;
  long *d;

  if (n < 1)
    return;
  s = (unsigned char*)buff + n;
  d = (long*)buff + n;
  while (s != buff) {
    *--d = ((long)(*--s)-128L)<<8;
  }
}

/*
 *      convert buffer of n samples to longs
 */
void bsample2_to_int(void *buff, long n) {
  unsigned char *s;
//unsigned char *ms;
//unsigned char *mms;

  long *d;

  if (n < 1) return;

  s = (unsigned char*)buff + n*2;
  d = (long*)buff + n;
  while (s != buff) {
//problem
//ms = s-1;
//mms=s-2;
    *--d = (((long)(*--s)-128L)<<7)+(((long)(*--s)-128L)<<7);
//    *--d = (((long)(*ms)-128L)<<7)+(((long)(*mms)-128L)<<7);
//s -=2;
  }
}

/*
 *      convert buffer of n samples to longs
 */
void sample_to_int(void *buff, long n) {
  short *s;
  long *d;

  if (n < 1)
    return;
  s = (short*)buff + n;
  d = (long*)buff + n;
  while (s != buff) {
    *--d = (long)(*--s);
  }
}

/*
 *      convert buffer of n samples to longs
 */
void sample2_to_int(void *buff, long n) {
  short *s;
//  short *ms;
//  short *mms;
  long *d;

  if (n < 1)
    return;
  s = (short*)buff + n*2;
  d = (long*)buff + n;
  while (s != buff) {
//problem
//ms = s-1;
//mms = s-2;
    *--d = ((long)(*--s)+(long)(*--s))>>1;
//    *--d = ((long)(*ms)+(long)(*mms))>>1;
//s -=2;
  }
}

/*
 *      convert buffer of n longs to samples
 */
void int_to_sample(short *buff, long n) {
  short *s, *e;
  unsigned char *d;

  if (n < 1)
    return;
  s = buff;
  d = (unsigned char*)buff;
  e = buff + n*2;
  while (s != e) {
    s++;
    *d++ = reverse_byte[linear2alaw(*s++)];
  }
}

/*
 *      convert buffer of n longs to samples
 */
void intlo_to_sample(short *buff, long n) {
  short *s, *e;
  unsigned char *d;

  if (n < 1)
    return;
  s = buff;
  d = (unsigned char*)buff;
  e = buff + n*2;
  while (s != e) {
    *d++ = reverse_byte[linear2alaw(*s++)];
    s++;
  }
}

/*
 *      FIR-routines,
 *      this is where we need all the MIPS
 */
void fir(register long *inp, register long *coep, long firlen, long *outp) {
  register long akku = 0, *endp;
  long n1 = (firlen / 8) * 8, n0 = firlen % 8;

  endp = coep + n1;
  while (coep != endp) {
    akku += *inp++ * *coep++;
    akku += *inp++ * *coep++;
    akku += *inp++ * *coep++;
    akku += *inp++ * *coep++;
    akku += *inp++ * *coep++;
    akku += *inp++ * *coep++;
    akku += *inp++ * *coep++;
    akku += *inp++ * *coep++;
  }

  endp = coep + n0;
  while (coep != endp) {
    akku += *inp++ * *coep++;
  }
  *outp = akku;
}

/*
 *      filtering from input buffer to output buffer;
 *      returns number of processed samples in output buffer:
 *      if it is not equal to output buffer size,
 *      the input buffer is expected to be refilled upon entry, so that
 *      the last firlen numbers of the old input buffer are
 *      the first firlen numbers of the new input buffer;
 *      if it is equal to output buffer size, the output buffer
 *      is full and is expected to be stowed away;
 *
 */
long filtering_on_buffers(long *inp, long insize, long *outp, long outsize,
                         long *coep, long firlen, long up, long down) {
  static long inbaseidx = 0, inoffset = 0, cycctr = 0, outidx = 0;

  for (;;) {
    inoffset = (cycctr * down)/up;
    if ((inbaseidx + inoffset + firlen) > insize) {
      inbaseidx -= insize - firlen + 1;
      return(outidx);
    }
    fir(inp + inoffset + inbaseidx,
       coep + cycctr * firlen,
       firlen, outp + outidx++);
    cycctr++;
    if (!(cycctr %= up))
      inbaseidx += down;
    if (!(outidx %= outsize))
      return(outsize);
  }
}

/*
 *      read and convert input sample format to integer
 */
long intread(FILE* hmmioSrc, void *buff, long n, long Bits, long Channels) {
  long rd = 0;

 if(Channels==2) {
  if(Bits==8) {
    if ((rd = fread (buff, 1, n*2, hmmioSrc)) <= 0)
      return(rd);
    bsample2_to_int(buff,n);
    return(rd/2);
  } else {
    if ((rd = fread (buff, 1, n*sizeof(short)*2, hmmioSrc)) <= 0)
      return(rd);
    sample2_to_int(buff,n);
    return(rd/sizeof(short)/2);
  }
 } else {
  if(Bits==8) {
    if ((rd = fread(buff, 1, n, hmmioSrc)) <= 0)
      return(rd);
    bsample_to_int(buff,n);
    return(rd);
  } else {
    if ((rd = fread (buff, 1, n*sizeof(short), hmmioSrc )) <= 0)
      return(rd);
    sample_to_int(buff,n);
    return(rd/sizeof(short));
  }
 }
}

/*
 *      do some conversion jobs and write
 */
long intwrite(long *buff, long n) {
  long written = 0;

  int_to_sample((short*)buff, n);
  if ((written = fwrite(buff, 1, n, g_outfilehandle)) <= 0)
    return(written);
  return(written);
}

/*
 *      do some conversion jobs and write
 */
long intlowrite(long *buff, long n) {
  long written = 0;

  intlo_to_sample((short*)buff, n);
  if ((written = fwrite(buff, 1, n, g_outfilehandle)) <= 0)
    return(written);
  return(written);
}

/*
 *      set up coefficient array
 */
void make_coe(void) {
  long i = 0, q =0;

  for (i = 0; i < g_firlen; i++) {
    for (q = 0; q < g_up; q++) {
      g_coep[q * g_firlen + i] = coefficient(i, q, g_firlen,
        g_fgk, g_fgg, g_fsi, g_up, g_down, g_ampli);
    }
  }
}


void comp_para(void) {
  long u = 0,d=0;
  double fin1=0.0,fdiff=0.0;

  fdiff=HUGE_VAL;

  for(u=1;u<=1024;u++) {
    d=(long)((g_fsi*u)/8000.);
    fin1=(8000.*d)/u;
    if(fdiff>fabs(g_fsi-fin1)) {
      fdiff=fabs(g_fsi-fin1);
      g_up=u;
      g_down=d;
    }
    d++;
    fin1=(8000.*d)/u;
    if(fdiff>fabs(g_fsi-fin1)) {
      fdiff=fabs(g_fsi-fin1);
      g_up=u;
      g_down=d;
    }
    if(fdiff==0.)
      break;
  }

  if (g_up >= g_down) { /* upsampling */
    g_fgg = g_fsi * 0.0116;
    g_fgk = g_fsi * 0.461;
    g_firlen = 162;
  } else {      /* downsampling */
    g_fgg = (g_up * g_fsi * 0.0116)/g_down;
    g_fgk = (g_up * g_fsi * 0.461)/g_down;
    g_firlen = (162 * g_down)/g_up;
  }
}

/*
 *      main
 */

short wav2alw(char *InName)
{
  long insize = 0, outsize = 0, skirtlen=0;
//  unsigned long         ulAudioHeaderLength=0;
  FILE*           hmmioSource;
//  unsigned long         ulBytesRead=0;
//  long          rc=0;
  char          OutName[300];
  char          *help;
  char          helpstr[300];

  memset (g_sin ,0,sizeof(long)*INBUFFSIZE);
  memset (g_sout,0,sizeof(long)*OUTBUFFSIZE);

  if (InName[0] == '*') return (-200);
  if (!util_file_exist(InName)) {
     sprintf (helpstr,STR_NOT_FOUND,InName);
     sigFunc (1,helpstr);
     return (-100);
  }

  if (!strstr(InName,WAV_EXT)) return (-101);

  strcpy (OutName,InName);

  help = strstr(OutName,WAV_EXT);
  *(++help) = 'a';
  *(++help) = 'l';
  *(++help) = 'w';

  strcpy (OutName,util_strip_path(OutName));

  if (util_file_exist(OutName)) {
     if (util_file_age(InName) <= util_file_age(OutName)) return (-102);
  }

  /*******************************/
  /* Set up/open the SOURCE file */
  /*******************************/

  hmmioSource = fopen(InName, "rb");
  if (hmmioSource == NULL) {
    return (2);
  }

  /****************************/
  /* Obtain the SOURCE HEADER */
  /****************************/

  g_outfilehandle=fopen(OutName,"wb");
  if (g_outfilehandle==NULL) {
    fclose (hmmioSource);
    return(-1);
  };

  fread (str,1,4,hmmioSource);
  str[4] = 0;
  if (strcmp("RIFF",str)) {
    sprintf (str,"Not a WAVE-file: %s!",InName);
    sigFunc (2,str);
    return (-1);
  }
//  printf ("\n\n#%s#\n",str);

  fread (&dwordvar,1,4,hmmioSource);
  if (!dwordvar) {
    sprintf (str,"WAVE-file is empty: %s!",InName);
    sigFunc (2,str);
    return (-1);
  }
//  printf ("dateil„nge=%d\n",dwordvar);

  fread (str,1,4,hmmioSource);
  str[4] = 0;
  if (strcmp("WAVE",str)) {
    sprintf (str,"Not a WAVE-file: %s!",InName);
    sigFunc (2,str);
    return (-1);
  }
//  printf ("#%s#\n",str);

  fread (str,1,4,hmmioSource);
  str[4] = 0;
  if (strcmp("fmt ",str)) {
    sprintf (str,"Not a WAVE-file: %s!",InName);
    sigFunc (2,str);
    return (-1);
  }
//  printf ("#%s#\n",str);

  fread (&dwordvar ,1,4,hmmioSource);
//  printf ("l„nge subchunk=%d\n",dwordvar);
//  if (dwordvar != 16) {
//    sprintf (str,"Unsupported format: %s!",InName);
//  sigFunc (2,str);
//  return (-1);
//}

  fread (&wordvar ,1,2,hmmioSource);
//  printf ("format tag=%d\n",wordvar);

  fread (&Channels ,1,2,hmmioSource);
//  printf ("channels=%d\n",Channels);
  if (!((Channels == 1) || (Channels == 2))) {
    sprintf (str,"Error num channels: %s!",InName);
    sigFunc (2,str);
    return (-1);
  }

  fread (&SamplesPerSecond ,1,4,hmmioSource);
//  printf ("Samples per second=%d\n",SamplesPerSecond);

  fread (str ,1,6,hmmioSource);

  fread (&BitsPerSample ,1,2,hmmioSource);
//  printf ("bit per sample=%d\n",BitsPerSample);


  if(SamplesPerSecond < 7984 || SamplesPerSecond > 8016) {

    g_fsi=SamplesPerSecond;

    comp_para();

    if ((g_coep = (long*)malloc(g_firlen * g_up * sizeof(int))) == NULL) {
      return (-2);
    }
    make_coe();
    skirtlen = g_firlen - 1;
    zerofill(g_sin, skirtlen);
    do {
      insize = intread(hmmioSource, g_sin + skirtlen,
        INBUFFSIZE - skirtlen,
        BitsPerSample,
        Channels);
      if (insize < 0 || insize > INBUFFSIZE - skirtlen) {
        fclose(g_outfilehandle);
        free(g_coep);
        return (-3);
      }
      for (;;) {
        outsize = filtering_on_buffers(g_sin, skirtlen + insize,
            g_sout, OUTBUFFSIZE, g_coep, g_firlen, g_up, g_down);
        if (outsize != OUTBUFFSIZE) {
          transfer_int(g_sin + insize, g_sin, skirtlen);
          break;
        }
        if (intwrite(g_sout, outsize) != outsize) {
          free(g_coep);
          return (-4);
        }
      };
    } while (insize > 0);
    zerofill(g_sin + skirtlen, skirtlen);
    do {
      outsize = filtering_on_buffers(g_sin, skirtlen + skirtlen,
          g_sout, OUTBUFFSIZE, g_coep, g_firlen, g_up, g_down);
      if (intwrite(g_sout, outsize) != outsize) {
        free(g_coep);
        return (-5);
      }
    } while (outsize == OUTBUFFSIZE);

    free(g_coep);

  } else { /* Input is 8000Sample/s no rate conversion */
    do {
      insize = intread(hmmioSource, g_sin,
        INBUFFSIZE,
        BitsPerSample,
        Channels);
      if (insize < 0 || insize > INBUFFSIZE) {
        fclose(g_outfilehandle);
        return (-6);
      }
      if (intlowrite(g_sin, insize) != insize) {
        return (-7);
      }
    } while (insize > 0);
  }

  if (fclose(g_outfilehandle)) {
    return (-8);
  }

  fclose (hmmioSource);

  return 0;
}

/*******************************************************************/

short runflag = 0;


void convert_all_lookup (char *filename)
{
  FILE *namedat;
  char str [300];
  char help[300];
  char *strptr;

  namedat = fopen (filename,"r");
  if (namedat) {
     while (fgets(str,200,namedat)) {
        if((str[0] != '#') && (strchr(str,'|'))) {
           strptr = &str[0];
           strptr = strchr(strptr,'|');
           strptr++;
           strptr = strchr(strptr,'|');
           strptr++;
           *strchr(strptr,'|') = 0;

           strcpy (help,check_time(strptr));
           wav2alw (help);
        }
     }
     fclose (namedat);
  }
}

void wav2alw_all (void)
{
  char help[200];

  convert_all_lookup (NAMFILE);
  convert_all_lookup (PRTFILE);

  config_file_read_string(STD_CFG_FILE,WELCOME_WAVE ,help,WELCOME_WAVE_DEF);
  strcpy (help,check_time(help));
  wav2alw (help);

  config_file_read_string(STD_CFG_FILE,RINGRING_WAVE,help,RINGRING_WAVE_DEF);
  strcpy (help,check_time(help));
  wav2alw (help);
}

void _Optlink convert_all (void *arglist)
{
   while (runflag) OsSleep (250);
   runflag++; sigFunc( 7, &runflag);
      wav2alw_all();
   runflag--; sigFunc( 7, &runflag);
};

char fname[200];

void _Optlink convert (void *arglist)
{
   while (runflag) OsSleep (250);
   runflag++; sigFunc( 7, &runflag);
   wav2alw (&fname[0]);
   runflag--; sigFunc( 7, &runflag);
};

void wav2alw_convert_all  (void)
{
  OsStartThread(convert_all);
};

void wav2alw_convert      (char * filename)
{
  strcpy (fname,filename);
  OsStartThread (convert);
};

short  wav2alw_convert_runs (void)
{
  return (runflag);
};


