/* taloader.c */

/*
 * Copyright (C) 2011-2022 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* originally file `afloader.c' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#include <config.h>
#include <string.h>

#include <ft2build.h>
#include FT_GLYPH_H

#include "ta.h"
#include "tahints.h"
#include "taglobal.h"


/* from file `ftobjs.h' (2011-Mar-28) from FreeType */
#if 0
typedef struct FT_Slot_InternalRec_
{
  TA_GlyphLoader loader;
  FT_UInt flags;
  FT_Bool glyph_transformed;
  FT_Matrix glyph_matrix;
  FT_Vector glyph_delta;
  void* glyph_hints;
} FT_GlyphSlot_InternalRec;
#endif


/* initialize glyph loader */

FT_Error
ta_loader_init(FONT* font)
{
  TA_Loader loader = font->loader;


  memset(loader, 0, sizeof (TA_LoaderRec));

  ta_glyph_hints_init(&loader->hints);
#ifdef TA_DEBUG
  _ta_debug_hints = &loader->hints;
#endif
  return TA_GlyphLoader_New(&loader->gloader);
}


/* reset glyph loader and compute globals if necessary */

FT_Error
ta_loader_reset(FONT* font,
                FT_Face face)
{
  FT_Error error = FT_Err_Ok;
  TA_Loader loader = font->loader;


  loader->face = face;
  loader->globals = (TA_FaceGlobals)face->autohint.data;

  TA_GlyphLoader_Rewind(loader->gloader);

  if (!loader->globals)
  {
    error = ta_face_globals_new(face, &loader->globals, font);
    if (!error)
    {
      face->autohint.data = (FT_Pointer)loader->globals;
      face->autohint.finalizer = (FT_Generic_Finalizer)ta_face_globals_free;
    }
  }

  return error;
}


/* finalize glyph loader */

void
ta_loader_done(FONT* font)
{
  TA_Loader loader = font->loader;


  ta_glyph_hints_done(&loader->hints);

  loader->face = NULL;
  loader->globals = NULL;

#ifdef TA_DEBUG
  _ta_debug_hints = NULL;
#endif
  TA_GlyphLoader_Done(loader->gloader);
  loader->gloader = NULL;
}


/* load a single glyph component; this routine calls itself recursively, */
/* if necessary, and does the main work of `ta_loader_load_glyph' */

