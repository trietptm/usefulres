        IDA SDK - Interactive Disassembler Module SDK
        =============================================

        This SDK should be used with IDA kernel version 5.0

        This package allows you to write:
                - processor modules
                - input file loader modules
                - plugin modules (including the
                   processor module extension plugins)

        Please read read through whole file before continuing!

-----------------------------------------------

        What you need:

To create 32bit or 64bit Win32 modules:    Borland C++ Builder v4.0
                                        or free BCC v5.5
                                        or Visual C++ >= v6.0
                                        or GNU C++ compiler

To create 32bit or 64bit Linux modules:    GNU C++ compiler

The Visual C++ users should refer to install_visual.txt for the explanations
how to install.

The CBuilder visual environment users should refer to install_cb.txt for the
explanations how to install (only for the development of plugins using the
visual components).

For installation under Linux, please refer to install_linux.txt

All others should refer to install_make.txt.

-----------------------------------------------

        A quick tour on header files:


AREA     HPP            class 'area'. This class is a base class for
                        'segment_t' and 'srarea_t' (segment register) classes.
                        This class keeps information about various areas
                        of the disassembled file.

AUTO     HPP            auto-analysis related functions

BYTES    HPP            Functions and definitions which describe each byte
                        of the disassembled program: is it an instruction,
                        data, operand types etc.

DBG      HPP            Debugger API for debugger users

DISKIO   HPP            file i/o functions
                        See file pro.h and fpro.h for additional system functions

ENTRY    HPP            List of entry points to the program being
                        disassembled.

ENUM     HPP            Enumeration types in the disassembled program

EXPR     HPP            IDC language functions.

FIXUP    HPP            information about relocation table of the program.

FPRO     H              Alternative set of system-indenendent file i/o
                        functions. These functions do check errors but never
                        exit even if an error occurs. They return extended
                        error code in qerrno variable.
                        You must use these functions, not functions from
                        stdio.h

FRAME    HPP            Local variables, stack trace

FUNCS    HPP            Functions in the disassembled program

HELP     H              Help subsystem. This subsystem is not used in
                        IDP files. I put it just in case.

IDA      HPP            the 'main' header file of IDA project.
                        This file should be included in all source files.
                        In this file the 'inf' structure is
                        defined: it keeps all parameters of the disassembled
                        file.

IDD      HPP            Debugger plugin API for debugger module writers

IDP      HPP            the 'main' header file of IDP modules.
                        2 structures are described here:
                          processor_t - description of processor
                          asm_t       - description of assembler
                        Each IDP has one processor_t and several asm_t structures

IEEE     H              IEEE floating point functions

INTEL    HPP            header file from the ibm pc module.
                        for information only, it will not compile
                        because it contains references to internal files!

INTS     HPP            predefined comments

KERNWIN  HPP            various functions to interact with the user.
                        Also, some functions to process strings are kept in
                        this header.

LINES    HPP            generation of source (assembler) lines and long
                        comment lines. variables controlling the exact time
                        and place to generate xrefs, indented comments etc.
                        shouldn't be used in simple IDP modules.
                        You must use these function instead of functions
                        from stdlib.h


NALT     HPP            some predefined netnode array indexes used by the
                        kernel. these functions should not be used directly
                        since they are very low level.

NAME     HPP            names: rename, unname bytes etc.

NETNODE  HPP            the lowest level of access to the database. Modules
                        can use this level to keep some private inforation
                        in the database. Here is a short description of
                        the concept:
                          the database consists of 'netnodes'.
                          The netnodes are numbered by 32-bit integers
                          and may have:
                            - a name (max length is MAXNAMESIZE-1)
                            - a value (a string)
                            - sparse arrays of values:
                              Each sparse array has a 8-bit tag. Therefore,
                              we can have 256 sparse arrays in one netnode.
                              Only non-zero elements of the arrays are stored in
                              the database. Arrays are indexed by 32-bit or 8-bit
                              indexes. You can keep any type of information in
                              an array element. The size of an element is limited
                              by MAXSPECSIZE. For example, you could have an
                              array of addresses that have been patched by the user:

                              <address> : <old_value_of_byte>

                              The array is empty at the start and will
                              grow as the user patches the input file.

                              There are 2 predefined arrays:

                                - strings       (supval)
                                - longs         (altval)

                              The arrays don't need to be declared or created
                              specially. They implicitly exist at each node.
                              To save something into an array simply write
                              to the array element (altset or supset functions)
                        There are no limitations on the size or number of
                        netnode arrays.
OFFSET   HPP            functions that work with offsets.

PRO      H              compiler related stuff and some system-independent functions

QUEUE    HPP            queue of problems.

SEGMENT  HPP            program segmentation
SRAREA   HPP            segment registers. If your processor doesn't have
                        segment registers, you don't need this file.
STRUCT   HPP            Structure types in the disassembled program

TYPEINF  HPP            Type information in the program

UA       HPP            This header file describes insn_t structure called
                        cmd: this structure keeps a disassembled instruction
                        in the internal form. Also, you will find here
                        helper functions to create output lines etc.
