/* ttfautohint-scripts.h */

/*
 * Copyright (C) 2013-2015 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* originally file `afscript.h' (2013-Aug-05) from FreeType */


/* The following part can be included multiple times. */
/* Define `SCRIPT' as needed. */


/*
 * Add new scripts here.  The first and second arguments are the
 * script name in lowercase and uppercase, respectively, followed
 * by a description string.  Then comes the corresponding HarfBuzz
 * script name tag, followed by the default characters (to derive
 * the standard width of stems).
 *
 * Note that fallback scripts only have a default style, thus we
 * use `HB_SCRIPT_INVALID' as the HarfBuzz script name tag for
 * them.
 */

SCRIPT(arab, ARAB,
       "Arabic",
       HB_SCRIPT_ARABIC,
       0x644, 0x62D, 0x640) /* ل ح ـ */

SCRIPT(cyrl, CYRL,
       "Cyrillic",
       HB_SCRIPT_CYRILLIC,
       0x43E, 0x41E, 0x0) /* оО */

SCRIPT(deva, DEVA,
       "Devanagari",
       HB_SCRIPT_DEVANAGARI,
       0x920, 0x935, 0x91F) /* ठ व ट */

SCRIPT(grek, GREK,
       "Greek",
       HB_SCRIPT_GREEK,
       0x3BF, 0x39F, 0x0) /* οΟ */

SCRIPT(hebr, HEBR,
       "Hebrew",
       HB_SCRIPT_HEBREW,
       0x5DD, 0x0, 0x0) /* ם */

/* only digit zero has a simple shape in the Lao script */
SCRIPT(lao, LAO,
       "Lao",
       HB_SCRIPT_LAO,
       0xED0, 0x0, 0x0) /* ໐ */

SCRIPT(latn, LATN,
       "Latin",
       HB_SCRIPT_LATIN,
       'o', 'O', '0')

SCRIPT(latb, LATB,
       "Latin Subscript Fallback",
       HB_SCRIPT_INVALID,
       0x2092, 0x2080, 0x0) /* ₒ ₀ */

SCRIPT(latp, LATP,
       "Latin Superscript Fallback",
       HB_SCRIPT_INVALID,
       0x1D52, 0x1D3C, 0x2070) /* ᵒ ᴼ ⁰ */

/* there are no simple forms for letters; we thus use two digit shapes */
SCRIPT(telu, TELU,
       "Telugu",
       HB_SCRIPT_TELUGU,
       0xC66, 0xC67, 0x0) /* ౦ ౧ */

SCRIPT(thai, THAI,
       "Thai",
       HB_SCRIPT_THAI,
       0xE32, 0xE45, 0xE50) /* า ๅ ๐ */

SCRIPT(none, NONE,
       "no script",
       HB_SCRIPT_INVALID,
       0x0, 0x0, 0x0)

/* end of ttfautohint-scripts.h */
