/*
programmiert nach dem goertzel algorithmus.
http://ptolemy.eecs.berkeley.edu/~pino/Ptolemy/papers/96/dtmf_ict/
*/

#include "../../../units/common.src/cfg_file.h"
#include "../../common/source/global.h"

#define NUM_DTMF_POINTS    205
#define NUM_FREQ           16
#define AMP_BITS           9
//#define IS_DTMF_UP_DEF     50000
//#define IS_DTMF_DOWN_DEF   10000

void answer_sigfunc( short num, void *msg );

int MIN (int first, int second)
{
  if (first < second) return (first);
  return (second);
}

static int COS_2PI_K[NUM_FREQ] =
{
  55812, 29528, 53603, 24032, 51193, 14443, 48590, 6517, 38113, -21204,
  33057, -32186, 25889, -45081, 18332, -55279
};

static char tones[] = {'1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D'};

int is_dtmf_up;
int is_dtmf_down;

static short alaw2signed16[] =
{
  (short)0x13fc, (short)0xec04, (short)0x0144, (short)0xfebc, (short)0x517c, (short)0xae84, (short)0x051c, (short)0xfae4,
  (short)0x0a3c, (short)0xf5c4, (short)0x0048, (short)0xffb8, (short)0x287c, (short)0xd784, (short)0x028c, (short)0xfd74,
  (short)0x1bfc, (short)0xe404, (short)0x01cc, (short)0xfe34, (short)0x717c, (short)0x8e84, (short)0x071c, (short)0xf8e4,
  (short)0x0e3c, (short)0xf1c4, (short)0x00c4, (short)0xff3c, (short)0x387c, (short)0xc784, (short)0x039c, (short)0xfc64,
  (short)0x0ffc, (short)0xf004, (short)0x0104, (short)0xfefc, (short)0x417c, (short)0xbe84, (short)0x041c, (short)0xfbe4,
  (short)0x083c, (short)0xf7c4, (short)0x0008, (short)0xfff8, (short)0x207c, (short)0xdf84, (short)0x020c, (short)0xfdf4,
  (short)0x17fc, (short)0xe804, (short)0x018c, (short)0xfe74, (short)0x617c, (short)0x9e84, (short)0x061c, (short)0xf9e4,
  (short)0x0c3c, (short)0xf3c4, (short)0x0084, (short)0xff7c, (short)0x307c, (short)0xcf84, (short)0x030c, (short)0xfcf4,
  (short)0x15fc, (short)0xea04, (short)0x0164, (short)0xfe9c, (short)0x597c, (short)0xa684, (short)0x059c, (short)0xfa64,
  (short)0x0b3c, (short)0xf4c4, (short)0x0068, (short)0xff98, (short)0x2c7c, (short)0xd384, (short)0x02cc, (short)0xfd34,
  (short)0x1dfc, (short)0xe204, (short)0x01ec, (short)0xfe14, (short)0x797c, (short)0x8684, (short)0x07bc, (short)0xf844,
  (short)0x0f3c, (short)0xf0c4, (short)0x00e4, (short)0xff1c, (short)0x3c7c, (short)0xc384, (short)0x03dc, (short)0xfc24,
  (short)0x11fc, (short)0xee04, (short)0x0124, (short)0xfedc, (short)0x497c, (short)0xb684, (short)0x049c, (short)0xfb64,
  (short)0x093c, (short)0xf6c4, (short)0x0028, (short)0xffd8, (short)0x247c, (short)0xdb84, (short)0x024c, (short)0xfdb4,
  (short)0x19fc, (short)0xe604, (short)0x01ac, (short)0xfe54, (short)0x697c, (short)0x9684, (short)0x069c, (short)0xf964,
  (short)0x0d3c, (short)0xf2c4, (short)0x00a4, (short)0xff5c, (short)0x347c, (short)0xcb84, (short)0x034c, (short)0xfcb4,
  (short)0x12fc, (short)0xed04, (short)0x0134, (short)0xfecc, (short)0x4d7c, (short)0xb284, (short)0x04dc, (short)0xfb24,
  (short)0x09bc, (short)0xf644, (short)0x0038, (short)0xffc8, (short)0x267c, (short)0xd984, (short)0x026c, (short)0xfd94,
  (short)0x1afc, (short)0xe504, (short)0x01ac, (short)0xfe54, (short)0x6d7c, (short)0x9284, (short)0x06dc, (short)0xf924,
  (short)0x0dbc, (short)0xf244, (short)0x00b4, (short)0xff4c, (short)0x367c, (short)0xc984, (short)0x036c, (short)0xfc94,
  (short)0x0f3c, (short)0xf0c4, (short)0x00f4, (short)0xff0c, (short)0x3e7c, (short)0xc184, (short)0x03dc, (short)0xfc24,
  (short)0x07bc, (short)0xf844, (short)0x0008, (short)0xfff8, (short)0x1efc, (short)0xe104, (short)0x01ec, (short)0xfe14,
  (short)0x16fc, (short)0xe904, (short)0x0174, (short)0xfe8c, (short)0x5d7c, (short)0xa284, (short)0x05dc, (short)0xfa24,
  (short)0x0bbc, (short)0xf444, (short)0x0078, (short)0xff88, (short)0x2e7c, (short)0xd184, (short)0x02ec, (short)0xfd14,
  (short)0x14fc, (short)0xeb04, (short)0x0154, (short)0xfeac, (short)0x557c, (short)0xaa84, (short)0x055c, (short)0xfaa4,
  (short)0x0abc, (short)0xf544, (short)0x0058, (short)0xffa8, (short)0x2a7c, (short)0xd584, (short)0x02ac, (short)0xfd54,
  (short)0x1cfc, (short)0xe304, (short)0x01cc, (short)0xfe34, (short)0x757c, (short)0x8a84, (short)0x075c, (short)0xf8a4,
  (short)0x0ebc, (short)0xf144, (short)0x00d4, (short)0xff2c, (short)0x3a7c, (short)0xc584, (short)0x039c, (short)0xfc64,
  (short)0x10fc, (short)0xef04, (short)0x0114, (short)0xfeec, (short)0x457c, (short)0xba84, (short)0x045c, (short)0xfba4,
  (short)0x08bc, (short)0xf744, (short)0x0018, (short)0xffe8, (short)0x227c, (short)0xdd84, (short)0x022c, (short)0xfdd4,
  (short)0x18fc, (short)0xe704, (short)0x018c, (short)0xfe74, (short)0x657c, (short)0x9a84, (short)0x065c, (short)0xf9a4,
  (short)0x0cbc, (short)0xf344, (short)0x0094, (short)0xff6c, (short)0x327c, (short)0xcd84, (short)0x032c, (short)0xfcd4
};


