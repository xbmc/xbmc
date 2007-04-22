#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

# ifndef YYSTYPE
#  define YYSTYPE int
#  define YYSTYPE_IS_TRIVIAL 1
# endif
# define	BREAK	257
# define	CASE	258
# define	CATCH	259
# define	CONTINUE	260
# define	DEFAULT	261
# define	DELETE	262
# define	DO	263
# define	ELSE	264
# define	FINALLY	265
# define	FOR	266
# define	FUNCTION	267
# define	IF	268
# define	IN	269
# define	INSTANCEOF	270
# define	NEW	271
# define	RETURN	272
# define	SWITCH	273
# define	THIS	274
# define	THROW	275
# define	TYPEOF	276
# define	TRY	277
# define	VAR	278
# define	VOID	279
# define	WHILE	280
# define	WITH	281
# define	LEXERROR	282
# define	THREERIGHTEQUAL	283
# define	IDENTIFIER	284
# define	NULLLIT	285
# define	FALSELIT	286
# define	TRUELIT	287
# define	NUMLIT	288
# define	STRINGLIT	289
# define	BUGGY_TOKEN	290
# define	PLUSPLUS	11051
# define	MINMIN	11565
# define	SHLEQ	15676
# define	SHREQ	15678
# define	SHLSHL	15420
# define	SHRSHR	15934
# define	SHRSHRSHR	291
# define	EQEQ	15677
# define	EXCLAMEQ	15649
# define	EQEQEQ	292
# define	EXCLAMEQEQ	293
# define	ANDAND	9766
# define	OROR	31868
# define	PLUSEQ	15659
# define	MINEQ	15661
# define	TIMESEQ	15658
# define	MODEQ	15653
# define	DIVEQ	15663
# define	ANDEQ	15654
# define	OREQ	15740
# define	XOREQ	15710
# define	SHLSHLEQ	294
# define	SHRSHREQ	295


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
