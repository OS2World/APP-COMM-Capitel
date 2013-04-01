
MRESULT EXPENTRY configProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY addNewCallerProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

typedef struct _EAZItemInfo
{
  HPOINTER icon;
  CHAR     port[512],
           desc[512],
           alwfile[512],
           reject_cause[512],
           maxrectime[512],
           answerdelay[512],
           ringingwave[512],
           notifynumber[512];
  PCHAR    pport, pdesc, palwfile, preject, pmaxrectime, panswerdelay,
           pringingwave, pnotifynumber;
} EAZITEMINFO;

typedef struct _EAZSetup
{
  MINIRECORDCORE core;
  EAZITEMINFO itemInfo;
} EAZSetup;

typedef struct _PeopleItemInfo
{
  HPOINTER icon;
  CHAR     name[512],
           number[512],
           alwfile[512],
           reject_cause[512],
           maxrectime[512],
           answerdelay[512],
           ringingwave[512],
           notifynumber[512];
  PCHAR    pnumber, pname, palwfile, preject, pmaxrectime, panswerdelay,
           pringingwave, pnotifynumber;
} PEOPLEITEMINFO;

typedef struct _PeopleSetup
{
  MINIRECORDCORE core;
  PEOPLEITEMINFO itemInfo;
} PeopleSetup;

typedef struct _DtmfItemInfo
{
  HPOINTER icon;
  CHAR     code[512],
           action[512],
           params[512],
           title[512],
           type[512],
           active[512];
  PCHAR    pcode, paction, pparams, ptitle, ptype, pactive;
} DTMFITEMINFO;

typedef struct _DtmfSetup
{
  MINIRECORDCORE core;
  DTMFITEMINFO itemInfo;
} DtmfSetup;