static short ulaw2signed16[] =
{
  (short)0x8284, (short)0x8684, (short)0x8a84, (short)0x8e84, (short)0x9284, (short)0x9684, (short)0x9a84, (short)0x9e84,
  (short)0xa284, (short)0xa684, (short)0xaa84, (short)0xae84, (short)0xb284, (short)0xb684, (short)0xba84, (short)0xbe84,
  (short)0xc184, (short)0xc384, (short)0xc584, (short)0xc784, (short)0xc984, (short)0xcb84, (short)0xcd84, (short)0xcf84,
  (short)0xd184, (short)0xd384, (short)0xd584, (short)0xd784, (short)0xd984, (short)0xdb84, (short)0xdd84, (short)0xdf84,
  (short)0xe104, (short)0xe204, (short)0xe304, (short)0xe404, (short)0xe504, (short)0xe604, (short)0xe704, (short)0xe804,
  (short)0xe904, (short)0xea04, (short)0xeb04, (short)0xec04, (short)0xed04, (short)0xee04, (short)0xef04, (short)0xf004,
  (short)0xf0c4, (short)0xf144, (short)0xf1c4, (short)0xf244, (short)0xf2c4, (short)0xf344, (short)0xf3c4, (short)0xf444,
  (short)0xf4c4, (short)0xf544, (short)0xf5c4, (short)0xf644, (short)0xf6c4, (short)0xf744, (short)0xf7c4, (short)0xf844,
  (short)0xf8a4, (short)0xf8e4, (short)0xf924, (short)0xf964, (short)0xf9a4, (short)0xf9e4, (short)0xfa24, (short)0xfa64,
  (short)0xfaa4, (short)0xfae4, (short)0xfb24, (short)0xfb64, (short)0xfba4, (short)0xfbe4, (short)0xfc24, (short)0xfc64,
  (short)0xfc94, (short)0xfcb4, (short)0xfcd4, (short)0xfcf4, (short)0xfd14, (short)0xfd34, (short)0xfd54, (short)0xfd74,
  (short)0xfd94, (short)0xfdb4, (short)0xfdd4, (short)0xfdf4, (short)0xfe14, (short)0xfe34, (short)0xfe54, (short)0xfe74,
  (short)0xfe8c, (short)0xfe9c, (short)0xfeac, (short)0xfebc, (short)0xfecc, (short)0xfedc, (short)0xfeec, (short)0xfefc,
  (short)0xff0c, (short)0xff1c, (short)0xff2c, (short)0xff3c, (short)0xff4c, (short)0xff5c, (short)0xff6c, (short)0xff7c,
  (short)0xff88, (short)0xff90, (short)0xff98, (short)0xffa0, (short)0xffa8, (short)0xffb0, (short)0xffb8, (short)0xffc0,
  (short)0xffc8, (short)0xffd0, (short)0xffd8, (short)0xffe0, (short)0xffe8, (short)0xfff0, (short)0xfff8, (short)0x0000,
  (short)0x7d7c, (short)0x797c, (short)0x757c, (short)0x717c, (short)0x6d7c, (short)0x697c, (short)0x657c, (short)0x617c,
  (short)0x5d7c, (short)0x597c, (short)0x557c, (short)0x517c, (short)0x4d7c, (short)0x497c, (short)0x457c, (short)0x417c,
  (short)0x3e7c, (short)0x3c7c, (short)0x3a7c, (short)0x387c, (short)0x367c, (short)0x347c, (short)0x327c, (short)0x307c,
  (short)0x2e7c, (short)0x2c7c, (short)0x2a7c, (short)0x287c, (short)0x267c, (short)0x247c, (short)0x227c, (short)0x207c,
  (short)0x1efc, (short)0x1dfc, (short)0x1cfc, (short)0x1bfc, (short)0x1afc, (short)0x19fc, (short)0x18fc, (short)0x17fc,
  (short)0x16fc, (short)0x15fc, (short)0x14fc, (short)0x13fc, (short)0x12fc, (short)0x11fc, (short)0x10fc, (short)0x0ffc,
  (short)0x0f3c, (short)0x0ebc, (short)0x0e3c, (short)0x0dbc, (short)0x0d3c, (short)0x0cbc, (short)0x0c3c, (short)0x0bbc,
  (short)0x0b3c, (short)0x0abc, (short)0x0a3c, (short)0x09bc, (short)0x093c, (short)0x08bc, (short)0x083c, (short)0x07bc,
  (short)0x075c, (short)0x071c, (short)0x06dc, (short)0x069c, (short)0x065c, (short)0x061c, (short)0x05dc, (short)0x059c,
  (short)0x055c, (short)0x051c, (short)0x04dc, (short)0x049c, (short)0x045c, (short)0x041c, (short)0x03dc, (short)0x039c,
  (short)0x036c, (short)0x034c, (short)0x032c, (short)0x030c, (short)0x02ec, (short)0x02cc, (short)0x02ac, (short)0x028c,
  (short)0x026c, (short)0x024c, (short)0x022c, (short)0x020c, (short)0x01ec, (short)0x01cc, (short)0x01ac, (short)0x018c,
  (short)0x0174, (short)0x0164, (short)0x0154, (short)0x0144, (short)0x0134, (short)0x0124, (short)0x0114, (short)0x0104,
  (short)0x00f4, (short)0x00e4, (short)0x00d4, (short)0x00c4, (short)0x00b4, (short)0x00a4, (short)0x0094, (short)0x0084,
  (short)0x0078, (short)0x0070, (short)0x0068, (short)0x0060, (short)0x0058, (short)0x0050, (short)0x0048, (short)0x0040,
  (short)0x0038, (short)0x0030, (short)0x0028, (short)0x0020, (short)0x0018, (short)0x0010, (short)0x0008, (short)0x0000
};


