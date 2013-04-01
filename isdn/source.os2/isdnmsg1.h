typedef struct st_capi_msg_hdr
{
  short length;
  short appid;
  char  cmd;
  char  subcmd;
  short  msgnum;
} CapiMsgHeader;

typedef struct st_listen_req
{
  CapiMsgHeader header;

  char controller;
  long info_mask;
  short serviced_eaz;
  short serviced_si_mask;
} TListenReqMsg;

typedef struct st_listen_conf
{
  CapiMsgHeader header;

  char controller;
  short info;
} TListenConfMsg;

typedef struct st_connect_ind
{
  CapiMsgHeader header;

  short plci;
  char controller;
  char req_service;
  char req_service_add;
  char req_eaz;

  char caller_addr_len;
  char caller_addr_type;
  char caller_addr;

} TConnectIndMsg;

typedef struct st_connect_resp
{
  CapiMsgHeader header;

  short plci;
  char reject;
} TConnectRespMsg;

typedef struct st_select_b2_prot_req
{
  CapiMsgHeader header;

  short plci;
  char  b2_prot;

  struct st_dlpd
  {
    char dlpd_length;
    short data_length;
    char link_a;
    char link_b;
    char modulo_mode;
    char win_size;
    char xid;
  } dlpd;

} TSelB2ProtReq;

typedef struct st_select_b2_prot_conf
{
  CapiMsgHeader header;

  short plci;
  short info;
} TSelB2ProtConf;

typedef struct st_select_b3_prot_req
{
  CapiMsgHeader header;

  short plci;
  char  b3_prot;

  struct st_ncpd
  {
    char  length;
    short lic;
    short hic;
    short ltc;
    short htc;
    short loc;
    short hoc;
    char  modulo_mode;
  } ncpd;

} TSelB3ProtReq;

typedef struct st_select_b3_prot_conf
{
  CapiMsgHeader header;

  short plci;
  short info;
} TSelB3ProtConf;

typedef struct st_listen_b3_req
{
  CapiMsgHeader header;

  short plci;
} TListenB3Req;

typedef struct st_listen_b3_conf
{
  CapiMsgHeader header;

  short plci;
  short info;
} TListenB3Conf;

typedef struct st_conn_act_ind
{
  CapiMsgHeader header;

  short plci;

  struct st_address
  {
    char length;
    char number[36];
  } address;

} TConnActInd;

typedef struct st_conn_act_resp
{
  CapiMsgHeader header;

  short plci;
} TConnActResp;

typedef struct st_ncpi
{
  char length;
  char data[14];
} TNCPI;

typedef struct st_conn_b3_ind
{
  CapiMsgHeader header;

  short ncci;
  short plci;
  TNCPI ncpi;
} TConnB3Ind;

typedef struct st_conn_b3_resp
{
  CapiMsgHeader header;

  short ncci;
  char  reject;
  TNCPI ncpi;
} TConnB3Resp;

typedef struct st_conn_b3_act_ind
{
  CapiMsgHeader header;

  short ncci;
  TNCPI ncpi;
} TConnB3ActInd;

typedef struct st_conn_b3_act_resp
{
  CapiMsgHeader header;

  short ncci;
} TConnB3ActResp;

typedef struct st_data_b3_ind
{
  CapiMsgHeader header;

  short ncci;
  short data_length;
  char  * _Seg16 data;
  char  number;
  short flags;
} TDataB3Ind;

typedef struct st_data_b3_resp
{
  CapiMsgHeader header;

  short ncci;
  char number;
} TDataB3Resp;

typedef struct st_disc_b3_ind
{
  CapiMsgHeader header;

  short ncci;
  short info;
  TNCPI ncpi;
} TDiscB3Ind;

typedef struct st_disc_b3_resp
{
  CapiMsgHeader header;

  short ncci;
} TDiscB3Resp;

typedef struct st_disc_req
{
  CapiMsgHeader header;

  short plci;
  char cause;
} TDiscReq;

typedef struct st_disc_conf
{
  CapiMsgHeader header;

  short plci;
  short info;
} TDiscConf;

typedef struct st_disc_ind
{
  CapiMsgHeader header;

  short plci;
  short info;
} TDiscInd;

typedef struct st_disc_resp
{
  CapiMsgHeader header;

  short plci;
} TDiscResp;

typedef struct st_data_b3_req
{
  CapiMsgHeader header;

  short ncci;
  short data_length;
  char  * _Seg16 data;
  char  number;
  short flags;
} TDataB3Req;

typedef struct st_data_b3_conf
{
  CapiMsgHeader header;

  short ncci;
  char  number;
  short info;
} TDataB3Conf;

