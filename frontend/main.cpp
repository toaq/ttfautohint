// main.cpp

// Copyright (C) 2011-2014 by Werner Lemberg.
//
// This file is part of the ttfautohint library, and may only be used,
// modified, and distributed under the terms given in `COPYING'.  By
// continuing to use, modify, or distribute this file you indicate that you
// have read `COPYING' and understand and accept it fully.
//
// The file `COPYING' mentioned in the previous paragraph is distributed
// with the ttfautohint library.


// This program is a wrapper for `TTF_autohint'.

#ifdef BUILD_GUI
#  ifndef _WIN32
#    define CONSOLE_OUTPUT
#  endif
#else
#  define CONSOLE_OUTPUT
#endif

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <unistd.h>
#include <locale.h>

#include <vector>
#include <string>

#if BUILD_GUI
#  include <QApplication>
#  include "maingui.h"
#else
#  include "info.h"
#endif

#include <ttfautohint.h>
#include <numberset.h>


#ifdef _WIN32
#  include <fcntl.h>
#  define SET_BINARY(f) do { \
                          if (!isatty(fileno(f))) \
                            setmode(fileno(f), O_BINARY); \
                        } while (0)
#endif

#ifndef SET_BINARY
#  define SET_BINARY(f) do {} while (0)
#endif


using namespace std;


// the available script tags and its descriptions are directly extracted
// from `ttfautohint-scripts.h'
typedef struct Script_Names_
{
  const char* tag;
  const char* description;
} Script_Names;

