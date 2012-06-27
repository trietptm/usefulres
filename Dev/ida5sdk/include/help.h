/*
        ** Help Subsystem **       Created 18-Sep-92 by Guilfanov I.

        Revisions history

  Date          Who                     Description
  ----          ---     -----------------------------------------------
18.09.92        ig      First version is created.
15.03.92        ig      ErOk is added
08.04.93        ig      some doc added, help_t type defined
18.11.93        ig      HelpTerm() function added.
07.07.94        ig      added ivalue_size()
22.08.94        ig      dynamic messages.
26.08.94        ig      HelpPutMessage() creates messages in a separate paragragh
16.11.98        ig      HelpTerm() function prototype is changed

*/

#ifndef _HELP_H
#define _HELP_H
#pragma pack(push, 1)

#define HELPDYN_SET     0x7000  /* Dynamical messages paragragh */

typedef int help_t;     /* Help messages are referred by ints         */

enum {                  /* These error codes don't use help subsystem */
  ErOk = 0,
  ErNoFile = -1,        /* because help subsystem is not initialized  */
  ErBadHelp = -2,       /* yet when they occur                        */
  ErBadVers = -3,
  ErIO = -4,
  ErNoMem = -5,
  ErBadOS = -6
};

/*------------------------------------------------------------------------*/
/* Function:    Initialize help subsystem.
   Input:       defaultpath - where to look for help file if other
                              methods fail.
                helpfile    - help file name
                argc,argv   - from main()
                sw          - language switcher char (for LANG variable).
                              The user can use -<sw>xx switch in the
                              command line to specify the language.
                              This switch will be deleted from argv if it exists.
   Description: Look for the help file in the following directories:
                        $(NLSPATH) - list of directories separated by
                                        : - unix
                                        ; - MS DOS
                                     This list can contain special symbols
                                       %N - help file name
                                       %L - current $LANG variable
                                            (if LANG doesn't exist, "En_US"
                                             is assumed)
                                            -<sw>xx overrides LANG variable.
                        defaultpath - the same format as NLSPATH

   Returns:     error code. Even if this function returns an error, you can
                call it again (with other parameters, of course :-)
   Example:

        code = HelpInit(getenv("FSDIR"),"fs.hlp",&argc,argv,'L');
                -- Find fs.hlp file in the directories denoted by NLSPATH
                   and FSDIR variables, if the command line contains -L##
                   switch, use '##' for the language name. E.g., if the user
                   starts this program using command line:
                        startprg -LRussian
                   the language name will be 'Russian' and the %L sequence in
                   the NLSPATH and FSDIR will be substituted by 'Russian'
*/

int HelpInit(char *defaultpath, char *helpfile, int *argc, char *argv[], char sw);

idaman char *ida_export ivalue(help_t mes, char *buf, size_t bufsize);
idaman char *ida_export qivalue(help_t mes);      // Return answer in dynamic memory
char *GetMessage(help_t para, help_t mes);        // Return answer in dynamic memory

#ifdef __cplusplus

/*      This class is used for temporary message:               */
/*              the message is kept until it goes out of scope. */
/*      simply use: itext(n)                                    */

class itext
{
  char *ptr;
public:
  itext(help_t mes) { ptr = qivalue(mes); }
  ~itext(void)      { qfree(ptr); }
  operator char*()  { return ptr; }
};

#endif /* __cplusplus */

#pragma pack(pop)
#endif /* _HELP_H */
