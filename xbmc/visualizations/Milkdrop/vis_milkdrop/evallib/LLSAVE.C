/*
 * This is linked from lexlib to resolve a global in yylex which
 * will be undefined if the user grammar has not defined any rules
 * with right-context (look-ahead)
 */
char    *llsave[1];             /* Look ahead buffer            */