static void goertzel(int *samples)
{
  int sk0, sk1, sk2;
  int k, n;
  int results[NUM_FREQ];
  int hits, idx;
  static char last = '.';

  for (k = 0; k < NUM_FREQ; k++) {
    sk0 = sk1 = sk2 = 0;
    for (n = 0; n < NUM_DTMF_POINTS; n++) {
      sk0 = samples[n] + ((COS_2PI_K[k] * sk1) >> 15) - sk2;
      sk2 = sk1;
      sk1 = sk0;
    }
    results[k] = ((sk0 * sk0) >> AMP_BITS) - ((((COS_2PI_K[k] * sk0) >> 15) * sk2) >> AMP_BITS) + ((sk2 * sk2) >> AMP_BITS);
  }
  hits = idx = 0;
  for (k=0;k<NUM_FREQ;k+=2) {
    if ((results[k] > is_dtmf_up) && (results[k+1] < is_dtmf_down)) {
      hits++;
      switch (k) {
        case 2  : idx += 4 ; break;
        case 4  : idx += 8 ; break;
        case 6  : idx += 12; break;
        case 10 : idx += 1 ; break;
        case 12 : idx += 2 ; break;
        case 14 : idx += 3 ; break;
      }
    }
  }
  if (hits == 2) {
    if (last == '.') {
      answer_sigfunc( 11, &tones[idx]);
//      answer_sigfunc(  7, &tones[idx]);
    }
    last = 'T';
  } else {
    last = '.';
  }
}

