/*
 * "$Id: texttops.c,v 1.1 1996/10/14 16:07:57 mike Exp $"
 *
 *   PostScript text output filter for espPrint, a collection of printer
 *   drivers.
 *
 *   Copyright 1993-1996 by Easy Software Products
 *
 *   These coded instructions, statements, and computer  programs  contain
 *   unpublished  proprietary  information  of Easy Software Products, and
 *   are protected by Federal copyright law.  They may  not  be  disclosed
 *   to  third  parties  or  copied or duplicated in any form, in whole or
 *   in part, without the prior written consent of Easy Software Products.
 *
 * Contents:
 *
 * Revision History:
 *
 *   $Log: texttops.c,v $
 *   Revision 1.1  1996/10/14 16:07:57  mike
 *   Initial revision
 *
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pod.h>
#include <errorcodes.h>
#include <license.h>


/*
 * Constants...
 */

#ifndef FALSE
#  define FALSE	0
#  define TRUE	(!FALSE)
#endif /* !FALSE */

#define MAX_COLUMNS	256
#define MAX_LINES	256

#define ATTR_BOLD	0x01
#define ATTR_UNDERLINE	0x02


/*
 * Character/attribute structure...
 */

typedef struct
{
  char ch,		/* ASCII character */
       attr;		/* Any attributes */
} lchar_t;


/*
 * Globals...
 */

int	Verbosity = 0;
int	SizeLines = 60,
	SizeColumns = 80,
	PageColumns = 1;

float	CharsPerInch = 10.0;
int	LinesPerInch = 6;


/*
 * 'Setup()' - Output a PostScript prolog for this page.
 */

void
Setup(FILE              *out,
      PDSizeTableStruct *size,
      char              *fontname,
      float		fontsize,
      int		landscape)
{
  int	i;
  float	length, width, left;


  LinesPerInch = 72.0 / fontsize;
  CharsPerInch = 120.0 / fontsize;
  
  if (landscape)
  {
    SizeColumns = (size->length - 2.0 * size->top_margin - (PageColumns - 1) * 0.25) * CharsPerInch;
    SizeLines   = (size->width - 2.0 * size->left_margin) * LinesPerInch;

    left   = 72.0 * size->top_margin;
    width  = 72.0 * (size->length - 2.0 * size->top_margin) / PageColumns;
    length = 72.0 * (size->width - size->left_margin) - fontsize;
  }
  else
  {
    SizeColumns = (size->width - 2.0 * size->left_margin - (PageColumns - 1) * 0.25) * CharsPerInch;
    SizeLines   = (size->length - 2.0 * size->top_margin) * LinesPerInch;

    left   = 72.0 * size->left_margin;
    width  = 72.0 * (size->width - 2.0 * size->left_margin) / PageColumns;
    length = 72.0 * (size->length - size->top_margin) - fontsize;
  };

  SizeColumns /= PageColumns;

  fputs("%!PS-Adobe-3.0\n", out);
  fprintf(out, "%%%%BoundingBox: %f %f %f %f\n",
          72.0 * size->left_margin, 
          72.0 * size->top_margin, 
          72.0 * (size->width - size->left_margin), 
          72.0 * (size->length - size->left_margin));
  fputs("%%LanguageLevel: 1\n", out);
  fputs("%%Creator: ntext2ps 3.1 Copyright 1993-1996 Easy Software Products\n", out);
  fprintf(out, "%%%%Pages: (atend)\n");
  fputs("%%EndComments\n\n", out);

  fputs("%%BeginProlog\n", out);
  fprintf(out, "/R /%s findfont %f scalefont def\n", fontname, fontsize);
  fprintf(out, "/B /%s-Bold findfont %f scalefont def\n", fontname, fontsize);
  fprintf(out, "/S { setfont /y exch %f mul %f sub neg def %f mul %f add exch %f mul add /x exch def "
               "x y moveto show } bind def\n",
          fontsize, length, 72.0 / CharsPerInch, left, width);
  fprintf(out, "/U { setfont /y exch %f mul %f sub neg def %f mul %f add exch %f mul add /x exch def "
               "x y moveto dup show x y moveto stringwidth rlineto } bind def\n",
          fontsize, length, 72.0 / CharsPerInch, left, width);
  fputs("%%EndProlog\n", out);
}


void
Shutdown(FILE *out,
         int  pages)
{
  fprintf(out, "%%%%Pages: %d\n", pages);
  fputs("%%EOF\n", out);
}


void
StartPage(FILE *out,
          int  page)
{
  fprintf(out, "%%%%Page: %d\n", page);
}


