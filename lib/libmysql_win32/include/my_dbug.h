/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef _dbug_h
#define _dbug_h

#if defined(__cplusplus) && !defined(DBUG_OFF)
class Dbug_violation_helper
{
public:
  inline Dbug_violation_helper() :
    _entered(TRUE)
  { }

  inline ~Dbug_violation_helper()
  {
    assert(!_entered);
  }

  inline void leave()
  {
    _entered= FALSE;
  }

private:
  bool _entered;
};
#endif /* C++ */

#ifdef	__cplusplus
extern "C" {
#endif
#if !defined(DBUG_OFF) && !defined(_lint)
struct _db_code_state_;
extern	int _db_keyword_(struct _db_code_state_ *cs, const char *keyword);
extern  int _db_strict_keyword_(const char *keyword);
extern  int _db_explain_(struct _db_code_state_ *cs, char *buf, size_t len);
extern  int _db_explain_init_(char *buf, size_t len);
extern	void _db_setjmp_(void);
extern	void _db_longjmp_(void);
extern  void _db_process_(const char *name);
extern	void _db_push_(const char *control);
extern	void _db_pop_(void);
extern  void _db_set_(struct _db_code_state_ *cs, const char *control);
extern  void _db_set_init_(const char *control);
extern	void _db_enter_(const char *_func_,const char *_file_,uint _line_,
			const char **_sfunc_,const char **_sfile_,
			uint *_slevel_, char ***);
extern	void _db_return_(uint _line_,const char **_sfunc_,const char **_sfile_,
			 uint *_slevel_);
extern	void _db_pargs_(uint _line_,const char *keyword);
extern	void _db_doprnt_ _VARARGS((const char *format,...))
  ATTRIBUTE_FORMAT(printf, 1, 2);
extern	void _db_dump_(uint _line_,const char *keyword,
                       const unsigned char *memory, size_t length);
extern	void _db_end_(void);
extern	void _db_lock_file_(void);
extern	void _db_unlock_file_(void);
extern FILE *_db_fp_(void);

#ifdef __cplusplus

#define DBUG_ENTER(a) \
        const char *_db_func_, *_db_file_; \
        uint _db_level_; \
        char **_db_framep_; \
        Dbug_violation_helper dbug_violation_helper; \
        _db_enter_ (a, __FILE__, __LINE__, &_db_func_, &_db_file_, \
                    &_db_level_, &_db_framep_)
#define DBUG_VIOLATION_HELPER_LEAVE dbug_violation_helper.leave()

#else /* C */

#define DBUG_ENTER(a) \
        const char *_db_func_, *_db_file_; \
        uint _db_level_; \
        char **_db_framep_; \
        _db_enter_ (a, __FILE__, __LINE__, &_db_func_, &_db_file_, \
                    &_db_level_, &_db_framep_)
#define DBUG_VIOLATION_HELPER_LEAVE do { } while(0)

#endif /* C++ */

#define DBUG_LEAVE \
        DBUG_VIOLATION_HELPER_LEAVE; \
	_db_return_ (__LINE__, &_db_func_, &_db_file_, &_db_level_)
#define DBUG_RETURN(a1) do {DBUG_LEAVE; return(a1);} while(0)
#define DBUG_VOID_RETURN do {DBUG_LEAVE; return;} while(0)
#define DBUG_EXECUTE(keyword,a1) \
        do {if (_db_keyword_(0, (keyword))) { a1 }} while(0)
#define DBUG_EXECUTE_IF(keyword,a1) \
        do {if (_db_strict_keyword_ (keyword)) { a1 } } while(0)
#define DBUG_EVALUATE(keyword,a1,a2) \
        (_db_keyword_(0,(keyword)) ? (a1) : (a2))
#define DBUG_EVALUATE_IF(keyword,a1,a2) \
        (_db_strict_keyword_((keyword)) ? (a1) : (a2))
#define DBUG_PRINT(keyword,arglist) \
        do {_db_pargs_(__LINE__,keyword); _db_doprnt_ arglist;} while(0)
#define DBUG_PUSH(a1) _db_push_ (a1)
#define DBUG_POP() _db_pop_ ()
#define DBUG_SET(a1) _db_set_ (0, (a1))
#define DBUG_SET_INITIAL(a1) _db_set_init_ (a1)
#define DBUG_PROCESS(a1) _db_process_(a1)
#define DBUG_FILE _db_fp_()
#define DBUG_SETJMP(a1) (_db_setjmp_ (), setjmp (a1))
#define DBUG_LONGJMP(a1,a2) (_db_longjmp_ (), longjmp (a1, a2))
#define DBUG_DUMP(keyword,a1,a2) _db_dump_(__LINE__,keyword,a1,a2)
#define DBUG_END()  _db_end_ ()
#define DBUG_LOCK_FILE _db_lock_file_()
#define DBUG_UNLOCK_FILE _db_unlock_file_()
#define DBUG_ASSERT(A) assert(A)
#define DBUG_EXPLAIN(buf,len) _db_explain_(0, (buf),(len))
#define DBUG_EXPLAIN_INITIAL(buf,len) _db_explain_init_((buf),(len))
#define IF_DBUG(A) A
#else						/* No debugger */

#define DBUG_ENTER(a1)
#define DBUG_LEAVE
#define DBUG_VIOLATION_HELPER_LEAVE
#define DBUG_RETURN(a1)             do { return(a1); } while(0)
#define DBUG_VOID_RETURN            do { return; } while(0)
#define DBUG_EXECUTE(keyword,a1)    do { } while(0)
#define DBUG_EXECUTE_IF(keyword,a1) do { } while(0)
#define DBUG_EVALUATE(keyword,a1,a2) (a2)
#define DBUG_EVALUATE_IF(keyword,a1,a2) (a2)
#define DBUG_PRINT(keyword,arglist) do { } while(0)
#define DBUG_PUSH(a1)
#define DBUG_SET(a1)                do { } while(0)
#define DBUG_SET_INITIAL(a1)        do { } while(0)
#define DBUG_POP()
#define DBUG_PROCESS(a1)
#define DBUG_SETJMP(a1) setjmp(a1)
#define DBUG_LONGJMP(a1) longjmp(a1)
#define DBUG_DUMP(keyword,a1,a2)    do { } while(0)
#define DBUG_END()
#define DBUG_ASSERT(A)              do { } while(0)
#define DBUG_LOCK_FILE
#define DBUG_FILE (stderr)
#define DBUG_UNLOCK_FILE
#define DBUG_EXPLAIN(buf,len)
#define DBUG_EXPLAIN_INITIAL(buf,len)
#define IF_DBUG(A)
#endif
#ifdef	__cplusplus
}
#endif
#endif
