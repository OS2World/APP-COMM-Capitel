short answer_init                (void (*)(short, void *), short, short); // zweiter paramter: 0 = nerv_message OFF, 1 = nerv_message ON.
void  answer_exit                (void);
void  answer_record_welcome      (char *);
//short answer_version_expired     (void);
short answer_cannot_close        (void);
void  answer_listen              (void);
void  answer_wav2alw_convert_all (void);
void  answer_name_of_interface   (char *);
void  answer_stop_bell           (void);
void  answer_play_all            (void);
void  answer_cti                 (char *);

typedef struct {
  char called_name    [200];
  char caller_org_name[200];
  char caller_name    [200];
  char is_digital;
} TCapiInfo;


extern char* beepdata;

