ExamDiff Pro(TM) Version 4.5
ExamDiff Pro Copyright 1997-2008 PrestoSoft LLC(TM) ALL RIGHTS RESERVED.

ExamDiff Pro is a powerful yet intuitive and easy to use visual file and 
directory comparison tool for Windows 98/Me/NT/2000/XP/2003/Vista. If 
you've been frustrated with other comparison utilities, you will find that
ExamDiff Pro offers a much more efficient and user-friendly way to compare
files and folders.

ExamDiff Pro features a double-pane view that allows for side-by-side 
comparisons, with customizable color-coding indicating whether each line is
added, deleted, or changed. ExamDiff Pro compares files and directories 
with equal ease, and boasts one of the fastest comparison algorithms of any
tool today.

ExamDiff Pro offers such advanced features as full Unicode support, word
wrapping of long lines, automatic detection of changes in files or folders,
drag and drop support, and the ability to preprocess compared files or
folders via user-defined plug-ins. The program can also create “snapshots”
of directories that can be compared later. Other features of ExamDiff Pro
include regular expression filters, manual synchronization, named sessions,
and comprehensive Ignore options.

===========================================================================
1.  KEY FEATURES
2.  DOWNLOAD
3.  BUG REPORTS
4.  ORDERING INFORMATION
5.  PRICING
6.  EXAMDIFF PRO (TM) ORDER FORM
7.  END-USER LICENSE AGREEMENT
===========================================================================


---------------------------------------------------------------------------
1. KEY FEATURES
---------------------------------------------------------------------------

- Compares text files, binary files, and directories.

- Highlights differences down to the level of lines, words or characters.

- Customizable syntax highlighting for document types, such as C++, Java, 
  XML, and much more.

- Allows editing files within file comparison panes.

- Prints and print previews diff reports.

- Allows to add manual synchronization points to text file comparison.

- Extensive drag and drop support.

- Includes full Unicode support.

- Allows to ignore whitespace, case, lines and line parts matching 
  a regular expression, multiple column ranges, programming language 
  comments and more.

- Allows creating directory snapshots for later comparison.

- Performs synchronous and simple word wrapping of long lines.

- Remembers a user-specified number of last compared first and 
  second files or directories.

- There is no need to specify both filenames -- just enter a directory
  name for one of the files to be compared. ExamDiff will attempt to use
  an entered filename with a previously specified directory name for the 
  second file.

- Automatically detects file or directory changes and prompts the user 
  to re-compare.

- One push re-compare function attempts to leave the viewer's focus in 
  the same place as before the re-compare.

- Drag and drop support for dropping one or two files or directories into 
  the program's window (e.g. from Windows Explorer) for comparison.

- Allows file or directory comparison from the directory comparison panes 
  with a simple double-click on one of the files or directories to be 
  compared.

- Allows the copying, renaming, and deleting files or directories from 
  the directory comparison panes via menu, toolbar buttons, or a right 
  button pop up. Any of these operations is followed by automatic 
  re-synchronization of the compared directories. You can also sort 
  directories in the left or the right pane by name, size, type, or last 
  modification time without losing synchronization.

- Easy editing of files. ExamDiff Pro will spawn any editor (configurable 
  by the user) with the file name and caret position options.

- Saves the differences in text (standard UNIX DIFF file for file 
  comparison) or HTML file.

- Easy navigation through the differences via "Next Difference"/
  "Current Difference"/"Previous Difference" buttons and hot keys or via 
  a list box of all the differences.

- Allows the copying of text from the file comparison panes via drag and 
  drop, a hot key, or a right button pop up.

- Simple "Search" command to search for strings in the comparison panes. 
  It also remembers a user specified number of most recent searches. 

- Customizable colors.

- Fully customizable file name, extension, and/or path filter. For 
  example, the user can choose only .c and .cpp files or any other files 
  he/she chooses to be compared.
  
- Support for name filter sets, and preinstalled filter sets for C++,
  Delphi, XML, and much more.
  
- Customizable date and size filter for directory comparison.

- Tooltips which include file or directory properties (when the mouse 
  cursor is placed over the pane title bars), difference number (when 
  the cursor is over the yellow triangle marking the current 
  difference), and etc.

- Adjustable pane splitter with smooth synchronized scrolling.

