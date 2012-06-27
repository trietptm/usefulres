/*

        *** For non-Unix systems ***

        This program calls Watcom wlink command
        with correctly generated response file.

        It also calls TLINK (BCC) if compiled with BCC

 ver    Created 25-Jun-95 by I. Guilfanov.

 2.0    adapted for BCC

*/

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>

char line[64*1024];

int verbose = 1;
int isbor = 0;                          /* for borland */

static char rspname[100] = "rsptmp";

static void permutate(char *ptr, const char *user, int const_files);
static int prc1(const char *file, const char *user); /* Process indirect file */
static int run(char *line);
/*------------------------------------------------------------------------*/
int main(int argc,char *argv[])
{
  char *sw;
  int code;
  int i;
  char *linker = "wlink";
  char *user = NULL;
  int keep = 0;

#define SW_CHAR '_'

  while ( argc > 1 && *argv[1] == SW_CHAR )
  {
    switch( argv[1][1] )
    {
      case 'b':
        isbor = 1;
        linker = "tlink ";
        break;
      case 'l':
        linker = &argv[1][2];
        break;
      case 'k':
        keep = 1;
        break;
      case 'u':
        user = &argv[1][2];
        if ( user[0] != '\0' ) break;
        // no break
      default:
usage:      
        fprintf(stderr,"ld: illegal switch '%c'\n",argv[1][1]);
        return 1;
    }
    argc--;
    argv++;
  }
  sprintf(line,"%s ",linker);

  if ( argc < 2 ) {
    printf(     "ld version 2.2\n"
                "\tUsage: ld [%cl##] [%cb] [%cu...] ...\n"
                "\t %cl - linker name\n"
                "\t %cb - borland style\n",
                "\t %cu - user data\n",
                "\t %ck - keep temporary file\n",
                SW_CHAR,
                SW_CHAR,
                SW_CHAR,
                SW_CHAR,
                SW_CHAR);
    return 1;
  }
  for ( i=1; i < argc; i++ )
  {
    if ( argv[i][0] == '@' )
    {
      static int first = 1;
      if ( !first )
      {
        fprintf(stderr,"ld: only one indirect file is allowed\n");
        return 1;
      }
      first = 0;
      code = prc1(&argv[i][1], user);
      if ( code != 0 ) return code;
      strcat(line," @");
      strcat(line,rspname);
      continue;
    }
    strcat(line," ");
    strcat(line,argv[i]);
  }
  if ( verbose ) printf("ld: %s\n",line);
  code = run(line);
  if ( !keep ) unlink(rspname);
  return code;
}

static char fl[4096];

/*------------------------------------------------------------------------*/
static int prc1(const char *file, const char *user) /* Process indirect file */
{
  FILE *fpo;
  FILE *fp = fopen(file,"r");
  if ( fp == 0 ) {
    fprintf(stderr,"ld: can't open indirect file\n");
    return 1;
  }
  fpo = fopen(tmpnam(rspname),"w");
  if ( fpo == 0 ) {
    fprintf(stderr,"ld: can't create temp file\n");
    return 1;
  }
  while ( fgets(fl,sizeof(fl),fp) )
  {
    if ( strncmp(fl, "noperm", 6) == 0 )
    {
      fputs(fl+6,fpo);
      continue;
    }
    if ( strncmp(fl,"file",4) == 0 || strncmp(fl,"lib",3) == 0 )
    {
      char *ptr = fl;
      // skip word and spaces
      while ( *ptr != ' ' && *ptr != '\t' && *ptr != 0 ) ptr++;
      while ( isspace(*ptr) ) ptr++;
      if ( user != NULL ) permutate(ptr, user, 0);
      if ( isbor )
      {
        fputs(ptr,fpo);
        continue;
      }
      *(ptr-1) = '\0';
      while ( 1 )
      {
        while ( isspace(*ptr) ) ptr++;
        if ( *ptr == '\0' ) break;
        fputs(fl,fpo);
        putc(' ',fpo);
        while ( *ptr != ' ' && *ptr != '\t' && *ptr != 0 ) putc(*ptr,fpo),ptr++;
        putc('\n',fpo);
      }
      continue;
    }
    if ( !isbor ) fputs(fl,fpo);
  }
  fclose(fp);
  fclose(fpo);
  return 0;
}

/*---------------------------------------------------------------------*/
static int run(char *line) 
{
  char *nargv[10000];
  char *ptr = line;
  int i;
  for ( i=0; i < 10000-1; i++ ) 
  {
    while ( *ptr == ' ' || *ptr == '\t' ) ptr++;
    if ( *ptr == '\0' ) break;
    nargv[i] = ptr;
    if ( *ptr == '"' )
      while ( *ptr != '"' && *ptr != '\0' ) 
      {
        if ( *ptr == '\\' ) memmove(ptr,ptr+1,strlen(ptr));
        ptr++;
      }
    else
      while ( *ptr != ' ' && *ptr != '\t' && *ptr != '\0' ) ptr++;
    if ( *ptr != '\0' ) *ptr++ = '\0';
  }
  nargv[i] = NULL;
  i = spawnvp(P_WAIT,nargv[0],nargv);
  if ( i == -1 ) {
    perror("exec error");
    exit(3);
  }
  return i;
}

/*------------------------------------------------------------------------*/
static int extract_words(const char *ptr, char **words, int maxwords)
{
  int n = 0;
  while ( 1 )
  {
    const char *beginning;
    int len;
    while ( isspace(*ptr) ) ptr++;
    if ( *ptr == 0 ) break;
    if ( n >= maxwords ) 
    {
      fprintf(stderr,"ld: too many words for permutation\n");
      exit(1);
    }
    beginning = ptr;
    while ( !isspace(*ptr) && *ptr != 0 ) ptr++;
    len = ptr - beginning;
    words[0] = malloc(len+1);
    memcpy(words[0], beginning, len);
    words[0][len] = 0;
    words++;
    n++;
  }
  return n;
}

/*------------------------------------------------------------------------*/
static void permutate(char *ptr, const char *user, int const_files)
{
#define MAX_WORDS 1024
  char *words[MAX_WORDS];
  int n = extract_words(ptr, words, MAX_WORDS);
  const char *ud = user;
  int i;
  for ( i=0; i < n; i++ )
  {
    int idx = i;
    if ( i >= const_files )
    {
      char x = *ud++;
      if ( x == 0 )
      {
        ud = user;
        x = *ud++;
      }
      idx = (unsigned char)(x) % (n-i);
      if ( i != 0 ) *ptr++ = ' ';
    }  
    // output the selected word  
    ptr = stpcpy(ptr, words[idx]);
    // delete the used word
    free(words[idx]);
    memmove(&words[idx], &words[idx+1], sizeof(char*)*(n-idx-1));
  }
  *ptr++ = '\n';
  *ptr = 0;
}

