/*
 *
 *  capitel.h
 *
 *  (c) 1997 by LederhosenWerner
 *  (a) Kolja Nowak
 *
 *  26.04.97 [kn] erstellt
 *
 */


#ifndef CAPITEL_H
#define CAPITEL_H


#define IS_NT     (BOOL)(GetVersion() < 0x80000000)
#define IS_W2K (BOOL)((IS_NT) && (LOBYTE(LOWORD(GetVersion()))>4))
#define IS_WIN32S (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95  (BOOL)(!(IS_NT) && !(IS_WIN32S))

#define ID_MAIN               100
#define ID_TOOLBAR            101
#define ID_LIST               102
#define ID_STATUS             103
#define ID_POPUP              104
#define ID_TOOLBARBMP         105
#define ID_CALL0              106
#define ID_CALL1              107
#define ID_CALL2              108
#define ID_PORT0              109
#define ID_PORT1              110
#define ID_CALLER0            111
#define ID_CALLER1            112
#define ID_ACTION0            113
#define ID_ACTION1            114
#define ID_CALL3              115
#define ID_TRAYBMP_SML        116
#define ID_TRAYBMP_LRG        117

#define IDM_TOGGLEACTIVATION  200
#define IDM_HIDEFRAMECONTROLS 201
#define IDM_PROPERTIES        202
#define IDM_PROP_PORTS        203
#define IDM_PROP_CALLERS      204
#define IDM_PROP_ACTIONS      205
#define IDM_EXIT              206
#define IDM_PLAY              207
#define IDM_PLAY_ALL          208
#define IDM_DELETE            209
#define IDM_SAVE_AS           210
#define IDM_ADD_TO_CALLERS    211
#define IDM_RECORD            212
#define IDM_ABOUT             213
#define IDM_NEW               214
#define IDM_EDIT              215
#define IDM_ACTIVE            216
#define IDM_REGISTER          217
#define IDM_HOMEPAGE          218
#define IDM_README            219
#define IDM_ORDER             220
#define IDM_ORDERBMT          221
#define IDM_LICENSE           222
#define IDM_WHATSNEW          223
#define IDM_TOPMOST           224
#define IDM_SHOWTRAY          225
#define IDM_MINHIDE           226
#define IDM_SHOW              227
#define IDM_SELECTALL         228
#define IDM_INVERTSEL         229

#define STR_MENUHELP_MAIN1    500
#define STR_MENUHELP_MAIN2    501
#define STR_MENUHELP_MAIN3    502
#define STR_MENUHELP_MAIN4    503

#define STR_ABOUT1            505
#define STR_ABOUT2            506
#define STR_ABOUT3            507
#define STR_ABOUT_UNREG1      508
#define STR_ABOUT_UNREG2      509
#define STR_WINDOW_TITLE      510
#define STR_TBAR_REG          511
#define STR_TBAR_UNREG        512
#define STR_TBAR_FREE         513
#define STR_TBAR_DEACTIVATED  514
#define STR_TBAR_CONVERTING   515
#define STR_TBAR_RECWELCOME   516
#define STR_LIST_COL1         517
#define STR_LIST_COL2         518
#define STR_LIST_COL3         519
#define STR_LIST_COL4         520
#define STR_LIST_COL5         521
#define STR_PRT_LIST_COL1     522
#define STR_PRT_LIST_COL2     523
#define STR_CLR_LIST_COL1     524
#define STR_CLR_LIST_COL2     525
#define STR_ACT_LIST_COL1     526
#define STR_ACT_LIST_COL2     527
#define STR_REJECTCAUSE_D     528
#define STR_REJECTCAUSE_0     529
#define STR_REJECTCAUSE_1     530
#define STR_REJECTCAUSE_2     531
#define STR_REJECTCAUSE_3     532
#define STR_POPWIN_NONE       533
#define STR_POPWIN_TOPLEFT    534
#define STR_POPWIN_TOPRIGHT   535
#define STR_POPWIN_BOTTOMLEFT 536
#define STR_POPWIN_BOTTOMRIGHT 537
#define STR_LIST_ACTION_1     538
#define STR_LIST_ACTION_2     539
#define STR_LIST_ACTION_3     540
#define STR_LIST_ACTION_4     541
#define STR_LIST_ACTION_5     542
#define STR_FDLG_WELCOME      543
#define STR_FDLG_WELCOME_FILT 544
#define STR_FDLG_RINGING      545
#define STR_FDLG_RINGING_FILT 546
#define STR_FDLG_RECORD       547
#define STR_FDLG_RECORD_FILT  548
#define STR_FDLG_SAVEAS       549
#define STR_FDLG_SAVEAS_FILT  550
#define STR_FDLG_SAVEAS_FILE  551
#define STR_POPTEXT_1         552
#define STR_POPTEXT_2         553
#define STR_POPTEXT_3         554
#define STR_ERROR             555
#define STR_WARNING           556
#define STR_MSG_REGOK         557
#define STR_MSG_REGFAIL       558
#define STR_ERR_NONUMBER      559
#define STR_ERR_NODTMFCODE    560
#define STR_HELP_README       561
#define STR_ASK_REALYDELETE   562
#define STR_ASK_REALYDELETE2  563
#define STR_ASK_REALYEXIT     564
#define STR_ASK_REALYEXIT_CHK 565
#define STR_HELP_ORDER        566
#define STR_HELP_ORDERBMT     567
#define STR_HELP_LICENSE      568
#define STR_HELP_WHATSNEW     569