- Allows easy toggling between horizontal and vertical splitter 
  orientation as well as splitter centering and hiding of panes.

- Command line support:

    Usage: ExamDiff [Name1] [Name2] [Options]
           or
           ExamDiff /se:Session [Name1] [Name2]
           or
           ExamDiff /sf:SessionFile
      where:
       Name1, Name2 are names of files or directories to be compared
       Session is a name of the session to be used for comparison
       Options are any of the following:
         /i           ignore case
         /w           ignore all white space in lines
         /b           ignore changes in amount of white space in lines
         /l           ignore leading white space in lines
         /e           ignore trailing white space in lines
         /k           ignore blank lines
         /j:RE        ignore lines matching regular expression RE
         /br          briefly report whether files/directories differ
         /t           treat both files as text files
         /tb          treat both files as binary files
         /d           show differences only
         /q:M         set tab size to M characters
         /s           consider files with same size and timestamp identical
         /ic          consider files with same CRC identical
         /y           consider files with different sizes different
                      (even if some ignore options are selected)
         /u           consider files with different attributes different
         /z           consider files with different timestamps different
         /dc          consider files with different CRC values different
         /dv          consider files with different versions different
         /r0          do not compare subdirectories at all
         /r1          compare subdirectories only to determine their status
         /r2          compare subdirectories recursively
         /fi:filter   specify include filter for directory comparison
         /fx:filter   specify exclude filter for directory comparison
         /f           report file differences during directory comparison
                      (used with /o option and with full file comparison,
                      i.e. without any "consider" options)
         /n           don't show initial "Compare" dialog
         /sd          always show initial "Compare" dialog, even if
                      two filenames are used
         /a:N         scroll through all differences with N second delay
                      and exit after the last difference
         /o:file      output diff results into a file 'file' (or to
                      standard output if '-' is specified) and exit
         /no          do not generate output file if no diffs are found
         /append      append to existing output file (used with /o option; 
                      default -- replace existing output file)
         /html        output HTML diff report (used with /o option; default --
                      text diff format; does not work with standard output and/or
                      with /f or /append options)          
         /p[:printer] print diff report to default or specified printer and exit
         /c           print file diffs when printing directory diff report
         /x           read-only mode
         /g:file      load ExamDiff Pro options from an options file
         /ff          force initial file comparison dialog
         /fd          force initial directory comparison dialog
         /swap        swap first and second file/directory
         /sn1:file    use this name when you save the first file
         /sn2:file    use this name when you save the second file
         /nh          do not add files or directories to comparison history
         /?,/h        print this screen

       Options may be entered before of after names of files or directories.
       Use // to specify that no more options will be entered in command line.

       If only /se:Session is specified, session's file/directory names and
       options will be used, even if some other options are also specified
       (they will be ignored.)
       If /se:Session option is specified along with file/directory names,
       specified names will be used, along with the options from the session
       (even if other options are specified in command line).
       If no session is specified, the unnamed default session will be used.

       If /sf:SessionFile is specified, the session file's file/directory names
       and options will be used, even if some other options are also specified
       (they will be ignored.)

       Options /i, /w, /b, /l, /e, /k, /j, /br, /d, /s, /y, /u, /z, /dc,
       /dv and /c can be used with "!", in which case the option will be
       negated (e.g. /!i means do not ignore case).
       If options /i, /w, /b, /l, /e, /k, /j, /br, /d,  /s, /y, /u, /z, /dc,
       /dv, /c (or their negations), /t, tb, /r0, /r1, /r2, /fi, or /fx are
       not set, the last used options in the specified or default session
       remain in effect.

       Command line options are in effect only for the duration of the current
       comparison. If the Options Dialog is started, matching command line
       options will used to initialize the dialog.

---------------------------------------------------------------------------
2. DOWNLOAD
---------------------------------------------------------------------------

The latest version of ExamDiff Pro can always be downloaded from 
http://www.prestosoft.com/ps.asp?page=edp_examdiffpro.

Freeware version of ExamDiff Pro is called ExamDiff. It doesn't have all 
the features of ExamDiff Pro, such as directory comparison. ExamDiff can 
be downloaded from http://www.prestosoft.com/ps.asp?page=edp_examdiff.