void
EndPage(FILE *out)
{
  fputs("showpage\n", out);
  fputs("%%EndPage\n", out);
}


void
output_line(FILE *out, int page_column, int column, int row, int attr, char *line)
{
  fprintf(out, "(%s) %d %d %d %s %s\n", line, page_column, column,
          row, (attr & ATTR_BOLD) ? "B" : "R", (attr & ATTR_UNDERLINE) ? "U" : "S");
}


void
OutputLine(FILE    *out,
           int     page_column,
           int     row,
           lchar_t *buffer)
{
  int	column,
        linecol,
	attr;
  char	line[MAX_COLUMNS * 2 + 1],
	*lineptr;


  for (column = 0, attr = 0, lineptr = line, linecol = 0;
       column < SizeColumns;
       column ++, buffer ++)
  {
    if (buffer->ch == '\0')
      break;

    if (attr ^ buffer->attr)
    {
      if (lineptr > line)
      {
        *lineptr = '\0';
        output_line(out, page_column, linecol, row, attr, line);
        lineptr = line;
      };

      attr    = buffer->attr;
      linecol = column;
    };

    if (strchr("()\\", buffer->ch) != NULL)
    {
      *lineptr = '\\';
      lineptr ++;
    };

    *lineptr = buffer->ch;
    lineptr ++;
  };

  if (lineptr > line)
  {
    *lineptr = '\0';
    output_line(out, page_column, linecol, row, attr, line);
  };
}


/*
 * 'Usage()' - Print usage message and exit.
 */

void
Usage(void)
{    
  fputs("Usage: ntext2ps -P <printer-name> [-D]\n", stderr);
  fputs("               [-e] [-s] [-w] [-Z]\n", stderr);
  fputs("               [-L <log-file>] [-O <output-file>]\n", stderr);
  fputs("               [-M <printer-model]\n", stderr);

  exit(ERR_BAD_ARG);
}


/*
 * 'main()' - Main entry and processing of driver.
 */

