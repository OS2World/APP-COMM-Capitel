#define CAPI_NUM_B3_CON      1
#define CAPI_NUM_B3_BLK      7
#define CAPI_NUM_B3_SIZE  2048

/* Nur CAPI 1.1: */
#define CAPI_NUM_MSGS       16

/* Nur CAPI 2.0: */
#define CAPI_MSGBUF_SIZE    (1024+(1024*CAPI_NUM_B3_CON)+8192)

typedef struct st_comm_connect_ind
{
  char caller_name    [200];
  char caller_org_name[200];
  char called_name    [200];
  char is_digital;
} AnsConIndMsg;

short comm_init                 (void (*)(short, void *));
short comm_exit                 (void);
short comm_listen               (short);
char  comm_connected            (void);
char  comm_data_available       (void);
void  comm_clear_all_buffer     (void);
//void  comm_clear_read_buffer    (void);
//void  comm_clear_write_buffer   (void);
long  comm_read_block           (char *buff);
void  comm_write_block          (char *, short);
//char  comm_can_write            (void);
void  comm_begin_of_record      (void);
void  comm_disc_req             (void);
void  comm_name_of_interface    (char *);

short comm11_init               (void (*)(short, void *), char);
short comm11_exit               (void);
short comm11_listen             (short);
char  comm11_connected          (void);
char  comm11_data_available     (void);
void  comm11_clear_all_buffer   (void);
//void  comm11_clear_read_buffer  (void);
//void  comm11_clear_write_buffer (void);
long  comm11_read_block         (char *buff);
void  comm11_write_block        (char *, short);
//char  comm11_can_write          (void);
void  comm11_begin_of_record    (void);
void  comm11_disc_req           (void);
void  comm11_name_of_interface  (char *);

short comm20_init               (void (*)(short, void *), char);
short comm20_exit               (void);
short comm20_listen             (short);
char  comm20_connected          (void);
char  comm20_data_available     (void);
void  comm20_clear_all_buffer   (void);
//void  comm20_clear_read_buffer  (void);
//void  comm20_clear_write_buffer (void);
long  comm20_read_block         (char *buff);
void  comm20_write_block        (char *, short);
//char  comm20_can_write          (void);
void  comm20_begin_of_record    (void);
void  comm20_disc_req           (void);
void  comm20_name_of_interface  (char *);

short commse_init               (void (*)(short, void *), char);
short commse_exit               (void);
short commse_listen             (short);
char  commse_connected          (void);
char  commse_data_available     (void);
void  commse_clear_all_buffer   (void);
//void  commse_clear_read_buffer  (void);
//void  commse_clear_write_buffer (void);
long  commse_read_block         (char *buff);
void  commse_write_block        (char *, short);
//char  commse_can_write          (void);
void  commse_begin_of_record    (void);
void  commse_disc_req           (void);
void  commse_name_of_interface  (char *);


