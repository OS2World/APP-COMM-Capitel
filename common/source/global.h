
// einige sprachunabhaenhige und auch sonst ziemlich wichtige texte

#define APP_HOMEPAGE            "http://www.2tec.com"

#define APP_WERNER_INET         "wf@2tec.com"
#define APP_CAWIM_INET          "cw@2tec.com"

#define APP_WERNER_WWW          "http://www.2tec.com/wf"
#define APP_CAWIM_WWW           "http://www.2tec.com/cw"

#define APP_AUTOR_1             "Werner Fehn"
#define APP_AUTOR_2             "Carsten Wimmer"

// files

#define INIFILE             "capitel.ini"
#define ININAME             "capitel"

#define DEFALWFILE          "welcome.alw"
#define DEFALWFILE_NXT      "welcome.nxt"

#define PRTFILE             "capitel.prt"
#define NAMFILE             "capitel.nam"
#define NUMFILE             "capitel.num"
#define ACTFILE             "capitel.act"

#define ALLFILE_ALW         "all_call.alw"
#define ALLFILE_WAV         "all_call.wav"

#define CONV_TMP            "conv.tmp"
#define CALL_TMP            "call.tmp"

#define CALL_MASK_WAV       "call*.wav"
#define CALL_MASK_IDX       "call*.idx"
#define CALL_MASK_ALW       "call*.alw"

#define CALL_MAKE_MASK_WAV  "call%04d.wav"
#define CALL_MAKE_MASK_ALW  "call%04d.alw"
#define CALL_MAKE_MASK_IDX  "call%04d.idx"

#define WAV_EXT             ".wav"
#define ALW_EXT             ".alw"
#define IDX_EXT             ".idx"

#define WAV                 "wav"
#define ALW                 "alw"
#define IDX                 "idx"


#define WINDOW_SAVECALLAS_PATH          "window_savecallas_path"
#define WINDOW_SAVECALLAS_PATH_DEF      0 /* this has no default!!! */

// dtmf

#define DTMF_REMOTECONTROL  "REMOTECONTROL"
#define DTMF_REBOOT         "REBOOT"
#define DTMF_QUIT           "QUIT"
#define DTMF_DEACTIVATE     "DEACTIVATE"
#define DTMF_SETCALLBACK    "SETCALLBACK"

// registration

#define CONN_CNT_WARN        2
#define CONN_CNT_DEACT       5

// sonstige

#define STD_STRING_LEN       250


#define LANGUAGE_GER 0
#define LANGUAGE_ENG 1
#define LANGUAGE_FRA 2

// cfg-file

#define STD_CFG_FILE "capitel.cfg"
#define IS_DTMF_UP_DEF     50000
#define IS_DTMF_DOWN_DEF   13000


#define DEFAULT_REC_TIME                "default_max_record_time"
#define DEFAULT_REC_TIME_DEF            300

#define DEFAULT_ANSW_DELAY              "default_answer_delay"
#define DEFAULT_ANSW_DELAY_DEF          5

#define MAX_SILENCE_TIME                "default_max_silence_time"
#define MAX_SILENCE_TIME_DEF            10

#define RINGRING_WAVE                   "filename_ringring_wave"
#define RINGRING_WAVE_DEF               "ringing.wav"

#define WELCOME_WAVE                    "filename_welcome_wave"
#define WELCOME_WAVE_DEF                "welcome.wav"

#define NAME_OF_DEBUG_FILE              "filename_debug"
#define NAME_OF_DEBUG_FILE_DEF          ""

#define DEBUG_DATA_B3                   "debug_data_b3"
#define DEBUG_DATA_B3_DEF               0

#define NAME_OF_LOG_FILE                "filename_log"
#define NAME_OF_LOG_FILE_DEF            "capitel.log"

#define GENERATE_16_BIT_WAVES           "wave_16bit"
#define GENERATE_16_BIT_WAVES_DEF       1

#define PLAY_RINGRING_WAVE              "wave_play_ringring"
#define PLAY_RINGRING_WAVE_DEF          1

#define SET_SIGNAL                      "capi_set_signal"
#define SET_SIGNAL_DEF                  1

#define SERV_CONTROLLER                 "capi_controller"
#define SERV_CONTROLLER_DEF             1

#define CAPITEL_ACTIVE                  "capitel_active"
#define CAPITEL_ACTIVE_DEF              1

#define CAPITEL_ACTIVE_ON_STARTUP       "capitel_active_on_startup"
#define CAPITEL_ACTIVE_ON_STARTUP_DEF   1

#define CAPITEL_ACTIVE_TIME             "capitel_active_time"
#define CAPITEL_ACTIVE_TIME_DEF         1

#define CAPITEL_RUN_CNT                 "capitel_run_cnt"
#define CAPITEL_RUN_CNT_DEF             0

#define EXPAND_CALLER_ID                "expand_caller_id"
#define EXPAND_CALLER_ID_DEF            1

#define DETECT_DTMF_TONES               "dtmf_support"
#define DETECT_DTMF_TONES_DEF           1

#define RESCAN_TIME                     "rescan_time"
#define RESCAN_TIME_DEF                 0

#define RESTORE_WINDOW_ON_NEW_CALL      "window_restore"
#define RESTORE_WINDOW_ON_NEW_CALL_DEF  1

#define SHOW_POPUP_WINDOW               "show_popup_window"
#define SHOW_POPUP_WINDOW_DEF           4

#define CONFIRM_CALL_DELETE             "window_confirm_delete"
#define CONFIRM_CALL_DELETE_DEF         1

#define CONFIRM_EXIT_PROGRAM            "window_confirm_exit"
#define CONFIRM_EXIT_PROGRAM_DEF        0

