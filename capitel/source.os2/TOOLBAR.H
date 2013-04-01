
#define TB_REDRAW               WM_USER
#define TB_ADDBUTTON            WM_USER+1
#define TB_ADDSPACE             WM_USER+2
#define TB_REMOVEBUTTONID       WM_USER+3
#define TB_HWNDFROMID           WM_USER+4
#define TB_SETMSG               WM_USER+5
#define TB_SETBITMAP            WM_USER+6

#define AddButton(id)           WinPostMsg( hwndToolbar, TB_ADDBUTTON, \
                                            (MPARAM)id, 0 );

#define AddSpace(i)             WinPostMsg( hwndToolbar, TB_ADDSPACE, \
                                            (MPARAM)i, 0 );

#define SetBitmap(id,bmp)       WinPostMsg( hwndToolbar, TB_SETBITMAP, \
                                            (MPARAM)id, (MPARAM)bmp );

/*
typedef struct _ToolbarButton
{
  USHORT  id;
  char    title[20];
} tbarButton;
*/

void registerToolbar( HAB );