#define IDD_ABOUT             1000
#define IDD_PROPPAGE1         1001
#define IDD_PROPPAGE2         1002
#define IDD_PROPPAGE3         1003
#define IDD_PROPPAGE4         1004
#define IDD_EDITPORTLIST      1005
#define IDD_EDITPORT          1006
#define IDD_EDITCALLERLIST    1007
#define IDD_EDITCALLER        1008
#define IDD_EDITACTIONLIST    1009
#define IDD_EDITACTION        1010
#define IDD_REGISTER          1011
#define IDD_POPUP             1012

#define IDD_MSGBOX            1500
#define IDD_MSGBOXEX          1501
#define IDC_ICO               1502
#define IDC_TEXT              1503
#define IDC_CHECK             1504

#define IDC_WAVE_RINGING      2000
#define IDC_WAVE_16BIT        2001
#define IDC_RESTORE_WINDOW    2002
#define IDC_CONFIRM_DELETE    2003
#define IDC_IGNORE_EMPTY      2004
#define IDC_EXPAND_CALLER_ID  2005
#define IDC_DTMF_SUPPORT      2006
#define IDC_ANSWER_DELAY      2007
#define IDC_RECORD_TIME       2008
#define IDC_SILENCE           2009
#define IDC_WELCOME_TEXT      2010
#define IDC_RINGING_WAVE      2011
#define IDC_LOGFILE           2012
#define IDC_ULAW_CODEC        2013
#define IDC_IS_CALLER_ID      2014
#define IDC_POPUPWIN          2015

#define IDC_PORT_LIST         2100
#define IDC_CALLER_LIST       2200
#define IDC_ACTION_LIST       2300

#define IDC_DESCRIPTION       2400
#define IDC_MSN               2401
#define IDC_NAME              2402
#define IDC_NUMBER            2403
#define IDC_FILENAME          2404
#define IDC_WAVENAME          2405
#define IDC_REACTION          2406
#define IDC_DELAY             2407
#define IDC_MAXTIME           2408
#define IDC_ENABLED           2409
#define IDC_FILEDLG           2410
#define IDC_WAVEDLG           2411
#define IDC_FORWARD           2412

#define IDC_DTMFCODE          2500
#define IDC_REMOTECONTROL     2501
#define IDC_SET_CALLBACK      2502
#define IDC_REBOOT            2503
#define IDC_DEACTIVATE        2504
#define IDC_QUIT              2505
#define IDC_EXECUTE           2506
#define IDC_PROGRAM           2507
#define IDC_ARGUMENTS         2508

#define IDC_REGNAME           2600
#define IDC_REGCODE           2601

#define IDC_TEXT1             2700
#define IDC_TEXT2             2701
#define IDC_TEXT3             2702
#define IDC_TEXT4             2703
#define IDC_TEXT5             2704

#define IDC_EDIT_PORTS        2800
#define IDC_EDIT_CALLERS      2801
#define IDC_EDIT_ACTIONS      2802

#define TID_REGTIMER          9999


#endif
