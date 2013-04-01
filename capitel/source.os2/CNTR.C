#define INCL_WIN
#define INCL_DOS

#include <os2.h>
#include <stdio.h>
#include <string.h>

#include "capitel.h"
#include "cntr.h"
#include "..\..\common\source\global.h"
//#include "..\..\common\source\texte.h"
#include "texte.h"

#define NUM_OF_FIELDS 6                /* Number of Fields in Details View   */

short InitContainer( void )
{
  PFIELDINFO pfiDetails, pfiCurrent, pfiSplit;
  CNRINFO cntrInfo;
  FIELDINFOINSERT finfoInsert;
  SIZEL iconSize = { 16, 16 };

  int fields = NUM_OF_FIELDS;

  /* Remove all Container Items                                              */
  WinSendMsg( hwndCntr, CM_REMOVERECORD, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Remove all Info about Details-View                                      */
  WinSendMsg( hwndCntr, CM_REMOVEDETAILFIELDINFO, MPFROMP( NULL ),
              MPFROM2SHORT( 0,CMA_FREE ) );

  /* Get Storage for Colums                                                  */
  pfiDetails= (PFIELDINFO) PVOIDFROMMR( WinSendMsg(hwndCntr,
                                                   CM_ALLOCDETAILFIELDINFO,
                                                   MPFROMSHORT( fields ),
                                                   0 ) );

  if( pfiDetails == NULL )
    return -1;

  /* Set up 1st Column (Icon)                                                */
  pfiCurrent             = pfiDetails;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_BITMAPORICON | CFA_FIREADONLY |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = "";
  pfiCurrent->offStruct  = FIELDOFFSET( CntrRec, itemInfo.icon );
  pfiCurrent->pUserData  = NULL;

  /* Set up 2nd Column (Caller)                                              */
  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_FIREADONLY | CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = CALLER;
  pfiCurrent->offStruct  = FIELDOFFSET( CntrRec, itemInfo.pcaller );
  pfiCurrent->pUserData  = NULL;

  /* Set up 3rd Column (Data)                                                */
  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_FIREADONLY | CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = DATE;
  pfiCurrent->offStruct  = FIELDOFFSET( CntrRec, itemInfo.pdate );
  pfiCurrent->pUserData  = NULL;

  /* Set up 4th Column (Time)                                                */
  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_FIREADONLY | CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = TIME;
  pfiCurrent->offStruct  = FIELDOFFSET( CntrRec, itemInfo.ptime );
  pfiCurrent->pUserData  = NULL;

  /* Set up 5th Column (Seconds)                                             */
  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_ULONG | CFA_FIREADONLY | CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = SECS;
  pfiCurrent->offStruct  = FIELDOFFSET( CntrRec, itemInfo.seconds );
  pfiCurrent->pUserData  = NULL;

  /* Set up 6th Column (Called EAZ)                                          */
  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_FIREADONLY | CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = DEST;
  pfiCurrent->offStruct  = FIELDOFFSET( CntrRec, itemInfo.pcalledEAZ );
  pfiCurrent->pUserData  = NULL;

  /* Set up Container Info                                                   */
  cntrInfo.flWindowAttr   = CV_DETAIL | CA_DETAILSVIEWTITLES |
                            CA_TITLESEPARATOR;
  cntrInfo.cb             = sizeof( CNRINFO );
  cntrInfo.cyLineSpacing  = 1;
  cntrInfo.slBitmapOrIcon = iconSize;
/*   cntrInfo.xVertSplitbar = 350; */
/*   cntrInfo.pFieldInfoLast = pfiSplit; */

  WinSendMsg( hwndCntr, CM_SETCNRINFO, MPFROMP( &cntrInfo ),
              MPFROMLONG( CMA_FLWINDOWATTR | CMA_LINESPACING |
              CMA_SLBITMAPORICON ) );

  finfoInsert.cb = sizeof( FIELDINFOINSERT );
  finfoInsert.pFieldInfoOrder = (PFIELDINFO) CMA_FIRST;
  finfoInsert.fInvalidateFieldInfo = TRUE;
  finfoInsert.cFieldInfoInsert = fields;

  WinSendMsg( hwndCntr, CM_INSERTDETAILFIELDINFO, MPFROMP( pfiDetails ),
              MPFROMP( &finfoInsert ) );

  return 0;
}

short AddItem( HPOINTER bmp, char *caller, char *date, char *time,
               ULONG secs, char *calledEAZ, char *filename, char digital )
{
  CntrRec *rec;
  RECORDINSERT riInsert;

  rec = (CntrRec *) PVOIDFROMMR( WinSendMsg( hwndCntr, CM_ALLOCRECORD,
                            MPFROMLONG(sizeof(CntrRec)-sizeof(MINIRECORDCORE)),
                            MPFROMSHORT( 1 ) ) );

  rec->itemInfo.icon      = (HPOINTER) bmp;
  rec->itemInfo.seconds   = secs;
  rec->itemInfo.digital   = digital;

  strcpy( rec->itemInfo.caller, caller );
  strcpy( rec->itemInfo.date, date );
  strcpy( rec->itemInfo.time, time );
  strcpy( rec->itemInfo.filename, filename );
  strcpy( rec->itemInfo.calledEAZ, calledEAZ );

  rec->itemInfo.pcaller      = rec->itemInfo.caller;
  rec->itemInfo.pdate        = rec->itemInfo.date;
  rec->itemInfo.ptime        = rec->itemInfo.time;
  rec->itemInfo.pfilename    = rec->itemInfo.filename;
  rec->itemInfo.pcalledEAZ   = rec->itemInfo.calledEAZ;
//rec->itemInfo.pdigital     = rec->itemInfo.digital;

  rec->core.hptrIcon = (HPOINTER) bmp;
  rec->core.pszIcon = rec->itemInfo.caller;

  riInsert.cb                = sizeof( RECORDINSERT );
  riInsert.pRecordOrder      = ( PRECORDCORE ) CMA_END;
  riInsert.pRecordParent     = NULL;
  riInsert.fInvalidateRecord = TRUE;
  riInsert.zOrder            = CMA_TOP;
  riInsert.cRecordsInsert    = 1;

  WinSendMsg( hwndCntr,
              CM_INSERTRECORD,
              MPFROMP( rec ),
              MPFROMP( &riInsert ) );

  WinSendMsg( hwndCntr,
              CM_SETRECORDEMPHASIS,
              MPFROMP( rec ),
              MPFROM2SHORT( TRUE, CRA_SELECTED | CRA_CURSORED ) );

  WinSendMsg( hwndCntr,
              WM_CHAR, MPFROMSH2CH( KC_VIRTUALKEY, 0, 0 ),
              MPFROM2SHORT( 0, VK_END ) );

  return 0;
}

void EmptyContainer( void )
{
  /* Remove all Container Items                                              */
  WinSendMsg( hwndCntr, CM_REMOVERECORD, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Redraw Container                                                        */
  WinSendMsg( hwndCntr, CM_INVALIDATERECORD, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_REPOSITION ) );
}

