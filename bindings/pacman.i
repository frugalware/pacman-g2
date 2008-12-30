#if defined(SWIGPERL)
%module "Pacman::Core"
#else
%module pacman
#endif
%include "cpointer.i"

/* Create casting functions */

%pointer_cast(void *, long, void_to_long);
%pointer_cast(void *, char *, void_to_char);
%pointer_cast(void *, unsigned long, void_to_unsigned_long);
%pointer_cast(char *, unsigned long, char_to_unsigned_long);
%pointer_cast(void *, PM_LIST *, void_to_PM_LIST);
%pointer_cast(void *, PM_PKG *, void_to_PM_PKG);
%pointer_cast(void *, PM_GRP *, void_to_PM_GRP);
%pointer_cast(void *, PM_SYNCPKG *, void_to_PM_SYNCPKG);
%pointer_cast(void *, PM_DB *, void_to_PM_DB);
%pointer_cast(void *, PM_CONFLICT *, void_to_PM_CONFLICT);
%pointer_cast(void *, PM_DEPMISS *, void_to_PM_DEPMISS);

%include "pacman.h"

%inline %{
/* PM_PKG */
PM_PKG** PKGp_new()
{
        PM_PKG **ptr;
        ptr = malloc(sizeof(PM_PKG*));
        return(ptr);
}

void PKGp_free(PM_PKG **ptr)
{
        free(ptr);
}

PM_PKG* PKGp_to_PKG(PM_PKG **ptr)
{
        return(*ptr);
}
/* PM_LIST */
PM_LIST** LISTp_new()
{
        PM_LIST **ptr;
        ptr = malloc(sizeof(PM_LIST*));
        return(ptr);
}

void LISTp_free(PM_LIST **ptr)
{
        free(ptr);
}

PM_LIST* LISTp_to_LIST(PM_LIST **ptr)
{
        return(*ptr);
}
%}