static FT_Error
ta_loader_load_g(TA_Loader loader,
                 TA_Scaler scaler,
                 FT_UInt glyph_index,
                 FT_Int32 load_flags,
                 FT_UInt depth)
{
  FT_Error error;
  FT_Face face = loader->face;
  TA_GlyphLoader gloader = loader->gloader;
  TA_StyleMetrics metrics = loader->metrics;
  TA_GlyphHints hints = &loader->hints;
  FT_GlyphSlot slot = face->glyph;
#if 0
  FT_Slot_Internal slot_internal = slot->internal;
#endif
  FT_Int32 flags;


  flags = load_flags | FT_LOAD_LINEAR_DESIGN;
  error = FT_Load_Glyph(face, glyph_index, flags);
  if (error)
    goto Exit;

#if 0
  loader->transformed = slot_internal->glyph_transformed;
  if (loader->transformed)
  {
    FT_Matrix inverse;


    loader->trans_matrix = slot_internal->glyph_matrix;
    loader->trans_delta = slot_internal->glyph_delta;

    inverse = loader->trans_matrix;
    if (!FT_Matrix_Invert(&inverse))
      FT_Vector_Transform(&loader->trans_delta, &inverse);
  }
#endif

  switch (slot->format)
  {
  case FT_GLYPH_FORMAT_OUTLINE:
    /* translate the loaded glyph when an internal transform is needed */
    if (loader->transformed)
      FT_Outline_Translate(&slot->outline,
                           loader->trans_delta.x,
                           loader->trans_delta.y);

    /* copy the outline points in the loader's current extra points */
    /* which are used to keep original glyph coordinates */
    error = TA_GLYPHLOADER_CHECK_POINTS(gloader,
                                        slot->outline.n_points + 4,
                                        slot->outline.n_contours);
    if (error)
      goto Exit;

    memcpy(gloader->current.outline.points,
           slot->outline.points,
           (size_t)slot->outline.n_points * sizeof (FT_Vector));
    memcpy(gloader->current.outline.contours,
           slot->outline.contours,
           (size_t)slot->outline.n_contours * sizeof (short));
    memcpy(gloader->current.outline.tags,
           slot->outline.tags,
           (size_t)slot->outline.n_points * sizeof (char));

    gloader->current.outline.n_points = slot->outline.n_points;
    gloader->current.outline.n_contours = slot->outline.n_contours;

    /* compute original horizontal phantom points */
    /* (and ignore vertical ones) */
    loader->pp1.x = hints->x_delta;
    loader->pp1.y = hints->y_delta;
    loader->pp2.x = FT_MulFix(slot->metrics.horiAdvance,
                              hints->x_scale) + hints->x_delta;
    loader->pp2.y = hints->y_delta;

    /* be sure to check for spacing glyphs */
    if (slot->outline.n_points == 0)
      goto Hint_Metrics;

    /* now load the slot image into the auto-outline */
    /* and run the automatic hinting process */
    {
      TA_StyleClass style_class = metrics->style_class;
      TA_WritingSystemClass writing_system_class =
        ta_writing_system_classes[style_class->writing_system];


      if (writing_system_class->style_hints_apply)
        writing_system_class->style_hints_apply(glyph_index,
                                                hints,
                                                &gloader->current.outline,
                                                metrics);
    }

    /* we now need to adjust the metrics according to the change in */
    /* width/positioning that occurred during the hinting process */
    if (scaler->render_mode != FT_RENDER_MODE_LIGHT)
    {
      TA_AxisHints axis = &hints->axis[TA_DIMENSION_HORZ];


      if (axis->num_edges > 1 && TA_HINTS_DO_ADVANCE(hints))
      {
        TA_Edge edge1 = axis->edges; /* leftmost edge */
        TA_Edge edge2 = edge1 +
                        axis->num_edges - 1; /* rightmost edge */

        FT_Pos old_rsb = loader->pp2.x - edge2->opos;
        /* loader->pp1.x is always zero at this point of time */
        FT_Pos old_lsb = edge1->opos /* - loader->pp1.x */;
        FT_Pos new_lsb = edge1->pos;

        /* remember unhinted values to later account */
        /* for rounding errors */
        FT_Pos pp1x_uh = new_lsb - old_lsb;
        FT_Pos pp2x_uh = edge2->pos + old_rsb;


        /* prefer too much space over too little space */
        /* for very small sizes */

        if (old_lsb < 24)
          pp1x_uh -= 8;
        if (old_rsb < 24)
          pp2x_uh += 8;

        loader->pp1.x = TA_PIX_ROUND(pp1x_uh);
        loader->pp2.x = TA_PIX_ROUND(pp2x_uh);

        if (loader->pp1.x >= new_lsb
            && old_lsb > 0)
          loader->pp1.x -= 64;
        if (loader->pp2.x <= edge2->pos
            && old_rsb > 0)
          loader->pp2.x += 64;

        slot->lsb_delta = loader->pp1.x - pp1x_uh;
        slot->rsb_delta = loader->pp2.x - pp2x_uh;
      }
      else
      {
        FT_Pos pp1x = loader->pp1.x;
        FT_Pos pp2x = loader->pp2.x;


        loader->pp1.x = TA_PIX_ROUND(pp1x + hints->xmin_delta);
        loader->pp2.x = TA_PIX_ROUND(pp2x + hints->xmax_delta);

        slot->lsb_delta = loader->pp1.x - pp1x;
        slot->rsb_delta = loader->pp2.x - pp2x;
      }
    }
    /* `light' mode uses integer advance widths */
    /* but sets `lsb_delta' and `rsb_delta' */
    else
    {
      FT_Pos pp1x = loader->pp1.x;
      FT_Pos pp2x = loader->pp2.x;


      loader->pp1.x = TA_PIX_ROUND(pp1x);
      loader->pp2.x = TA_PIX_ROUND(pp2x);

      slot->lsb_delta = loader->pp1.x - pp1x;
      slot->rsb_delta = loader->pp2.x - pp2x;
    }

    /* good, we simply add the glyph to our loader's base */
    TA_GlyphLoader_Add(gloader);
    break;

  case FT_GLYPH_FORMAT_COMPOSITE:
    {
      FT_UInt nn, num_subglyphs = slot->num_subglyphs;
      FT_UInt num_base_subgs, start_point;
      TA_SubGlyph subglyph;


      start_point = (FT_UInt)gloader->base.outline.n_points;

      /* first of all, copy the subglyph descriptors in the glyph loader */
      error = TA_GlyphLoader_CheckSubGlyphs(gloader, num_subglyphs);
      if (error)
        goto Exit;

      memcpy(gloader->current.subglyphs,
             slot->subglyphs,
             num_subglyphs * sizeof (TA_SubGlyphRec));

      gloader->current.num_subglyphs = num_subglyphs;

      /* stop processing if list of composites is requested */
      if ( load_flags & FT_LOAD_NO_RECURSE )
        goto Exit;

      num_base_subgs = gloader->base.num_subglyphs;

      /* now read each subglyph independently */
      for (nn = 0; nn < num_subglyphs; nn++)
      {
        FT_Vector pp1, pp2;
        FT_Pos x, y;
        FT_UInt num_points, num_new_points, num_base_points;


        /* gloader.current.subglyphs can change during glyph loading due */
        /* to re-allocation -- we must recompute the current subglyph on */
        /* each iteration */
        subglyph = gloader->base.subglyphs + num_base_subgs + nn;

        pp1 = loader->pp1;
        pp2 = loader->pp2;

        num_base_points = (FT_UInt)gloader->base.outline.n_points;

        error = ta_loader_load_g(loader, scaler, (FT_UInt)subglyph->index,
                                 load_flags, depth + 1);
        if (error)
          goto Exit;

        /* recompute subglyph pointer */
        subglyph = gloader->base.subglyphs + num_base_subgs + nn;

        if (!(subglyph->flags & FT_SUBGLYPH_FLAG_USE_MY_METRICS))
        {
          loader->pp1 = pp1;
          loader->pp2 = pp2;
        }

        num_points = (FT_UInt)gloader->base.outline.n_points;
        num_new_points = num_points - num_base_points;

        /* now perform the transformation required for this subglyph */
        if (subglyph->flags & (FT_SUBGLYPH_FLAG_SCALE
                               | FT_SUBGLYPH_FLAG_XY_SCALE
                               | FT_SUBGLYPH_FLAG_2X2))
        {
          FT_Vector* cur = gloader->base.outline.points + num_base_points;
          FT_Vector* limit = cur + num_new_points;


          for (; cur < limit; cur++)
            FT_Vector_Transform(cur, &subglyph->transform);
        }

        /* apply offset */
        if (!(subglyph->flags & FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES))
        {
          FT_UInt k = (FT_UInt)subglyph->arg1;
          FT_UInt l = (FT_UInt)subglyph->arg2;
          FT_Vector* p1;
          FT_Vector* p2;


          if (start_point + k >= num_base_points
              || l >= num_new_points)
          {
            error = FT_Err_Invalid_Composite;
            goto Exit;
          }

          l += num_base_points;

          /* for now, only use the current point coordinates; */
          /* we eventually may consider another approach */
          p1 = gloader->base.outline.points + start_point + k;
          p2 = gloader->base.outline.points + start_point + l;

          x = p1->x - p2->x;
          y = p1->y - p2->y;
        }
        else
        {
          x = FT_MulFix(subglyph->arg1, hints->x_scale) + hints->x_delta;
          y = FT_MulFix(subglyph->arg2, hints->y_scale) + hints->y_delta;

          x = TA_PIX_ROUND(x);
          y = TA_PIX_ROUND(y);
        }

        {
          FT_Outline dummy = gloader->base.outline;


          dummy.points += num_base_points;
          dummy.n_points = (short)num_new_points;

          FT_Outline_Translate(&dummy, x, y);
        }
      }
    }
    break;

  default:
    /* we don't support other formats (yet?) */
    error = FT_Err_Unimplemented_Feature;
  }

Hint_Metrics:
  if (depth == 0)
  {
    FT_BBox bbox;
    FT_Vector vvector;


    vvector.x = slot->metrics.vertBearingX - slot->metrics.horiBearingX;
    vvector.y = slot->metrics.vertBearingY - slot->metrics.horiBearingY;
    vvector.x = FT_MulFix(vvector.x, metrics->scaler.x_scale);
    vvector.y = FT_MulFix(vvector.y, metrics->scaler.y_scale);

    /* transform the hinted outline if needed */
    if (loader->transformed)
    {
      FT_Outline_Transform(&gloader->base.outline, &loader->trans_matrix);
      FT_Vector_Transform(&vvector, &loader->trans_matrix);
    }
#if 1
    /* we must translate our final outline by -pp1.x */
    /* and compute the new metrics */
    if (loader->pp1.x)
      FT_Outline_Translate(&gloader->base.outline, -loader->pp1.x, 0);
#endif
    FT_Outline_Get_CBox(&gloader->base.outline, &bbox);

    bbox.xMin = TA_PIX_FLOOR(bbox.xMin);
    bbox.yMin = TA_PIX_FLOOR(bbox.yMin);
    bbox.xMax = TA_PIX_CEIL(bbox.xMax);
    bbox.yMax = TA_PIX_CEIL(bbox.yMax);

    slot->metrics.width = bbox.xMax - bbox.xMin;
    slot->metrics.height = bbox.yMax - bbox.yMin;
    slot->metrics.horiBearingX = bbox.xMin;
    slot->metrics.horiBearingY = bbox.yMax;

    slot->metrics.vertBearingX = TA_PIX_FLOOR(bbox.xMin + vvector.x);
    slot->metrics.vertBearingY = TA_PIX_FLOOR(bbox.yMax + vvector.y);

    /* for mono-width fonts (like Andale, Courier, etc.) we need */
    /* to keep the original rounded advance width; ditto for */
    /* digits if all have the same advance width */
    if (scaler->render_mode != FT_RENDER_MODE_LIGHT
        && (FT_IS_FIXED_WIDTH(slot->face)
            || (ta_face_globals_is_digit(loader->globals, glyph_index)
                && metrics->digits_have_same_width)))
    {
      slot->metrics.horiAdvance = FT_MulFix(slot->metrics.horiAdvance,
                                            metrics->scaler.x_scale);

      /* set delta values to 0, otherwise code that uses them */
      /* is going to ruin the fixed advance width */
      slot->lsb_delta = 0;
      slot->rsb_delta = 0;
    }
    else
    {
      /* non-spacing glyphs must stay as-is */
      if (slot->metrics.horiAdvance)
        slot->metrics.horiAdvance = loader->pp2.x - loader->pp1.x;
    }

    slot->metrics.vertAdvance = FT_MulFix(slot->metrics.vertAdvance,
                                          metrics->scaler.y_scale);

    slot->metrics.horiAdvance = TA_PIX_ROUND(slot->metrics.horiAdvance);
    slot->metrics.vertAdvance = TA_PIX_ROUND(slot->metrics.vertAdvance);

#if 0
    /* now copy outline into glyph slot */
    TA_GlyphLoader_Rewind(internal->loader);
    error = TA_GlyphLoader_CopyPoints(internal->loader, gloader);
    if (error)
      goto Exit;

    /* reassign all outline fields except flags to protect them */
    slot->outline.n_contours = internal->loader->base.outline.n_contours;
    slot->outline.n_points = internal->loader->base.outline.n_points;
    slot->outline.points = internal->loader->base.outline.points;
    slot->outline.tags = internal->loader->base.outline.tags;
    slot->outline.contours = internal->loader->base.outline.contours;

    slot->format = FT_GLYPH_FORMAT_OUTLINE;
#endif
  }

Exit:
  return error;
}