ExamDiff is a part of a suite of products that includes FtpVC a tool for 
FTP based Version Control System. It allows joint software development 
remotely over the Internet. For the latest version of FtpVC please visit 
http://www.prestosoft.com/ps.asp?page=fvc_ftpvc.


---------------------------------------------------------------------------
3. BUG REPORTS
---------------------------------------------------------------------------

Please send bug reports, new feature requests and comments to:

examdiffpro@prestosoft.com

or

PrestoSoft LLC
P.O. Box 3611
Thousand Oaks, CA 91359

Before sending your report please visit ExamDiff Pro web page at
http://www.prestosoft.com/ps.asp?page=edp_examdiffpro 
in order to get the latest version or to read about reported 
problems.


---------------------------------------------------------------------------
4. ORDERING INFORMATION
---------------------------------------------------------------------------

This program is not free, but is copyrighted software that is provided to 
allow the user to evaluate it before paying. If the user makes use of this 
software for any purpose other than evaluation or evaluates it for a longer 
period than 30 days, then the user must purchase the software. 

In order to purchase the software you need to send your name (or any name 
you would like to register the software to) and payment (see options below). 
When payment is received, you will be e-mailed (if e-mail address is 
included) or mailed a password necessary to register the software. 
After getting an e-mail message with your password, follow the registration 
instructions, which will result in the creation of a password file in the 
same directory where ExamDiff.exe is located. When run, ExamDiff Pro will 
use the file to validate your registration. Please do not remove your 
password file from the ExamDiff Pro directory.

Upgrades are free for 12 months since the purchase of ExamDiff Pro. After 
12 months passes, you may (1) use the latest of your free upgrades forever, 
or (2) renew your license for the next 12 months at the current license 
renewal rate (see ExamDiff Pro Pricing below). To order an upgrade, please 
e-mail to orders@prestosoft.com with your current registration name and 
number of licenses you own (you can find this information in the About Box). 
In return you will be e-mailed instructions on how to purchase an upgrade.
If you purchased ExamDiff Pro more than once, and you wish to upgrade all 
of your separate licenses in one consolidated upgrade, please e-mail all 
of your registration names along with number of licenses for each 
registration.

There are several ways to purchase ExamDiff Pro:

1. By credit card:
     Visit ExamDiff Pro Ordering at
     http://www.prestosoft.com/ps.asp?page=edp_purchase

2. By purchase order
     PrestoSoft LLC does not process POs directly.
     Visit ExamDiff Pro Purchase Orders for the ways  
     of ordering ExamDiff Pro via POs at
     http://www.prestosoft.com/ps.asp?page=edp_po

3. By check, money order, or cash (US dollars only):
  - Print and fill the order form (see below)
  - Send the filled order form along with a check, money order, 
    or cash payable to PrestoSoft LLC to 
         P.O. Box 3611
         Thousand Oaks, CA 91359

4. By wire transfer:
  - Please e-mail  orders@prestosoft.com  to find out bank 
    information necessary to complete wire transfer
  - Transfer funds
  - Print and fill the order form
  - E-mail the filled order form to  orders@prestosoft.com 
    or send it to
         P.O. Box 3611
         Thousand Oaks, CA 91359

To check on the status of your order, please contact PrestoSoft LLC directly.

The credit card orders are handled by third party registration services. 
They notify PrestoSoft LLC, and within one business day of your order 
PrestoSoft LLC will send your password directly to you.


---------------------------------------------------------------------------
5. PRICING
---------------------------------------------------------------------------

The latest pricing information for ExamDiff Pro can always be found at 
http://www.prestosoft.com/ps.asp?page=edp_pricing.

The password(s) will be sent by e-mail. If you prefer postal mail, 
please add $5.00.


---------------------------------------------------------------------------
6. EXAMDIFF PRO (TM) ORDER FORM
---------------------------------------------------------------------------

Name:               __________________________________________

Registration name:  __________________________________________

Number of licenses: __________________________________________

Company:            __________________________________________

Street Number:      __________________________________________

City, State, Zip:   __________________________________________

Country:            __________________________________________

E-mail:             __________________________________________

Comments:           __________________________________________

                    __________________________________________

                    __________________________________________

                    __________________________________________ 

                    __________________________________________ 


---------------------------------------------------------------------------
7. END-USER LICENSE AGREEMENT
---------------------------------------------------------------------------

See EULA.TXT file distributed as part of this software package.
