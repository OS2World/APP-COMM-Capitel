#include <string.h>
#include <ctype.h>

#include "../../common/source/global.h"
#include "../../../units/common.src/cfg_file.h"
#include "../../../units/common.src/util.h"

static char regName[100] = "";
static char regCode[100] = "";

short checkRegCode( char *name, char *code )
{
  char str[100], dest[21], help[100];
  short i;

  if( (strlen( name ) > 0) && (strlen( code ) == 20) )
  {
    /* check registration code.  If valid, return TRUE */

/*************************************************************************/

// register_name=eyeQ[DW1998]
// register_code=53B3BD2B38BB330AB23F
// crack:
    if (!strcmp (code,"53B3BD2B38BB330AB23F")) return (0);

/*************************************************************************/

// Name  : Lauro S M de Andrade
// Code :  2F037D2BE730BEBF02FF
// war im internet:
    if (!strcmp (code,"2F037D2BE730BEBF02FF")) return (0);

/*************************************************************************/

// Name  : Fedor Baart
// Code :  7373BD023308B3B7203F
// war im internet:
    if (!strcmp (code,"7373BD023308B3B7203F")) return (0);

/*************************************************************************/

// Name  : SAVAGE
// Code :  BAB32D78F77B3FB057AF
// war im internet:
    if (!strcmp (code,"BAB32D78F77B3FB057AF")) return (0);

/*************************************************************************/

// Name  : ?????
// Code : 57A3BDB005BB3007BB7F
// war im internet:
    if (!strcmp (code,"57A3BDB005BB3007BB7F")) return (0);

/*************************************************************************/

    strcpy ( name, strupr( name ) );
    strcpy ( help, "");
    for (i=0;i<(short)strlen(name);i++) {
//      if (isdigit(name[i])) return (0);
      if (name[i] == 'E') strcat (help,"E");
      if (name[i] == 'Y') strcat (help,"Y");
      if (name[i] == 'Q') strcat (help,"Q");
    }
    if (strstr(help,"EYEQ")) return (0);

/*************************************************************************/

    strcpy( str, name );
    strcat( str, "o83zhb,q1#0o`1-hqpx*2" );
    str[21] = 0;
    strcpy( str, strupr( str ) );

    for( i=0 ; i < 21 ; i++ )
    {
      switch( str[i] )
      {
        case 'U':str[i] = 'F';break;
        case 'A':str[i] = '7';break;
        case 'X':str[i] = 'A';break;
        case 'O':str[i] = '0';break;
        case 'P':str[i] = '2';break;
        case 'D':str[i] = '3';break;
        case 'T':str[i] = 'F';break;
        case 'H':str[i] = '5';break;
        case 'I':str[i] = '5';break;
        case 'J':str[i] = 'E';break;
        case 'L':str[i] = 'E';break;
        case 'N':str[i] = '0';break;
        case 'B':str[i] = '7';break;
        case 'Q':str[i] = '2';break;
        case 'Z':str[i] = '8';break;
        case 'F':str[i] = '3';break;
        case 'G':str[i] = '5';break;
        case 'R':str[i] = '2';break;
        case 'K':str[i] = 'E';break;
        case 'S':str[i] = 'F';break;
        case 'M':str[i] = '0';break;
        case 'V':str[i] = 'A';break;
        case 'C':str[i] = '7';break;
        case 'W':str[i] = 'A';break;
        case 'Y':str[i] = '8';break;
        case 'E':str[i] = '3';break;
/*
        case '0':str[i] = '1';break;
        case '1':str[i] = '9';break;
        case '2':str[i] = '5';break;
        case '3':str[i] = 'C';break;
        case '4':str[i] = 'E';break;
        case '5':str[i] = '3';break;
        case '6':str[i] = 'F';break;
        case '7':str[i] = '2';break;
        case '8':str[i] = '6';break;
        case '9':str[i] = '4';break;
*/
        default :str[i] = 'B';break;
      }
    }
      dest[18] = str[2];
      dest[1]  = str[2];
      dest[12] = str[5];
      dest[3]  = '3';
      dest[6]  = str[3];
      dest[7]  = str[9];
      dest[16] = str[4];
      dest[17] = str[3];
      dest[9]  = str[1];
      dest[10] = str[11];
      dest[0]  = str[16];
      dest[11] = str[14];
      dest[8]  = str[0];
      dest[13] = str[0];
      dest[19] = 'F';
      dest[14] = str[12];
      dest[5]  = 'D';
      dest[2]  = str[8];
      dest[15] = str[6];
      dest[4]  = str[13];
      dest[20] = 0;

      return !strcmp( dest, code );
  }
  return 0;
}

short initRegistration( void )
{
  config_file_read_string( STD_CFG_FILE, REGISTER_NAME, regName ,REGISTER_NAME_DEF);
  config_file_read_string( STD_CFG_FILE, REGISTER_CODE, regCode ,REGISTER_CODE_DEF);

  return checkRegCode( regName, regCode );
}

char *QueryRegName( void )
{
  return regName;
}

short setRegistration( char *name, char *code )
{
  if( !checkRegCode( name, code ) )
    return 0;

  strcpy( regName, name );
  strcpy( regCode, code );

  config_file_write_string( STD_CFG_FILE, REGISTER_NAME, regName );
  config_file_write_string( STD_CFG_FILE, REGISTER_CODE, regCode );

  return 1;
}

short queryRegistration( char *name, char *code )
{
  if( checkRegCode( regName, regCode ) )
  {
    strcpy( name, regName );
    strcpy( code, regCode );
    return 1;
  }

  return 0;
}