/* load a glyph */

FT_Error
ta_loader_load_glyph(FONT* font,
                     FT_Face face,
                     FT_UInt gindex,
                     FT_Int32 load_flags)
{
  FT_Error error;
  FT_Size size = face->size;
  TA_Loader loader = font->loader;
  TA_ScalerRec scaler;


  if (!size)
    return FT_Err_Invalid_Size_Handle;

  memset(&scaler, 0, sizeof (TA_ScalerRec));

  scaler.face = face;
  scaler.x_scale = size->metrics.x_scale;
  scaler.x_delta = 0;
  scaler.y_scale = size->metrics.y_scale;
  scaler.y_delta = 0;

  scaler.render_mode = FT_LOAD_TARGET_MODE(load_flags);
  scaler.flags = 0; /* XXX: fix this */

  /* XXX this is an ugly hack of ttfautohint: */
  /* bit 29 triggers vertical hinting only */
  if (load_flags & (1 << 29))
    scaler.flags |= TA_SCALER_FLAG_NO_HORIZONTAL;

  /* note that the fallback style can't be changed anymore */
  /* after the first call of `ta_loader_load_glyph' */
  error = ta_loader_reset(font, face);
  if (!error)
  {
    TA_StyleMetrics metrics;
    FT_UInt options = TA_STYLE_NONE_DFLT;


#ifdef FT_OPTION_AUTOFIT2
    /* XXX: undocumented hook to activate the latin2 hinter */
    if (load_flags & (1UL << 20))
      options = TA_STYLE_LTN2_DFLT;
#endif

    error = ta_face_globals_get_metrics(loader->globals, gindex,
                                        options, &metrics);
    if (!error)
    {
      TA_StyleClass style_class = metrics->style_class;
      TA_WritingSystemClass writing_system_class =
        ta_writing_system_classes[style_class->writing_system];


      loader->metrics = metrics;

      if (writing_system_class->style_metrics_scale)
        writing_system_class->style_metrics_scale(metrics, &scaler);
      else
        metrics->scaler = scaler;

      load_flags |= FT_LOAD_NO_SCALE
                    | FT_LOAD_IGNORE_TRANSFORM;
      load_flags &= ~FT_LOAD_RENDER;

      if (writing_system_class->style_hints_init)
      {
        error = writing_system_class->style_hints_init(&loader->hints,
                                                       metrics);
        if (error)
          goto Exit;
      }

      error = ta_loader_load_g(loader, &scaler, gindex, load_flags, 0);
    }
  }
Exit:
  return error;
}


void
ta_loader_register_hints_recorder(TA_Loader loader,
                                  TA_Hints_Recorder hints_recorder,
                                  void* user)
{
  loader->hints.recorder = hints_recorder;
  loader->hints.user = user;
}

/* end of taloader.c */
