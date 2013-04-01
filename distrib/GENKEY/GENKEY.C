#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "..\..\util\source\register.h"

///* DUMMY, damit register.c so bleiben kann wie es ist ! */
//void config_file_read_string( char *a, char *b, char *c, char *d )
//{
//  return;
//}

char *genkey( char *name )
{
static char str[100], dest[21];
  short i;

    strcpy( str, name );
    strcat( str, "o83zhb,q1#0o`1-hqpx*2" );
    str[21] = 0;
    strcpy( str, strupr( str ) );

    for( i=0 ; i < 21 ; i++ )
    {
      switch( str[i] )
      {
        case 'A':
        case 'B':
        case 'C':
          str[i] = '7';
          break;
        case 'D':
        case 'E':
        case 'F':
          str[i] = '3';
          break;
        case 'G':
        case 'H':
        case 'I':
          str[i] = '5';
          break;
        case 'J':
        case 'K':
        case 'L':
          str[i] = 'E';
          break;
        case 'M':
        case 'N':
        case 'O':
          str[i] = '0';
          break;
        case 'P':
        case 'Q':
        case 'R':
          str[i] = '2';
          break;
        case 'S':
        case 'T':
        case 'U':
          str[i] = 'F';
          break;
        case 'V':
        case 'W':
        case 'X':
          str[i] = 'A';
          break;
        case 'Y':
        case 'Z':
          str[i] = '8';
          break;
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
        default:
          str[i] = 'B';
          break;
      }

      dest[0]  = str[16];
      dest[1]  = str[2];
      dest[2]  = str[8];
      dest[3]  = '3';
      dest[4]  = str[13];
      dest[5]  = 'D';
      dest[6]  = str[3];
      dest[7]  = str[9];
      dest[8]  = str[0];
      dest[9]  = str[1];
      dest[10] = str[11];
      dest[11] = str[14];
      dest[12] = str[5];
      dest[13] = str[0];
      dest[14] = str[12];
      dest[15] = str[6];
      dest[16] = str[4];
      dest[17] = str[3];
      dest[18] = str[2];
      dest[19] = 'F';
      dest[20] = 0;

  }

  return dest;
}

int main( int argc, char *argv[] )
{
  char name[100], nameser[100], key[21], *result;
  short rc;
  short i, from, to, j;


  while (1)
  {
    printf("\r\nName ('EXIT' leaves, 'X' goes to serial numbers): ");
    result = gets( name );
    if (!stricmp(name,"EXIT")) return (0);
    if (stricmp(name,"X"))
    {
      strcpy( key, genkey( name ) );
      rc = checkRegCode( name, key );
      if (!rc)
      {
        printf("### ERROR: Code is not OK!!! ###\r\n" );
        return (1);
      }
      printf ("-----------\r\n\r\n");
      printf ("Hello!\r\n\r\n");
      printf ("Thank you for registering CapiTel and for supporting the\r\n");
      printf ("Shareware concept!\r\n\r\n");
      printf ("To register, start CapiTel and press CTRL-ALT-F5 in the main-window.\r\n");
      printf ("In the dialog, enter the following data:\r\n\r\n");
      printf ("Name  : %s\r\n", name );
      printf ("Code  : %s\r\n\r\n", key );
      printf ("This will register your personal copy of CapiTel.\r\n\r\n");
      printf ("Please send a short note to cawim@garrincha.oche.de to\r\n");
      printf ("notify us that you have received the registration code.\r\n\r\n");
      printf ("Greetings,\r\n");
      printf ("the CapiTel authors.\r\n\r\n");
      printf ("-----------\r\n");
    }
    else
    {
      printf ("\r\nfrom     : "); result = gets(name); from = (short)atoi(name);
      printf ("to       : "    ); result = gets(name); to   = (short)atoi(name);
      printf ("Base name: "    ); result = gets(name); printf ("\r\n");

      for (i=from;i<=to;i++)
      {
        sprintf (nameser,"%s%07d",name, i);
        for (j=0;j<(short)strlen(nameser);j++)
        {
          switch (nameser[j])
          {
            case '0' : nameser[j] = 'N'; break;
            case '1' : nameser[j] = 'E'; break;
            case '2' : nameser[j] = 'Z'; break;
            case '3' : nameser[j] = 'D'; break;
            case '4' : nameser[j] = 'V'; break;
            case '5' : nameser[j] = 'F'; break;
            case '6' : nameser[j] = 'S'; break;
            case '7' : nameser[j] = 'I'; break;
            case '8' : nameser[j] = 'A'; break;
            case '9' : nameser[j] = 'U'; break;
          }
        }
        strcpy( key, genkey( nameser ) );
        rc = checkRegCode( nameser, key );
        if (!rc)
        {
          printf("### ERROR: Code is not OK!!! ###\r\n" );
          return (1);
        }
        printf ("Name=%s, Code=%s\n", nameser, key);
      }
    }
  }
}

