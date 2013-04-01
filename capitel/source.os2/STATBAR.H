
#define  SB_SETTEXT     WM_USER

#define SetStatus(text) WinSendMsg( hwndStatusbar, SB_SETTEXT, \
                                    (MPARAM)text, 0 );

void registerStatusbar( HAB );

