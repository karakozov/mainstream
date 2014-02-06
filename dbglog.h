
#ifndef __DBGLOG_H__
#define __DBGLOG_H__

#ifdef FPRINTF
#undef FPRINTF
#endif
#ifndef FPRINTF
#define FPRINTF(S...) fprintf(stderr, S)
#endif

#ifdef dbg_msg
#undef dbg_msg
#endif
#ifndef dbg_msg
#define dbg_msg(flag, S...) do { if(flag) FPRINTF(S); } while(0)
#endif

#endif //__DBGLOG_H__