VA       HPP            Virtual array. Used by other parts of IDA.
                        IDP module don't use it directly.
VM       HPP            Virtual memory. Used by other parts of IDA.
                        IDP module don't use it directly.
XREF     HPP            cross-references.


All functions usable in the modules are marked by the "ida_export" keyword.
There are some exported functions that should be not used except very cautiously
like setFlags() and many functions from nalt.hpp, for example.
In general, try to find a high-level counterpart of the function in these cases.

-----------------------------------------------

        Libraries

IDA      LIB    - import library with all functions exported from the kernel

        There are several different versions of this file, one for each platform.

-----------------------------------------------

        Description of the processor module example

     The module disassembles an instruction in several steps:
        - analysis (decoding)           file ana.cpp
        - emulation                     file amu.cpp
        - output                        file out.cpp

     The analyser (ana.cpp) should be fast and simple: just decode an
     instruction and fill the 'cmd' structure. The analyser will always be called
     before calling emulator and output functions. If the current address
     can't contain an instruction, it should return 0. Otherwise, it returns
     the length of the instruction in bytes.

     The emulator and outputter should use the contents of the 'cmd' array.
     The emulator performs the following tasks:
        - creates cross-references
        - plans to disassemble subsequent instructions
        - create stack variables (optional)
        - tries to keep track of register contents (optional)
        - provides better analysis method for the kernel (optional)
        - etc

     The outputter produces a line (or lines) that will be displayed on
     the screen.
     It generates only essential part of the line: line prefix, comments,
     cross-references will be generated by the kernel itself.
     To generate a line, MakeLine() or printf_line() should be used.

MAKEFILE        - makefile for a processor module
                  The DESCRIPTION line
                  should contain names of processors handled by this IDP
                  module, separated by colons. The first name is description
                  of whole module (not a processor name).
STUB            - MSDOS stub for the module
ANA      CPP    - analysis of an instruction: fills the cmd structure.
EMU      CPP    - emulation: creates xrefs, plans to analyse subsequent
                  instructions
INS      CPP    - table of instructions.
OUT      CPP    - generate source lines.
REG      CPP    - description of processor, assemblers, and notify() function.
                  This function is called when certain events occur. You
                  may want to have some additional processing of those events.
IDP      DEF    - the module description for the linker.
I51      HPP    - local header file. you may have another header file for
                  you module.
INS      HPP    - list of instructions.

-----------------------------------------------

        And finally:

  I recommend to study the samples, compile and run them.

  Limitations on the modules:
        - for the dynamic memory allocation: please use qalloc/qfree()
          while you are free to use any other means, these functions
          are provided by the kernel and everything allocated by the
          kernel should be deleted using qfree()
        - for the file i/o: never use functions from stdio.h.
          Use functions from fpro.h instead.
          If you still want to use the standard functions, never pass
          FILE* pointer obtained from the standard functions to the kernel
          and vice versa.
        - the exported descriptor names are fixed:
                processor module        LPH
                loader module           LDSC
                plugin module           PLUGIN

  Usually I write a new processor module in the following way:
        - copy the sample module files to a new directory
        - first I edit INS.CPP and INS.HPP files
        - write the analyser ana.cpp
        - then outputter
        - and emulator (you can start with an almost empty emulator)
        - and describe the processor & assembler, write the notify() function

  Naturally, it is easier to copy and to modify example files than to write
  your own files from the scratch.

  Debugging:
  You can use the following debug print functions:
        deb() - display a line in the messages window if -z command
                line switch is specified. You may use debug one of:
                IDA_DEBUG_IDP, IDA_DEBUG_LDR, IDA_DEBUG_PLUGIN
        msg() - display a line in the messages window
        warning() - display a dialog box with the message

  To stop in the debugger when the module is loaded, you may use the
  __emit__(0xCC) construct in the module initialization code.
  (__asm _emit 0xCC; for Visual C++)

  BTW, you can save all lines appearing in the messages window to a file.
  Just set an enviroment variable:

        set IDALOG=ida.log

  I always have this variable set, it is great help in the development.

  The SDK support is not included in the IDA Pro purchase but
  you can subscribe for the extended SDK support:

        http://www.datarescue.com/idabase/idaorder.htm

-----------------------------------------------------
Information on the compilers
-----------------------------------------------------

Borland:
--------
BCC 5.5 (free)
  download from www.borland.com/bcppbuilder/freecompiler/

Borland C++ Builder v4.0, v5.0 (commercial)


Microsoft:
----------
.NET framework SDK (free, for CL.EXE, LINK.EXE)
  + .NET framework (free, only to install .NET framework SDK)
  + Platform SDK (free, for NMAKE.EXE):
      Core SDK - build environment (for NMAKE.EXE and C headers)
  downloads at:
    www.microsoft.com/italy/msdn/download/frameworksdk.asp
    www.microsoft.com/msdownload/platformsdk/sdkupdate/

Visual C++ v6.0 (commercial)


GNU:
----
MinGW (free) + MSYS (free, needed for GNU MAKE)
  download at http://www.mingw.org/

CYGWIN (free)
  download at http://www.cygwin.com/


