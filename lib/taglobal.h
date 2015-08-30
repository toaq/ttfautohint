/* taglobal.h */

/*
 * Copyright (C) 2011-2015 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* originally file `afglobal.h' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#ifndef __TAGLOBAL_H__
#define __TAGLOBAL_H__

#include "ta.h"
#include "tatypes.h"
#include "taharfbuzz.h"


extern TA_WritingSystemClass const ta_writing_system_classes[];


#undef SCRIPT
#define SCRIPT(s, S, d, h, sc1, sc2, sc3) \
          extern const TA_ScriptClassRec ta_ ## s ## _script_class;

#include "ttfautohint-scripts.h"

extern TA_ScriptClass const ta_script_classes[];


#undef STYLE
#define STYLE(s, S, d, ws, sc, ss, c) \
          extern const TA_StyleClassRec ta_ ## s ## _style_class;

#include "tastyles.h"

extern TA_StyleClass const ta_style_classes[];


#ifdef TA_DEBUG
extern const char* ta_style_names[];
#endif


/* Default values and flags for both autofitter globals */
/* (originally found in AF_ModuleRec, we use FONT instead) */
/* and face globals (in TA_FaceGlobalsRec). */

/* index of fallback style in `ta_style_classes' */
#define TA_STYLE_FALLBACK TA_STYLE_NONE_DFLT
/* default script for OpenType */
#define TA_SCRIPT_DEFAULT  AF_SCRIPT_LATN
/* a bit mask indicating an uncovered glyph */
#define TA_STYLE_UNASSIGNED 0x7FFF
/* if this flag is set, we have an ASCII digit */
#define TA_DIGIT 0x8000U

/* `increase-x-height' property */
#define TA_PROP_INCREASE_X_HEIGHT_MIN 6
#define TA_PROP_INCREASE_X_HEIGHT_MAX 0


/* note that glyph_styles[] maps each glyph to an index into the */
/* `ta_style_classes' array. */
typedef struct TA_FaceGlobalsRec_
{
  FT_Face face;
  FT_Long glyph_count; /* same as face->num_glyphs */
  FT_UShort* glyph_styles;

  hb_font_t* hb_font;

  /* per-face auto-hinter properties */
  FT_UInt increase_x_height;

  TA_StyleMetrics metrics[TA_STYLE_MAX];

  FONT* font; /* to access global properties */
} TA_FaceGlobalsRec;


/* this models the global hints data for a given face, */
/* decomposed into style-specific items */

FT_Error
ta_face_globals_new(FT_Face face,
                    TA_FaceGlobals *aglobals,
                    FONT* font);

FT_Error
ta_face_globals_get_metrics(TA_FaceGlobals globals,
                            FT_UInt gindex,
                            FT_UInt options,
                            TA_StyleMetrics *ametrics);

void
ta_face_globals_free(TA_FaceGlobals globals);

FT_Bool
ta_face_globals_is_digit(TA_FaceGlobals globals,
                         FT_UInt gindex);

#endif /* __TAGLOBAL_H__ */

/* end of taglobal.h */