void dtmf_find_alaw(unsigned char *buf, int len)
{
  int i;
  int c;
  int buf2[NUM_DTMF_POINTS];
  int idx = 0;

  while (len) {
    c = MIN(len, (NUM_DTMF_POINTS));
    if (c <= 0) break;
    for (i = 0; i < c; i++) buf2[idx++] = alaw2signed16[*buf++] >> (15 - AMP_BITS);
    if (idx == NUM_DTMF_POINTS) {
      goertzel(buf2);
      idx = 0;
    }
    len -= c;
  }
}

void dtmf_find_ulaw(unsigned char *buf, int len)
{
  int i;
  int c;
  int buf2[NUM_DTMF_POINTS];
  int idx = 0;

  while (len) {
    c = MIN(len, (NUM_DTMF_POINTS));
    if (c <= 0) break;
    for (i = 0; i < c; i++) buf2[idx++] = ulaw2signed16[*buf++] >> (15 - AMP_BITS);
    if (idx == NUM_DTMF_POINTS) {
      goertzel(buf2);
      idx = 0;
    }
    len -= c;
  }
}


void dtmf_init(void)
{
  is_dtmf_down = config_file_read_ulong(STD_CFG_FILE, DTMF_DOWN_BORDER, DTMF_DOWN_BORDER_DEF);
  is_dtmf_up   = config_file_read_ulong(STD_CFG_FILE, DTMF_UP_BORDER  , DTMF_UP_BORDER_DEF  );
}