void
main(int  argc,    /* I - Number of command-line arguments */
     char *argv[]) /* I - Command-line arguments */
{
  int			i,		/* Looping var */
			ch;		/* Current char from file */
  char			*opt;		/* Current option character */
  int			empty_infile,	/* TRUE if the input file is empty */
			need_status;	/* TRUE if all we need to do is update the printer status */
  char			*filename,	/* Input filename, if specified (NULL otherwise). */
			*printer;	/* Name of printer */
  FILE			*fp;		/* Input file */
  int			line,
  			column,
  			page_column,
  			page,
  			landscape;
  char			*fontname;
  float			fontsize;
  PDInfoStruct		*info;		/* POD info */
  PDStatusStruct	*status;	/* POD status */
  time_t		mod_time;	/* Modification time */
  PDSizeTableStruct	*size;		/* Page size */
  char			*outfile;
  FILE			*out;
  lchar_t		buffer[MAX_COLUMNS];


 /*
  * Process any command-line args...
  */

  filename  = NULL;
  outfile   = NULL;
  fontname  = "Courier";
  fontsize  = 12.0;
  landscape = 0;

  if (argc < 3)
    Usage();

  for (i = 1; i < argc; i ++)
    if (argv[i][0] == '-')
      for (opt = argv[i] + 1; *opt != '\0'; opt ++)
        switch (*opt)
        {
          case 'P' : /* Specify the printer name */
              i ++;
              if (i >= argc)
                Usage();

              printer = argv[i];
              break;

          case 'L' : /* Log file */
              i ++;
              if (i >= argc)
                Usage();

              freopen(argv[i], "w", stderr);
              break;

          case 'O' : /* Output file */
              i ++;
              if (i >= argc || outfile != NULL)
                Usage();

              outfile = argv[i];
              break;

          case 'D' : /* Produce debugging messages */
              Verbosity ++;
              break;

          case 'l' : /* Landscape output */
              landscape = TRUE;
              break;

          case 'F' : /* Font name */
              i ++;
              if (i >= argc)
                Usage();

              fontname = argv[i];
              break;

          case 'p' : /* Font pointsize */
              i ++;
              if (i >= argc)
                Usage();

              fontsize = atof(argv[i]);
              break;

          case 'M' : /* Multiple column mode */
              i ++;
              if (i >= argc)
                Usage();

              PageColumns = atof(argv[i]);
              break;

          default :
              Usage();
              break;
        }
    else if (filename != NULL)
      Usage();
    else
      filename = argv[i];

  if (Verbosity)
  {
    fputs("ntext2ps: Command-line args are:", stderr);
    for (i = 1; i < argc; i ++)
      fprintf(stderr, " %s", argv[i]);
    fputs("\n", stderr);
  };

 /*
  * Check for necessary args...
  */

  if (printer == NULL)
    Usage();

 /*
  * Open the POD database files and get the printer definition record.
  */
  
  if (PDLocalReadInfo(printer, &info, &mod_time) < 0)
  {
    fprintf(stderr, "ntext2ps: Could not open required POD database files for printer \'%s\'.\n", 
            printer);
    fprintf(stderr, "         Are you sure all required POD files are properly installed?\n");

    PDPerror("ntext2ps");
    exit(ERR_POD_ACCESS);
  };

  status = info->active_status;
  size   = PDFindPageSize(info, PD_SIZE_CURRENT);

 /*
  * Setup the output file...
  */

  if (outfile == NULL)
    out = stdout;
  else
    out = fopen(outfile, "w");

  if (out == NULL)
  {
    fprintf(stderr, "ntext2ps: Unable to create PostScript output to %s - %s\n",
            outfile == NULL ? "(stdout)" : outfile, strerror(errno));
    exit(ERR_TRANSMISSION);
  };

  Setup(out, size, fontname, fontsize, landscape);

 /*
  * Read text from the specified source and print them...
  */

  if (filename != NULL)
  {
    if ((fp = fopen(filename, "r")) == NULL)
    {
      Shutdown(out, 0);
      exit(ERR_DATA_FILE);
    };
  }
  else
    fp = stdin;

  if (fp == NULL)
  {
    Shutdown(out, 0);
    exit(ERR_DATA_SHORT_FILE);
  };

  empty_infile = TRUE;
  column       = 0;
  line         = -1;
  page         = 0;
  page_column  = 0;

  memset(buffer, 0, sizeof(buffer));

  while ((ch = getc(fp)) >= 0)
  {
    empty_infile = FALSE;

    if (line < 0)
    {
      page ++;
      StartPage(out, page);
      line         = 0;
      page_column  = 0;
    };

    switch (ch)
    {
      case 0x08 :		/* BS - backspace for boldface & underline */
          if (column > 0)
            column --;
          break;
      case 0x09 :		/* HT - tab to next 8th column */
          do
          {
            if (column >= SizeColumns)
            {			/* Wrap text to margins */
              OutputLine(out, page_column, line, buffer);
              line ++;
              if (line >= SizeLines)
              {
                page_column ++;
                line = 0;
                if (page_column >= PageColumns)
                {
          	  EndPage(out);
        	  line = -1;
        	};
              };
              memset(buffer, 0, sizeof(buffer));
              column = 0;
            };

            buffer[column].ch = ' ';
            column ++;
          }
          while (column & 7);
          break;
      case 0x0a :		/* LF - output current line */
          OutputLine(out, page_column, line, buffer);
          line ++;
          if (line >= SizeLines)
          {
            page_column ++;
            line = 0;
            if (page_column >= PageColumns)
            {
              EndPage(out);
              line = -1;
            };
          };
          memset(buffer, 0, sizeof(buffer));
          column = 0;
          break;
      case 0x0c :		/* FF - eject current page... */
          OutputLine(out, page_column, line, buffer);
          page_column ++;
          line = 0;
          if (page_column >= PageColumns)
          {
            EndPage(out);
            line = -1;
          };
          memset(buffer, 0, sizeof(buffer));
          column = 0;
          break;
      case 0x0d :		/* CR - ignored */
          break;
      default :			/* All others... */
          if (ch < ' ')
            break;		/* Ignore other control chars */

          if (column >= SizeColumns)
          {			/* Wrap text to margins */
            OutputLine(out, page_column, line, buffer);
            line ++;
            if (line >= SizeLines)
            {
              page_column ++;
              line = 0;
              if (page_column >= PageColumns)
              {
        	EndPage(out);
        	line = -1;
              };
            };
            memset(buffer, 0, sizeof(buffer));
            column = 0;
          };

          if (ch == buffer[column].ch)
            buffer[column].attr |= ATTR_BOLD;
          else if (buffer[column].ch == '_')
            buffer[column].attr |= ATTR_UNDERLINE;

          buffer[column].ch = ch;
          column ++;
          break;          
    };
  };

  if (line >= 0)
  {
    OutputLine(out, page_column, line, buffer);
    EndPage(out);
    page ++;
  };

  Shutdown(out, page);

  if (empty_infile)
    exit(ERR_DATA_SHORT_FILE);

 /*
  * Exit with no errors...
  */

  exit(NO_ERROR);
}


/*
 * End of "$Id: texttops.c,v 1.1 1996/10/14 16:07:57 mike Exp $".
 */
