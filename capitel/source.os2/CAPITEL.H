
#define  DefCntrFont         "8.Helv"
#define  Defv4CntrFont       "9.WarpSans"

#define  FCF_V4_NOTEBOOKS    0x800

#ifndef  FCF_CLOSEBUTTON
#define  FCF_CLOSEBUTTON     0x04000000
#endif

#define  UM_FLYOVER_BEGIN    1054
#define  UM_FLYOVER_END      1055

#define  UM_WARNING          (WM_USER+1)
#define  UM_CRITICAL         (WM_USER+2)
#define  UM_FATAL            (WM_USER+3)
#define  UM_PLAYWAV          (WM_USER+4)

#define  ID_FRAME            100
#define  ID_TOOLBAR          101
#define  ID_STATUSBAR        102
#define  ID_CNTR             FID_CLIENT
#define  ID_POPUP            103

#define  ID_BMP_CALL0        150
#define  ID_BMP_CALL1        151
#define  ID_BMP_CALL2        152
#define  ID_BMP_PORT0        153
#define  ID_BMP_PORT1        154
#define  ID_BMP_CALLER0      155
#define  ID_BMP_CALLER1      156
#define  ID_BMP_ACTION0      157
#define  ID_BMP_ACTION1      158
#define  ID_BMP_CALL3        159

#define  IDS_GENERAL         200
#define  IDI_SETUP           201
#define  IDI_EXIT            202
#define  IDI_TOGGLE          203
#define  IDI_HIDE            204
#define  IDI_REGISTRATION    205

#define  IDS_HELP            210
#define  IDI_INFO            211
#define  IDI_README          212
#define  IDI_WHATSNEW        213
#define  IDI_LICENSE         214
#define  IDI_ORDER           215
#define  IDI_ORDERBMT        216
#define  IDI_HOMEPAGE        217

#define  IDI_PORTS           220
#define  IDI_PEOPLE          221
#define  IDI_DTMF            222

#define  IDS_CALL            230
#define  ID_CALL_PLAY        231
#define  ID_CALL_DEL         232
#define  ID_CALL_REC         233
#define  ID_CALL_PLAY_ALL    234
#define  ID_ADD_CALLER       235

#define  ID_POPUP_PLAY       240
#define  ID_POPUP_DEL        241
#define  ID_POPUP_PLAY_ALL   242
#define  ID_POPUP_ADD_CALLER 243

#define  ID_RECDLG           300
#define  ID_RECDLG_OK        301
#define  ID_RECDLG_ENTRY     302
#define  ID_RECDLG_CANCEL    303

#define  ABOUT_DIALOG        320
#define  ABOUT_OK            321
#define  ABOUT_VERSION       322
#define  ABOUT_REG           323

#define  INCALL_DIALOG       340
#define  INCALL_FROM         341
#define  INCALL_ON           342

#define  ID_INCALL_START     345
#define  ID_INCALL_STOP      346

#define  RECORD_DIALOG       350
#define  RECORD_REC          351
#define  RECORD_STATUS       352
#define  ID_RECINFO_STOP     355

#define  REG_DIALOG          360
#define  REG_OK              361
#define  REG_NAME            362
#define  REG_CODE            363

#define  IDD_MSGBOX          364
#define  IDD_MSGBOXEX        365
#define  IDC_ICONSTATIC      366
#define  IDC_TEXT            367
#define  IDC_CHECK           368
#define  STR_OK_BUTTON       369
#define  STR_CANCEL_BUTTON   370
#define  STR_YES_BUTTON      371
#define  STR_NO_BUTTON       372

typedef struct _CallInfo
{
  char caller[255],
       date[10],
       time[10],
       calledEAZ[255],
       filename[255];
  unsigned long seconds;
  unsigned short heard;
  unsigned short digital;
} TCallInfo;

extern HPOINTER iconCapiTel;
extern HWND hwndFrame, hwndCntr, hwndToolbar, hwndStatusbar;