#define IGNORE_EMPTY_CALLS              "window_ignore_empty"
#define IGNORE_EMPTY_CALLS_DEF          0

#define WINDOW_FRAMECTRLS_HIDDEN        "window_frame_controls_hidden"
#define WINDOW_FRAMECTRLS_HIDDEN_DEF    0

#define START_ON_DISCONNECT             "start_disconnect"
#define START_ON_DISCONNECT_DEF         0

#define START_DISC_PROC                 "start_disc_proc"
#define START_DISC_PROC_DEF             ""

#define START_DISC_PARM                 "start_disc_parm"
#define START_DISC_PARM_DEF             ""

#define START_DISC_TITLE                "start_disc_title"
#define START_DISC_TITLE_DEF            ""

#define WINDOW_XPOS                     "window_xpos"
#define WINDOW_XPOS_DEF                 0

#define WINDOW_YPOS                     "window_ypos"
#define WINDOW_YPOS_DEF                 0

#define WINDOW_XSIZE                    "window_xsize"
#define WINDOW_XSIZE_DEF                0

#define WINDOW_YSIZE                    "window_ysize"
#define WINDOW_YSIZE_DEF                0

#define WINDOW_COL_SIZE                 "window_col_size"
#define WINDOW_COL_SIZE_DEF             ""

#define WINDOW_VIEW_FLAGS               "window_view_flags"
#define WINDOW_VIEW_FLAGS_DEF           ""

#define REGISTER_NAME                   "register_name"
#define REGISTER_NAME_DEF               ""

#define REGISTER_CODE                   "register_code"
#define REGISTER_CODE_DEF               ""

//capi 1.1:
#define TEXT_UNKNOWN                    "text_unknown"
#define TEXT_UNKNOWN_DEF                "Unknown"

//capi 2.0:
#define TEXT_UNKNOWN_ISDN               "text_unknown_isdn"
#define TEXT_UNKNOWN_ISDN_DEF           "Unknown ISDN"

//capi 2.0:
#define TEXT_UNKNOWN_ANALOG             "text_unknown_analog"
#define TEXT_UNKNOWN_ANALOG_DEF         "Unknown analog"

//capi 2.0:
#define OUTGOING_NUMBER                 "outgoing_number"
#define OUTGOING_NUMBER_DEF             ""

#define COM_DEVICE                      "com_device"
#define COM_DEVICE_DEF                  0

#define BEEP_ON_CALLS                   "beep_on_calls"
#define BEEP_ON_CALLS_DEF               0

#define BEEP_ON_CALLS_FREQ              "beep_on_calls_frequency"
#define BEEP_ON_CALLS_FREQ_DEF          4000

#define BEEP_ON_CALLS_DURA              "beep_on_calls_duration"
#define BEEP_ON_CALLS_DURA_DEF          500

#define BEEP_ON_CALLS_DELAY             "beep_on_calls_delay"
#define BEEP_ON_CALLS_DELAY_DEF         5000

#define CAPITEL_PRIORITY                "capitel_priority"
#define CAPITEL_PRIORITY_DEF            2

#define CAPITEL_AFFINITY                "capitel_affinity"
#define CAPITEL_AFFINITY_DEF            1

#define CAPITEL_CTI_SUPPORT             "capitel_cti_support"
#define CAPITEL_CTI_SUPPORT_DEF         0

#define CAPITEL_CTI_PROGRAM             "capitel_cti_program"
#define CAPITEL_CTI_PROGRAM_DEF         ""

#define CAPITEL_CODEC_ULAW              "capitel_codec_ulaw"
#define CAPITEL_CODEC_ULAW_DEF          0

#define DTMF_DOWN_BORDER                "dtmf_down_border"
#define DTMF_DOWN_BORDER_DEF            IS_DTMF_DOWN_DEF

#define DTMF_UP_BORDER                  "dtmf_up_border"
#define DTMF_UP_BORDER_DEF              IS_DTMF_UP_DEF

#define SILENCE_BORDER                  "silence_border"
#define SILENCE_BORDER_DEF              85

#define OUT_CIP_VALUE                   "out_cip_value"
#define OUT_CIP_VALUE_DEF               1

#define CALL_BACK_NUMBER                "call_back_number"
#define CALL_BACK_NUMBER_DEF            ""

#define CALL_BACK_ACTIVE                "call_back_active"
#define CALL_BACK_ACTIVE_DEF            "1"

#define CALL_BACK_LIMIT                 "call_back_limit"
#define CALL_BACK_LIMIT_DEF             2

#define CAPITEL_IS_CALLER_ID            "capitel_is_caller_id"
#define CAPITEL_IS_CALLER_ID_DEF        1

#define MODEM_INIT_STR                  "modem_init_string"
#define MODEM_INIT_STR_DEF              "at&f$ibp=trans$isci=0,1,1,0,0s153=255"

#define MODEM_RING_STR                  "modem_ring_string"
#define MODEM_RING_STR_DEF              "RING"

#define MODEM_CONNECT_STR               "modem_connect_string"
#define MODEM_CONNECT_STR_DEF           "CONNECT "

#define MODEM_SEARCH_FROM_COM           "modem_search_from_com"
#define MODEM_SEARCH_FROM_COM_DEF       1

#define MODEM_SEARCH_TO_COM             "modem_search_to_com"
#define MODEM_SEARCH_TO_COM_DEF         8

#define MODEM_BITRATE                   "modem_bitrate"
#define MODEM_BITRATE_DEF               115200

#define PLAY_BEEP                       "play_beep"
#define PLAY_BEEP_DEF                   1

#define ZERO_BEHAVIOR                   "zero_behavior"
#define ZERO_BEHAVIOR_DEF               0