#undef SCRIPT
#define SCRIPT(s, S, d, h, sc1, sc2, sc3) \
          {#s, d},

const Script_Names script_names[] =
{
#include <ttfautohint-scripts.h>
  {NULL, NULL}
};


#ifndef BUILD_GUI
extern "C" {

typedef struct Progress_Data_
{
  long last_sfnt;
  bool begin;
  int last_percent;
} Progress_Data;


int
progress(long curr_idx,
         long num_glyphs,
         long curr_sfnt,
         long num_sfnts,
         void* user)
{
  Progress_Data* data = (Progress_Data*)user;

  if (num_sfnts > 1 && curr_sfnt != data->last_sfnt)
  {
    fprintf(stderr, "subfont %ld of %ld\n", curr_sfnt + 1, num_sfnts);
    data->last_sfnt = curr_sfnt;
    data->last_percent = 0;
    data->begin = true;
  }

  if (data->begin)
  {
    fprintf(stderr, "  %ld glyphs\n"
                    "   ", num_glyphs);
    data->begin = false;
  }

  // print progress approx. every 10%
  int curr_percent = curr_idx * 100 / num_glyphs;
  int curr_diff = curr_percent - data->last_percent;

  if (curr_diff >= 10)
  {
    fprintf(stderr, " %d%%", curr_percent);
    data->last_percent = curr_percent - curr_percent % 10;
  }

  if (curr_idx + 1 == num_glyphs)
    fprintf(stderr, "\n");

  return 0;
}


typedef struct Error_Data_
{
  const char* deltas_name;
} Error_Data;


void
err(TA_Error error,
    const char* error_string,
    unsigned int errlinenum,
    const char* errline,
    const char* errpos,
    void* user)
{
  Error_Data* data = static_cast<Error_Data*>(user);

  if (!error)
    return;

  // We replace some terse error strings with more user-friendly versions.
  if (error == TA_Err_Invalid_FreeType_Version)
    fprintf(stderr,
            "FreeType version 2.4.5 or higher is needed.\n"
            "Perhaps using a wrong FreeType DLL?\n");
  else if (error == TA_Err_Invalid_Font_Type)
    fprintf(stderr,
            "This font is not a valid font"
              " in SFNT format with TrueType outlines.\n"
            "In particular, CFF outlines are not supported.\n");
  else if (error == TA_Err_Already_Processed)
    fprintf(stderr,
            "This font has already been processed with ttfautohint.\n");
  else if (error == TA_Err_Missing_Legal_Permission)
    fprintf(stderr,
            "Bit 1 in the `fsType' field of the `OS/2' table is set:\n"
            "This font must not be modified"
              " without permission of the legal owner.\n"
            "Use command line option `-i' to continue"
              " if you have such a permission.\n");
  else if (error == TA_Err_Missing_Unicode_CMap)
    fprintf(stderr,
            "No Unicode character map.\n");
  else if (error == TA_Err_Missing_Symbol_CMap)
    fprintf(stderr,
            "No symbol character map.\n");
  else if (error == TA_Err_Missing_Glyph)
    fprintf(stderr,
            "No glyph for a standard character"
              " to derive standard width and height.\n"
            "Please check the documentation for a list of"
              " script-specific standard characters,\n"
            "or use option `--symbol'.\n");
  else
  {
    if (error < 0x100)
      fprintf(stderr, "An error with code 0x%02x occurred"
                        " while autohinting fonts",
                      error);
    else if (error >= 0x100 && error < 0x200)
    {
      fprintf(stderr, "An error with code 0x%03x occurred"
                        " while parsing the argument of option `-X'",
                      error);
      fprintf(stderr, errline ? ":\n" : ".\n");

      if (errline)
        fprintf(stderr, "  %s\n", errline);
      if (errpos && errline)
        fprintf(stderr, "  %*s\n", int(errpos - errline + 1), "^");
    }
    else if (error >= 0x200 && error < 0x300)
    {
      fprintf(stderr, "%s:", data->deltas_name);
      if (errlinenum)
        fprintf(stderr, "%d:", errlinenum);
      if (errpos && errline)
        fprintf(stderr, "%d:", int(errpos - errline + 1));
      if (error_string)
        fprintf(stderr, " %s", error_string);
      fprintf(stderr, " (0x%02X)\n", error);
      if (errline)
        fprintf(stderr, "  %s\n", errline);
      if (errpos && errline)
        fprintf(stderr, "  %*s\n", int(errpos - errline + 1), "^");
    }
  }
}


} // extern "C"
#endif // !BUILD_GUI


#ifdef CONSOLE_OUTPUT
static void
show_help(bool
#ifdef BUILD_GUI
               all
#endif
                  ,
          bool is_error)
{
  FILE* handle = is_error ? stderr : stdout;

  fprintf(handle,
#ifdef BUILD_GUI
"Usage: ttfautohintGUI [OPTION]...\n"
"A GUI application to replace hints in a TrueType font.\n"
#else
"Usage: ttfautohint [OPTION]... [IN-FILE [OUT-FILE]]\n"
"Replace hints in TrueType font IN-FILE and write output to OUT-FILE.\n"
"If OUT-FILE is missing, standard output is used instead;\n"
"if IN-FILE is missing also, standard input and output are used.\n"
#endif
"\n"
"The new hints are based on FreeType's auto-hinter.\n"
"\n"
"This program is a simple front-end to the `ttfautohint' library.\n"
"\n");

  fprintf(handle,
"Long options can be given with one or two dashes,\n"
"and with and without equal sign between option and argument.\n"
"This means that the following forms are acceptable:\n"
"`-foo=bar', `--foo=bar', `-foo bar', `--foo bar'.\n"
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
#ifdef BUILD_GUI
"Options not related to Qt or X11 set default values.\n"
#endif
"\n"
);

  fprintf(handle,
"Options:\n"
#ifndef BUILD_GUI
"      --debug                print debugging information\n"
#endif
"  -c, --composites           hint glyph composites also\n"
"  -d, --dehint               remove all hints\n"
"  -D, --default-script=S     set default OpenType script (default: latn)\n"
"  -f, --fallback-script=S    set fallback script (default: none)\n"
"  -G, --hinting-limit=N      switch off hinting above this PPEM value\n"
"                             (default: %d); value 0 means no limit\n"
"  -h, --help                 display this help and exit\n"
"  -H, --fallback-stem-width=N\n"
"                             set fallback stem width\n"
"                             (default: 50 font units at 2048 UPEM)\n"
#ifdef BUILD_GUI
"      --help-all             show Qt and X11 specific options also\n"
#endif
"  -i, --ignore-restrictions  override font license restrictions\n"
"  -l, --hinting-range-min=N  the minimum PPEM value for hint sets\n"
"                             (default: %d)\n"
#ifndef BUILD_GUI
"  -m, --deltas-file=FILE     get delta exceptions from FILE\n"
#endif
"  -n, --no-info              don't add ttfautohint info\n"
"                             to the version string(s) in the `name' table\n"
"  -p, --adjust-subglyphs     handle subglyph adjustments in exotic fonts\n",
          TA_HINTING_LIMIT, TA_HINTING_RANGE_MIN);
  fprintf(handle,
"  -r, --hinting-range-max=N  the maximum PPEM value for hint sets\n"
"                             (default: %d)\n"
"  -s, --symbol               input is symbol font\n"
"  -t, --ttfa-table           add TTFA information table\n"
"  -v, --verbose              show progress information\n"
"  -V, --version              print version information and exit\n"
"  -w, --strong-stem-width=S  use strong stem width routine for modes S,\n"
"                             where S is a string of up to three letters\n"
"                             with possible values `g' for grayscale,\n"
"                             `G' for GDI ClearType, and `D' for\n"
"                             DirectWrite ClearType (default: G)\n"
"  -W, --windows-compatibility\n"
"                             add blue zones for `usWinAscent' and\n"
"                             `usWinDescent' to avoid clipping\n"
"  -x, --increase-x-height=N  increase x height for sizes in the range\n"
"                             6<=PPEM<=N; value 0 switches off this feature\n"
"                             (default: %d)\n"
"  -X, --x-height-snapping-exceptions=STRING\n"
"                             specify a comma-separated list of\n"
"                             x-height snapping exceptions, for example\n"
"                             \"-9, 13-17, 19\" (default: \"\")\n"
"\n",
          TA_HINTING_RANGE_MAX, TA_INCREASE_X_HEIGHT);

#ifdef BUILD_GUI
  if (all)
  {
    fprintf(handle,
"Qt Options:\n"
"      --graphicssystem=SYSTEM\n"
"                             select a different graphics system backend\n"
"                             instead of the default one\n"
"                             (possible values: `raster', `opengl')\n"
"      --reverse              set layout direction to right-to-left\n");
    fprintf(handle,
"      --session=ID           restore the application for the given ID\n"
"      --style=STYLE          set application GUI style\n"
"                             (possible values: motif, windows, platinum)\n"
"      --stylesheet=SHEET     apply the given Qt stylesheet\n"
"                             to the application widgets\n"
"\n");

    fprintf(handle,
"X11 options:\n"
"      --background=COLOR     set the default background color\n"
"                             and an application palette\n"
"                             (light and dark shades are calculated)\n"
"      --bg=COLOR             same as --background\n"
"      --btn=COLOR            set the default button color\n"
"      --button=COLOR         same as --btn\n"
"      --cmap                 use a private color map on an 8-bit display\n"
"      --display=NAME         use the given X-server display\n");
    fprintf(handle,
"      --fg=COLOR             set the default foreground color\n"
"      --fn=FONTNAME          set the application font\n"
"      --font=FONTNAME        same as --fn\n"
"      --foreground=COLOR     same as --fg\n"
"      --geometry=GEOMETRY    set the client geometry of first window\n"
"      --im=SERVER            set the X Input Method (XIM) server\n"
"      --inputstyle=STYLE     set X Input Method input style\n"
"                             (possible values: onthespot, overthespot,\n"
"                             offthespot, root)\n");
    fprintf(handle,
"      --name=NAME            set the application name\n"
"      --ncols=COUNT          limit the number of colors allocated\n"
"                             in the color cube on an 8-bit display,\n"
"                             if the application is using the\n"
"                             QApplication::ManyColor color specification\n"
"      --title=TITLE          set the application title (caption)\n"
"      --visual=VISUAL        force the application\n"
"                             to use the given visual on an 8-bit display\n"
"                             (only possible value: TrueColor)\n"
"\n");
  }
#endif // BUILD_GUI

  fprintf(handle,
"The program accepts both TTF and TTC files as input.\n"
"Use option -i only if you have a legal permission to modify the font.\n"
"The used PPEM value for option -p is FUnits per em, normally 2048.\n"
"With option -s, use default values for standard stem width and height,\n"
"otherwise they are derived from script-specific characters\n"
"resembling the shape of character `o'.\n"
"\n");
  fprintf(handle,
"A hint set contains the optimal hinting for a certain PPEM value;\n"
"the larger the hint set range (as given by options -l and -r),\n"
"the more hint sets get computed, usually increasing the output font size.\n"
"The `gasp' table of the output file always enables grayscale hinting\n"
"for all sizes (limited by option -G, which is handled in the bytecode).\n"
"Increasing the value of -G does not increase the output font size.\n"
"\n");
  fprintf(handle,
"Options -f and -D take a four-letter string that identifies a script.\n"
"Option -f sets the script used as a fallback for glyphs that have\n"
"character codes outside of known script ranges.  Option -D sets the\n"
"default script for handling OpenType features.  Possible values are\n"
"\n");
  const Script_Names* sn = script_names;
  for(;;)
  {
    fprintf(handle, "  %s (%s)",
            sn->tag, sn->description);
    sn++;
    if (sn->tag)
      fprintf(handle, ",\n");
    else
    {
      fprintf(handle, ".\n");
      break;
    }
  }
  fprintf(handle,
#ifndef BUILD_GUI
"\n"
"A delta exceptions file contains lines of the form\n"
"\n"
"  [<subfont idx>] <glyph id> p <points> [x <shift>] [y <shift>] @ <ppems>\n"
"\n"
"to fine-tune point positions after hinting.  <glyph id> is a glyph name\n"
"or index, <shift> is in px, <points> and <ppems> are ranges as with\n"
"option `-X'.  `#' starts a line comment, which gets ignored.\n"
"Empty lines are ignored, too.\n"
#endif
"\n"
#ifdef BUILD_GUI
"A command-line version of this program is called `ttfautohint'.\n"
#else
"A GUI version of this program is called `ttfautohintGUI'.\n"
#endif
"\n"
"Report bugs to: freetype-devel@nongnu.org\n"
"ttfautohint home page: <http://www.freetype.org/ttfautohint>\n");

  if (is_error)
    exit(EXIT_FAILURE);
  else
    exit(EXIT_SUCCESS);
}


static void
show_version()
{
  fprintf(stdout,
#ifdef BUILD_GUI
"ttfautohintGUI " VERSION "\n"
#else
"ttfautohint " VERSION "\n"
#endif
"Copyright (C) 2011-2014 Werner Lemberg <wl@gnu.org>.\n"
"License: FreeType License (FTL) or GNU GPLv2.\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n");

  exit(EXIT_SUCCESS);
}
#endif // CONSOLE_OUTPUT


int
main(int argc,
     char** argv)
{
  int hinting_range_min = 0;
  int hinting_range_max = 0;
  int hinting_limit = 0;
  int increase_x_height = 0;
  int fallback_stem_width = 0;

  bool have_hinting_range_min = false;
  bool have_hinting_range_max = false;
  bool have_hinting_limit = false;
  bool have_increase_x_height = false;
  bool have_fallback_stem_width = false;

  bool gray_strong_stem_width = false;
  bool gdi_cleartype_strong_stem_width = true;
  bool dw_cleartype_strong_stem_width = false;

  bool ignore_restrictions = false;
  bool windows_compatibility = false;
  bool adjust_subglyphs = false;
  bool hint_composites = false;
  bool no_info = false;
  bool TTFA_info = false;
  bool symbol = false;

  const char* default_script = NULL;
  bool have_default_script = false;
  const char* fallback_script = NULL;
  bool have_fallback_script = false;
  const char* x_height_snapping_exceptions_string = NULL;
  bool have_x_height_snapping_exceptions_string = false;

  bool dehint = false;

#ifndef BUILD_GUI
  bool debug = false;

  TA_Progress_Func progress_func = NULL;
  TA_Error_Func err_func = err;
  TA_Info_Func info_func = info;

  const char* deltas_name = NULL;
#endif

  // For real numbers (both parsing and displaying) we only use `.' as the
  // decimal separator; similarly, we don't want localized formats like a
  // thousands separator for any number.
  setlocale(LC_NUMERIC, "C");

  // make GNU, Qt, and X11 command line options look the same;
  // we allow `--foo=bar', `--foo bar', `-foo=bar', `-foo bar',
  // and short options specific to ttfautohint

  // set up a new argument string
  vector<string> new_arg_string;
  new_arg_string.push_back(argv[0]);

  while (1)
  {
    // use pseudo short options for long-only options
    enum
    {
      PASS_THROUGH = CHAR_MAX + 1,
      HELP_ALL_OPTION,
      DEBUG_OPTION
    };

    static struct option long_options[] =
    {
      {"help", no_argument, NULL, 'h'},
#ifdef BUILD_GUI
      {"help-all", no_argument, NULL, HELP_ALL_OPTION},
#endif

      // ttfautohint options
      {"adjust-subglyphs", no_argument, NULL, 'p'},
      {"composites", no_argument, NULL, 'c'},
#ifndef BUILD_GUI
      {"debug", no_argument, NULL, DEBUG_OPTION},
#endif
      {"default-script", required_argument, NULL, 'D'},
      {"dehint", no_argument, NULL, 'd'},
#ifndef BUILD_GUI
      {"deltas-file", required_argument, NULL, 'm'},
#endif
      {"fallback-script", required_argument, NULL, 'f'},
      {"fallback-stem-width", required_argument, NULL, 'H'},
      {"hinting-limit", required_argument, NULL, 'G'},
      {"hinting-range-max", required_argument, NULL, 'r'},
      {"hinting-range-min", required_argument, NULL, 'l'},
      {"ignore-restrictions", no_argument, NULL, 'i'},
      {"increase-x-height", required_argument, NULL, 'x'},
      {"no-info", no_argument, NULL, 'n'},
      {"pre-hinting", no_argument, NULL, 'p'},
      {"strong-stem-width", required_argument, NULL, 'w'},
      {"symbol", no_argument, NULL, 's'},
      {"ttfa-table", no_argument, NULL, 't'},
      {"verbose", no_argument, NULL, 'v'},
      {"version", no_argument, NULL, 'V'},
      {"windows-compatibility", no_argument, NULL, 'W'},
      {"x-height-snapping-exceptions", required_argument, NULL, 'X'},

      // Qt options
      {"graphicssystem", required_argument, NULL, PASS_THROUGH},
      {"reverse", no_argument, NULL, PASS_THROUGH},
      {"session", required_argument, NULL, PASS_THROUGH},
      {"style", required_argument, NULL, PASS_THROUGH},
      {"stylesheet", required_argument, NULL, PASS_THROUGH},

      // X11 options
      {"background", required_argument, NULL, PASS_THROUGH},
      {"bg", required_argument, NULL, PASS_THROUGH},
      {"btn", required_argument, NULL, PASS_THROUGH},
      {"button", required_argument, NULL, PASS_THROUGH},
      {"cmap", no_argument, NULL, PASS_THROUGH},
      {"display", required_argument, NULL, PASS_THROUGH},
      {"fg", required_argument, NULL, PASS_THROUGH},
      {"fn", required_argument, NULL, PASS_THROUGH},
      {"font", required_argument, NULL, PASS_THROUGH},
      {"foreground", required_argument, NULL, PASS_THROUGH},
      {"geometry", required_argument, NULL, PASS_THROUGH},
      {"im", required_argument, NULL, PASS_THROUGH},
      {"inputstyle", required_argument, NULL, PASS_THROUGH},
      {"name", required_argument, NULL, PASS_THROUGH},
      {"ncols", required_argument, NULL, PASS_THROUGH},
      {"title", required_argument, NULL, PASS_THROUGH},
      {"visual", required_argument, NULL, PASS_THROUGH},

      {NULL, 0, NULL, 0}
    };

    int option_index;
    int c = getopt_long_only(argc, argv,
#ifdef BUILD_GUI
                             "cdD:f:G:hH:il:npr:stVvw:Wx:X:",
#else
                             "cdD:f:G:hH:il:m:npr:stVvw:Wx:X:",
#endif
                             long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
    case 'c':
      hint_composites = true;
      break;

    case 'd':
      dehint = true;
      break;

    case 'D':
      default_script = optarg;
      have_default_script = true;
      break;

    case 'f':
      fallback_script = optarg;
      have_fallback_script = true;
      break;

    case 'G':
      hinting_limit = atoi(optarg);
      have_hinting_limit = true;
      break;

    case 'h':
#ifdef CONSOLE_OUTPUT
      show_help(false, false);
#endif
      break;

    case 'H':
      fallback_stem_width = atoi(optarg);
      have_fallback_stem_width = true;
      break;

    case 'i':
      ignore_restrictions = true;
      break;

    case 'l':
      hinting_range_min = atoi(optarg);
      have_hinting_range_min = true;
      break;

#ifndef BUILD_GUI
    case 'm':
      deltas_name = optarg;
      break;
#endif

    case 'n':
      no_info = true;
      break;

    case 'p':
      adjust_subglyphs = true;
      break;

    case 'r':
      hinting_range_max = atoi(optarg);
      have_hinting_range_max = true;
      break;

    case 's':
      symbol = true;
      break;

    case 't':
      TTFA_info = true;
      break;

    case 'v':
#ifndef BUILD_GUI
      progress_func = progress;
#endif
      break;

    case 'V':
#ifdef CONSOLE_OUTPUT
      show_version();
#endif
      break;

    case 'w':
      gray_strong_stem_width = strchr(optarg, 'g') ? true : false;
      gdi_cleartype_strong_stem_width = strchr(optarg, 'G') ? true : false;
      dw_cleartype_strong_stem_width = strchr(optarg, 'D') ? true : false;
      break;

    case 'W':
      windows_compatibility = true;
      break;

    case 'x':
      increase_x_height = atoi(optarg);
      have_increase_x_height = true;
      break;

    case 'X':
      x_height_snapping_exceptions_string = optarg;
      have_x_height_snapping_exceptions_string = true;
      break;

#ifndef BUILD_GUI
    case DEBUG_OPTION:
      debug = true;
      break;
#endif

#ifdef BUILD_GUI
    case HELP_ALL_OPTION:
#ifdef CONSOLE_OUTPUT
      show_help(true, false);
#endif
      break;
#endif

    case PASS_THROUGH:
      {
        // append argument with proper syntax for Qt
        string arg;
        arg += '-';
        arg += long_options[option_index].name;

        new_arg_string.push_back(arg);
        if (optarg)
          new_arg_string.push_back(optarg);
        break;
      }

    default:
      exit(EXIT_FAILURE);
    }
  }

  if (dehint)
  {
    // -d makes ttfautohint ignore all other hinting options
    have_default_script = false;
    have_fallback_script = false;
    have_fallback_stem_width = false;
    have_hinting_range_max = false;
    have_hinting_range_min = false;
    have_hinting_limit = false;
    have_increase_x_height = false;
    have_x_height_snapping_exceptions_string = false;
  }

  if (!have_default_script)
    default_script = "latn";
  if (!have_fallback_script)
    fallback_script = "none";
  if (!have_hinting_range_min)
    hinting_range_min = TA_HINTING_RANGE_MIN;
  if (!have_hinting_range_max)
    hinting_range_max = TA_HINTING_RANGE_MAX;
  if (!have_hinting_limit)
    hinting_limit = TA_HINTING_LIMIT;
  if (!have_increase_x_height)
    increase_x_height = TA_INCREASE_X_HEIGHT;
  if (!have_x_height_snapping_exceptions_string)
    x_height_snapping_exceptions_string = "";
  if (!have_fallback_stem_width)
    fallback_stem_width = 0; /* redundant, but avoids a compiler warning */

#ifndef BUILD_GUI

  if (!isatty(fileno(stderr)) && !debug)
    setvbuf(stderr, (char*)NULL, _IONBF, BUFSIZ);

  if (hinting_range_min < 2)
  {
    fprintf(stderr, "The hinting range minimum must be at least 2\n");
    exit(EXIT_FAILURE);
  }
  if (hinting_range_max < hinting_range_min)
  {
    fprintf(stderr, "The hinting range maximum must not be smaller"
                    " than the minimum (%d)\n",
                    hinting_range_min);
    exit(EXIT_FAILURE);
  }
  if (hinting_limit != 0 && hinting_limit < hinting_range_max)
  {
    fprintf(stderr, "A non-zero hinting limit must not be smaller"
                    " than the hinting range maximum (%d)\n",
                    hinting_range_max);
    exit(EXIT_FAILURE);
  }
  if (increase_x_height != 0 && increase_x_height < 6)
  {
    fprintf(stderr, "A non-zero x height increase limit"
                    " must be larger than or equal to 6\n");
    exit(EXIT_FAILURE);
  }
  if (have_fallback_stem_width && fallback_stem_width <= 0)
  {
    fprintf(stderr, "The fallback stem width"
                    " must be a positive integer\n");
    exit(EXIT_FAILURE);
  }

  if (have_default_script)
  {
    const Script_Names* sn;

    for (sn = script_names; sn->tag; sn++)
      if (!strcmp(default_script, sn->tag))
        break;
    if (!sn->tag)
    {
      fprintf(stderr, "Unknown script tag `%s'\n", default_script);
      exit(EXIT_FAILURE);
    }
  }

  if (have_fallback_script)
  {
    const Script_Names* sn;

    for (sn = script_names; sn->tag; sn++)
      if (!strcmp(fallback_script, sn->tag))
        break;
    if (!sn->tag)
    {
      fprintf(stderr, "Unknown script tag `%s'\n", fallback_script);
      exit(EXIT_FAILURE);
    }
  }

  if (symbol
      && have_fallback_stem_width
      && !strcmp(fallback_script, "none"))
    fprintf(stderr,
            "Warning: Setting a fallback stem width for a symbol font\n"
            "         without setting a fallback script has no effect\n");

  int num_args = argc - optind;

  if (num_args > 2)
    show_help(false, true);

  FILE* in;
  if (num_args > 0)
  {
    in = fopen(argv[optind], "rb");
    if (!in)
    {
      fprintf(stderr,
              "The following error occurred while opening font `%s':\n"
              "\n"
              "  %s\n",
              argv[optind], strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    if (isatty(fileno(stdin)))
      show_help(false, true);
    in = stdin;
  }

  FILE* out;
  if (num_args > 1)
  {
    if (!strcmp(argv[optind], argv[optind + 1]))
    {
      fprintf(stderr, "Input and output file names must not be identical\n");
      exit(EXIT_FAILURE);
    }

    out = fopen(argv[optind + 1], "wb");
    if (!out)
    {
      fprintf(stderr,
              "The following error occurred while opening font `%s':\n"
              "\n"
              "  %s\n",
              argv[optind + 1], strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    if (isatty(fileno(stdout)))
      show_help(false, true);
    out = stdout;
  }

  FILE* deltas = NULL;
  if (deltas_name)
  {
    deltas = fopen(deltas_name, "r");
    if (!deltas)
    {
      fprintf(stderr,
              "The following error occurred while open deltas file `%s':\n"
              "\n"
              "  %s\n",
              deltas_name, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  else
    deltas = NULL;

  Progress_Data progress_data = {-1, 1, 0};
  Error_Data error_data = {deltas_name};
  Info_Data info_data;

  if (no_info)
    info_func = NULL;
  else
  {
    info_data.data = NULL; // must be deallocated after use
    info_data.data_wide = NULL; // must be deallocated after use
    info_data.data_len = 0;
    info_data.data_wide_len = 0;

    info_data.deltas_name = deltas_name;

    info_data.hinting_range_min = hinting_range_min;
    info_data.hinting_range_max = hinting_range_max;
    info_data.hinting_limit = hinting_limit;

    info_data.gray_strong_stem_width = gray_strong_stem_width;
    info_data.gdi_cleartype_strong_stem_width = gdi_cleartype_strong_stem_width;
    info_data.dw_cleartype_strong_stem_width = dw_cleartype_strong_stem_width;

    info_data.windows_compatibility = windows_compatibility;
    info_data.adjust_subglyphs = adjust_subglyphs;
    info_data.hint_composites = hint_composites;
    info_data.increase_x_height = increase_x_height;
    info_data.x_height_snapping_exceptions_string = x_height_snapping_exceptions_string;
    info_data.fallback_stem_width = fallback_stem_width;
    info_data.symbol = symbol;
    info_data.TTFA_info = TTFA_info;

    strncpy(info_data.default_script,
            default_script,
            sizeof (info_data.default_script));
    strncpy(info_data.fallback_script,
            fallback_script,
            sizeof (info_data.fallback_script));

    info_data.dehint = dehint;

    int ret = build_version_string(&info_data);
    if (ret == 1)
      fprintf(stderr, "Warning: Can't allocate memory"
                      " for ttfautohint options string in `name' table\n");
    else if (ret == 2)
      fprintf(stderr, "Warning: ttfautohint options string"
                      " in `name' table too long\n");
  }

  if (in == stdin)
    SET_BINARY(stdin);
  if (out == stdout)
    SET_BINARY(stdout);

  TA_Error error =
    TTF_autohint("in-file, out-file, deltas-file,"
                 "hinting-range-min, hinting-range-max, hinting-limit,"
                 "gray-strong-stem-width, gdi-cleartype-strong-stem-width,"
                 "dw-cleartype-strong-stem-width,"
                 "progress-callback, progress-callback-data,"
                 "error-callback, error-callback-data,"
                 "info-callback, info-callback-data,"
                 "ignore-restrictions, windows-compatibility,"
                 "adjust-subglyphs, hint-composites,"
                 "increase-x-height, x-height-snapping-exceptions,"
                 "fallback-stem-width, default-script, fallback-script,"
                 "symbol, dehint, debug, TTFA-info",
                 in, out, deltas,
                 hinting_range_min, hinting_range_max, hinting_limit,
                 gray_strong_stem_width, gdi_cleartype_strong_stem_width,
                 dw_cleartype_strong_stem_width,
                 progress_func, &progress_data,
                 err_func, &error_data,
                 info_func, &info_data,
                 ignore_restrictions, windows_compatibility,
                 adjust_subglyphs, hint_composites,
                 increase_x_height, x_height_snapping_exceptions_string,
                 fallback_stem_width, default_script, fallback_script,
                 symbol, dehint, debug, TTFA_info);

  if (!no_info)
  {
    free(info_data.data);
    free(info_data.data_wide);
  }

  if (in != stdin)
    fclose(in);
  if (out != stdout)
    fclose(out);
  if (deltas)
    fclose(deltas);

  exit(error ? EXIT_FAILURE : EXIT_SUCCESS);

  return 0; // never reached

#else // BUILD_GUI

  int new_argc = new_arg_string.size();
  char** new_argv = new char*[new_argc];

  // construct new argc and argv variables from collected data
  for (int i = 0; i < new_argc; i++)
    new_argv[i] = const_cast<char*>(new_arg_string[i].data());

  QApplication app(new_argc, new_argv);
  app.setApplicationName("TTFautohint");
  app.setApplicationVersion(VERSION);
  app.setOrganizationName("FreeType");
  app.setOrganizationDomain("freetype.org");

  bool alternative_layout = false;
  {
    // Display the window off the screen -- to get proper window dimensions
    // including the frame, the window manager must have a chance to
    // decorate it.
    //
    // We don't want to change the default window positioning algorithm of
    // the platform's window manager, so we create the main GUI window
    // twice.
    //
    // The original idea, however, was to simply move the off-screen window
    // back to the screen with
    //
    //   gui.move(100, 100);
    //   gui.setAttribute(Qt::WA_Moved, false);
    //   gui.show();
    //
    // (unsetting the `WA_Moved' attribute makes the window manager handle
    // the previous call to `move' as a position suggestion instead of a
    // request).  Unfortuntely, there seems to be a bug in Qt 4.8.4 which
    // prevents any effect of unsetting `WA_Moved' if `show' has already
    // been called.

    Main_GUI dummy(alternative_layout,
                   hinting_range_min, hinting_range_max, hinting_limit,
                   gray_strong_stem_width, gdi_cleartype_strong_stem_width,
                   dw_cleartype_strong_stem_width, increase_x_height,
                   x_height_snapping_exceptions_string, fallback_stem_width,
                   ignore_restrictions, windows_compatibility, adjust_subglyphs,
                   hint_composites, no_info, default_script, fallback_script,
                   symbol, dehint, TTFA_info);

    dummy.move(-50000, -50000);
    dummy.show();

    // if the vertical size of our window is too large,
    // select a horizontal layout
    QRect screen(QApplication::desktop()->availableGeometry());
    if (dummy.frameGeometry().height() > screen.width())
      alternative_layout = true;
  }

  Main_GUI gui(alternative_layout,
               hinting_range_min, hinting_range_max, hinting_limit,
               gray_strong_stem_width, gdi_cleartype_strong_stem_width,
               dw_cleartype_strong_stem_width, increase_x_height,
               x_height_snapping_exceptions_string, fallback_stem_width,
               ignore_restrictions, windows_compatibility, adjust_subglyphs,
               hint_composites, no_info, default_script, fallback_script,
               symbol, dehint, TTFA_info);
  gui.show();

  return app.exec();

#endif // BUILD_GUI
}

// end of main.cpp
