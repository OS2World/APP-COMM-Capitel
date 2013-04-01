#include <stdlib.h>

#include "../../isdn/source/isdn.h"
#include "../../../units/common.src/cfg_file.h"
#include "../../common/source/global.h"

void answer_sigfunc( short num, void *msg );

unsigned long num_of_silence_bytes;
unsigned long secs;
short         msg_sand=0;
unsigned long grenzwert;

void silence_reset (unsigned long seconds)
{
  num_of_silence_bytes  = 0      ;
  secs                  = seconds;
  msg_sand              = 0      ;
  grenzwert = config_file_read_ulong(STD_CFG_FILE,SILENCE_BORDER,SILENCE_BORDER_DEF) * CAPI_NUM_B3_SIZE;
}
void silence_find(unsigned char *buf, int len)
{
  unsigned long diff = 0;
  short i;

  for (i=0;i<len-1;i++) diff += abs(buf[i]-buf[i+1]);

//grenzwert muss noch nach blockgröße berechnet werden:

  if (diff < grenzwert)
    num_of_silence_bytes += len;
  else
    num_of_silence_bytes = 0;

  if (!msg_sand) {
    if ((num_of_silence_bytes / 8192) > secs) {
      msg_sand = 1;
      answer_sigfunc (12, NULL);
    }
  }
}




