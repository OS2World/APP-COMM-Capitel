typedef struct _ItemInfo
{
  HPOINTER icon;
  CHAR     caller[255],
           date[10],
           time[10],
           calledEAZ[255],
           filename[255];
  ULONG    seconds;
  CHAR     digital;

  PCHAR    pcaller, pdate, ptime, pfilename, pcalledEAZ;

} ITEMINFO;

typedef struct _ContainerRecord
{
  MINIRECORDCORE core;
  ITEMINFO itemInfo;
} CntrRec;

short InitContainer( void );
short AddItem( HPOINTER, char *, char *, char *, ULONG, char *, char *, char );
void EmptyContainer( void );

