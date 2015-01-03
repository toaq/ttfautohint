/* taprep.c */

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


#include "ta.h"


#define PREP(snippet_name) prep_ ## snippet_name


const unsigned char PREP(hinting_limit_a) [] =
{

  /* all our measurements are taken along the y axis, */
  /* including the ppem and CVT values */
  SVTCA_y,

  /* first of all, check whether we do hinting at all */
  MPPEM,
  PUSHW_1,

};

/*  %d, hinting size limit */

const unsigned char PREP(hinting_limit_b) [] =
{

  GT,
  IF,
    PUSHB_2,
      1, /* switch off hinting */
      1,
    INSTCTRL,
  EIF,

};

/* we store 0x10000 in CVT index `cvtl_funits_to_pixels' as a scaled value */
/* to have a conversion factor from FUnits to pixels */

const unsigned char PREP(store_funits_to_pixels) [] =
{

  PUSHB_1,
    cvtl_funits_to_pixels,

  PUSHW_2,
    0x08, /* 0x800 */
    0x00,
    0x08, /* 0x800 */
    0x00,
  MUL, /* 0x10000 */

  WCVTF, /* store value 1 in 16.16 format, scaled */

};

/* if the current ppem value is an exception, don't apply scaling */

const unsigned char PREP(test_exception_a) [] =
{

  PUSHB_1,
    cvtl_is_element,
  RCVT,
  NOT,
  IF,

};

/* provide scaling factors for all styles */

const unsigned char PREP(align_top_a) [] =
{

    PUSHB_2,
      sal_i,
      CVT_SCALING_VALUE_OFFSET(0),
    WS,

};

/*  PUSHB (num_used_styles + 2) */
/*    ... */
/*    %c, style 1's x height blue zone idx */
/*    %c, style 0's x height blue zone idx */
/*    %c, num_used_styles */

const unsigned char PREP(align_top_b) [] =
{

      bci_align_top,
    LOOPCALL,

};

const unsigned char PREP(loop_cvt_a) [] =
{

    /* loop over (almost all) vertical CVT entries of all styles, part 1 */
    PUSHB_2,
      sal_i,
      CVT_SCALING_VALUE_OFFSET(0),
    WS,

};

/*  PUSHB (2*num_used_styles + 2) */
/*    ... */
/*    %c, style 1's first vertical index */
/*    %c, style 1's number of vertical indices */
/*        (std. width, widths, flat blues zones without artifical ones) */
/*    %c, style 0's first vertical index */
/*    %c, style 0's number of vertical indices */
/*        (std. width, widths, flat blues zones without artifical ones) */
/*    %c, num_used_styles */

const unsigned char PREP(loop_cvt_b) [] =
{

      bci_cvt_rescale_range,
    LOOPCALL,

    /* loop over (almost all) vertical CVT entries of all styles, part 2 */
    PUSHB_2,
      sal_i,
      CVT_SCALING_VALUE_OFFSET(0),
    WS,

};

/*  PUSHB (2*num_used_styles + 2) */
/*    ... */
/*    %c, style 1's first round blue zone index */
/*    %c, style 1's number of round blue zones (without artificial ones) */
/*    %c, style 0's first round blue zone index */
/*    %c, style 0's number of round blue zones (without artificial ones) */
/*    %c, num_used_styles */

const unsigned char PREP(loop_cvt_c) [] =
{

      bci_cvt_rescale_range,
    LOOPCALL,

};

const unsigned char PREP(test_exception_b) [] =
{

  EIF,

};

const unsigned char PREP(store_vwidth_data_a) [] =
{

  PUSHB_2,
    sal_i,

};

/*  %c, offset to vertical width offset data in CVT */

const unsigned char PREP(store_vwidth_data_b) [] =
{

  WS,

};

/*PUSHW (num_used_styles + 2) */
/*  ... */
/*  %d, style 1's first vertical width index (in multiples of 64) */
/*  %d, style 0's first vertical width index (in multiples of 64) */
/*  %d, num_used_styles */

const unsigned char PREP(store_vwidth_data_c) [] =
{

    0x00, /* high byte */
    bci_vwidth_data_store, /* low byte */
  LOOPCALL,

  PUSHB_2,
    sal_i,

};

/*  %c, offset to vertical width size data in CVT */

const unsigned char PREP(store_vwidth_data_d) [] =
{

  WS,

};

/*PUSHW (num_used_styles + 2) */
/*  ... */
/*  %d, style 1's number of vertical widths (in multiples of 64) */
/*  %d, style 0's number of vertical widths (in multiples of 64) */
/*  %d, num_used_styles */

const unsigned char PREP(store_vwidth_data_e) [] =
{

    0x00, /* high byte */
    bci_vwidth_data_store, /* low byte */
  LOOPCALL,

};

const unsigned char PREP(set_smooth_or_strong_a) [] =
{

  /*
   * There are two ClearType flavours available on Windows: The older GDI
   * ClearType, introduced in 2000, and the recent DW ClearType, introduced
   * in 2008.  The main difference is that the older incarnation behaves
   * like a B/W renderer along the y axis, while the newer version does
   * vertical smoothing also.
   *
   * The only possibility to differentiate between GDI and DW ClearType is
   * testing bit 10 in the GETINFO instruction (with return value in bit 17;
   * this works for TrueType version >= 38), checking whether sub-pixel
   * positioning is available.
   *
   * If GDI ClearType is active, we use different functions for stem width
   * computation and blue zone rounding that snap to integer pixels as much
   * as possible.
   */

  /* set default value */
  PUSHB_2,
    cvtl_use_strong_functions,

};

/*  %c, either 0 or 100 */

const unsigned char PREP(set_smooth_or_strong_b) [] =
{

  WCVTP,

  /* get rasterizer version (bit 0) */
  PUSHB_2,
    36,
    0x01,
  GETINFO,

  /* `GDI ClearType': */
  /* version >= 36 and version < 38, ClearType enabled */
  LTEQ,
  IF,
    /* check whether ClearType is enabled (bit 6) */
    PUSHB_1,
      0x40,
    GETINFO,
    IF,
      PUSHB_2,
        cvtl_use_strong_functions,
};

/*      %c, either 0 or 100 */

const unsigned char PREP(set_smooth_or_strong_c) [] =
{

      WCVTP,

      /* get rasterizer version (bit 0) */
      PUSHB_2,
        38,
        0x01,
      GETINFO,

      /* `DW ClearType': */
      /* version >= 38, sub-pixel positioning is enabled */
      LTEQ,
      IF,
        /* check whether sub-pixel positioning is enabled (bit 10) -- */
        /* due to a bug in FreeType 2.5.0 and earlier, */
        /* bit 6 must be set also to get the correct information, */
        /* so we test that both return values (in bits 13 and 17) are set */
        PUSHW_3,
          0x08, /* bits 13 and 17 shifted by 6 bits */
          0x80,
          0x00, /* we do `MUL' with value 1, */
          0x01, /* which is essentially a division by 64 */
          0x04, /* bits 6 and 10 */
          0x40,
        GETINFO,
        MUL,
        EQ,
        IF,
          PUSHB_2,
            cvtl_use_strong_functions,

};

/*          %c, either 0 or 100 */

const unsigned char PREP(set_smooth_or_strong_d) [] =
{

          WCVTP,
        EIF,
      EIF,
    EIF,
  EIF,

};

/*PUSHB (2*num_used_styles + 2) */
/*  ... */
/*  %c, style 1's first blue ref index */
/*  %c, style 1's number of blue ref indices */
/*  %c, style 0's first blue ref index */
/*  %c, style 0's number of blue ref indices */
/*  %c, num_used_styles */

const unsigned char PREP(round_blues) [] =
{

    bci_blue_round_range,
  LOOPCALL,

};

const unsigned char PREP(set_dropout_mode) [] =
{

  PUSHW_1,
    0x01, /* 0x01FF, activate dropout handling unconditionally */
    0xFF,
  SCANCTRL,
  PUSHB_1,
    4, /* smart dropout include stubs */
  SCANTYPE,

};

const unsigned char PREP(reset_component_counter) [] =
{

  /* In case an application tries to render `.ttfautohint' */
  /* (which it should never do), */
  /* hinting of all glyphs rendered afterwards is disabled */
  /* because the `cvtl_is_subglyph' counter gets incremented, */
  /* but there is no counterpart to decrement it. */
  /* Font inspection tools like the FreeType demo programs */
  /* are an exception to that rule, however, */
  /* since they can directly access a font by glyph indices. */
  /* The following guard alleviates the problem a bit: */
  /* Any change of the graphics state */
  /* (for example, rendering at a different size or with a different mode) */
  /* resets the counter to zero. */
  PUSHB_2,
    cvtl_is_subglyph,
    0,
  WCVTP,

};

const unsigned char PREP(adjust_delta_exceptions) [] =
{

  /* set delta base */
  PUSHB_1,
    CONTROL_DELTA_PPEM_MIN,
  SDB,

};


const unsigned char PREP(do_iup_y) [] =
{

  /* We set a default value for `cvtl_do_iup_y'. */
  /* If we have delta exceptions before IUP_y, */
  /* the glyph's bytecode sets this CVT value temporarily to zero */
  /* and manually inserts IUP_y afterwards. */
  /* It would be more elegant to use a storage area location instead, */
  /* however, it is not possible to have a default value for them */
  /* since storage area locations might be reset on a per-glyph basis */
  /* (this is dependent on the bytecode interpreter implementation). */
  PUSHB_2,
    cvtl_do_iup_y,
    100,
  WCVTP,

};


/* this function allocates `buf', parsing `number_set' to create bytecode */
/* which eventually sets CVT index `cvtl_is_element' */
/* (in functions `bci_number_set_is_element' and */
/* `bci_number_set_is_element2') */

static FT_Byte*
TA_sfnt_build_number_set(SFNT* sfnt,
                         FT_Byte** buf,
                         number_range* number_set)
{
  FT_Byte* bufp = NULL;
  number_range* nr;

  FT_UInt num_singles2 = 0;
  FT_UInt* single2_args;
  FT_UInt* single2_arg;
  FT_UInt num_singles = 0;
  FT_UInt* single_args;
  FT_UInt* single_arg;

  FT_UInt num_ranges2 = 0;
  FT_UInt* range2_args;
  FT_UInt* range2_arg;
  FT_UInt num_ranges = 0;
  FT_UInt* range_args;
  FT_UInt* range_arg;

  FT_UInt have_single = 0;
  FT_UInt have_range = 0;

  FT_UShort num_stack_elements;


  /* build up four stacks to stay as compact as possible */
  nr = number_set;
  while (nr)
  {
    if (nr->start == nr->end)
    {
      if (nr->start < 256)
        num_singles++;
      else
        num_singles2++;
    }
    else
    {
      if (nr->start < 256 && nr->end < 256)
        num_ranges++;
      else
        num_ranges2++;
    }
    nr = nr->next;
  }

  /* collect all arguments temporarily in arrays (in reverse order) */
  /* so that we can easily split into chunks of 255 args */
  /* as needed by NPUSHB and friends; */
  /* for simplicity, always allocate an extra slot */
  single2_args = (FT_UInt*)malloc((num_singles2 + 1) * sizeof (FT_UInt));
  single_args = (FT_UInt*)malloc((num_singles + 1) * sizeof (FT_UInt));
  range2_args = (FT_UInt*)malloc((2 * num_ranges2 + 1) * sizeof (FT_UInt));
  range_args = (FT_UInt*)malloc((2 * num_ranges + 1) * sizeof (FT_UInt));
  if (!single2_args || !single_args
      || !range2_args || !range_args)
    goto Fail;

  /* check whether we need the extra slot for the argument to CALL */
  if (num_singles || num_singles2)
    have_single = 1;
  if (num_ranges || num_ranges2)
    have_range = 1;

  /* set function indices outside of argument loop (using the extra slot) */
  if (have_single)
    single_args[num_singles] = bci_number_set_is_element;
  if (have_range)
    range_args[2 * num_ranges] = bci_number_set_is_element2;

  single2_arg = single2_args + num_singles2 - 1;
  single_arg = single_args + num_singles - 1;
  range2_arg = range2_args + 2 * num_ranges2 - 1;
  range_arg = range_args + 2 * num_ranges - 1;

  nr = number_set;
  while (nr)
  {
    if (nr->start == nr->end)
    {
      if (nr->start < 256)
        *(single_arg--) = nr->start;
      else
        *(single2_arg--) = nr->start;
    }
    else
    {
      if (nr->start < 256 && nr->end < 256)
      {
        *(range_arg--) = nr->start;
        *(range_arg--) = nr->end;
      }
      else
      {
        *(range2_arg--) = nr->start;
        *(range2_arg--) = nr->end;
      }
    }
    nr = nr->next;
  }

  /* this rough estimate of the buffer size gets adjusted later on */
  *buf = (FT_Byte*)malloc((2 + 1) * num_singles2
                          + (1 + 1) * num_singles
                          + (4 + 1) * num_ranges2
                          + (2 + 1) * num_ranges
                          + 10);
  if (!*buf)
    goto Fail;
  bufp = *buf;

  BCI(PUSHB_2);
    BCI(cvtl_is_element);
    BCI(0);
  BCI(WCVTP);

  bufp = TA_build_push(bufp, single2_args, num_singles2, 1, 1);
  bufp = TA_build_push(bufp, single_args, num_singles + have_single, 0, 1);
  if (have_single)
    BCI(CALL);

  bufp = TA_build_push(bufp, range2_args, 2 * num_ranges2, 1, 1);
  bufp = TA_build_push(bufp, range_args, 2 * num_ranges + have_range, 0, 1);
  if (have_range)
    BCI(CALL);

  num_stack_elements = num_singles + num_singles2;
  if (num_stack_elements > num_ranges + num_ranges2)
    num_stack_elements = num_ranges + num_ranges2;
  num_stack_elements += ADDITIONAL_STACK_ELEMENTS;
  if (num_stack_elements > sfnt->max_stack_elements)
    sfnt->max_stack_elements = num_stack_elements;

Fail:
  free(single2_args);
  free(single_args);
  free(range2_args);
  free(range_args);

  return bufp;
}


#define COPY_PREP(snippet_name) \
          do \
          { \
            memcpy(bufp, prep_ ## snippet_name, \
                   sizeof (prep_ ## snippet_name)); \
            bufp += sizeof (prep_ ## snippet_name); \
          } while (0)

static FT_Error
TA_table_build_prep(FT_Byte** prep,
                    FT_ULong* prep_len,
                    SFNT* sfnt,
                    FONT* font)
{
  SFNT_Table* glyf_table = &font->tables[sfnt->glyf_idx];
  glyf_Data* data = (glyf_Data*)glyf_table->data;

  FT_Int i;

  FT_Byte* buf = NULL;
  FT_Byte* buf_new;
  FT_UInt buf_len;
  FT_UInt buf_new_len;

  FT_UInt len;
  FT_Byte* bufp = NULL;


  if (font->x_height_snapping_exceptions)
  {
    bufp = TA_sfnt_build_number_set(sfnt, &buf,
                                    font->x_height_snapping_exceptions);
    if (!bufp)
      return FT_Err_Out_Of_Memory;
  }

  buf_len = bufp - buf;
  buf_new_len = buf_len;

  if (font->hinting_limit)
    buf_new_len += sizeof (PREP(hinting_limit_a))
                   + 2
                   + sizeof (PREP(hinting_limit_b));

  buf_new_len += sizeof (PREP(store_funits_to_pixels));

  if (font->x_height_snapping_exceptions)
    buf_new_len += sizeof (PREP(test_exception_a));

  buf_new_len += sizeof (PREP(align_top_a))
                 + (data->num_used_styles > 6
                      ? data->num_used_styles + 3
                      : data->num_used_styles + 2)
                 + sizeof (PREP(align_top_b));
  buf_new_len += sizeof (PREP(loop_cvt_a))
                 + (data->num_used_styles > 3
                     ? 2 * data->num_used_styles + 3
                     : 2 * data->num_used_styles + 2)
                 + sizeof (PREP(loop_cvt_b))
                 + (data->num_used_styles > 3
                     ? 2 * data->num_used_styles + 3
                     : 2 * data->num_used_styles + 2)
                 + sizeof (PREP(loop_cvt_c));

  if (font->x_height_snapping_exceptions)
    buf_new_len += sizeof (PREP(test_exception_b));

  buf_new_len += sizeof (PREP(store_vwidth_data_a))
                 + 1
                 + sizeof (PREP(store_vwidth_data_b))
                 + (data->num_used_styles > 6
                      ? 2 * (data->num_used_styles + 1) + 2
                      : 2 * (data->num_used_styles + 1) + 1)
                 + sizeof (PREP(store_vwidth_data_c))
                 + 1
                 + sizeof (PREP(store_vwidth_data_d))
                 + (data->num_used_styles > 6
                      ? 2 * (data->num_used_styles + 1) + 2
                      : 2 * (data->num_used_styles + 1) + 1)
                 + sizeof (PREP(store_vwidth_data_e));
  buf_new_len += sizeof (PREP(set_smooth_or_strong_a))
                 + 1
                 + sizeof (PREP(set_smooth_or_strong_b))
                 + 1
                 + sizeof (PREP(set_smooth_or_strong_c))
                 + 1
                 + sizeof (PREP(set_smooth_or_strong_d));
  buf_new_len += (data->num_used_styles > 3
                     ? 2 * data->num_used_styles + 3
                     : 2 * data->num_used_styles + 2)
                 + sizeof (PREP(round_blues));
  buf_new_len += sizeof (PREP(set_dropout_mode));
  buf_new_len += sizeof (PREP(reset_component_counter));
  if (font->control_data_head)
    buf_new_len += sizeof (PREP(adjust_delta_exceptions));
  buf_new_len += sizeof (PREP(do_iup_y));

  /* buffer length must be a multiple of four */
  len = (buf_new_len + 3) & ~3;
  buf_new = (FT_Byte*)realloc(buf, len);
  if (!buf_new)
  {
    free(buf);
    return FT_Err_Out_Of_Memory;
  }
  buf = buf_new;

  /* pad end of buffer with zeros */
  buf[len - 1] = 0x00;
  buf[len - 2] = 0x00;
  buf[len - 3] = 0x00;

  /* copy remaining cvt program into buffer */
  /* and fill in the missing variables */
  bufp = buf + buf_len;

  if (font->hinting_limit)
  {
    COPY_PREP(hinting_limit_a);
    *(bufp++) = HIGH(font->hinting_limit);
    *(bufp++) = LOW(font->hinting_limit);
    COPY_PREP(hinting_limit_b);
  }

  COPY_PREP(store_funits_to_pixels);

  if (font->x_height_snapping_exceptions)
    COPY_PREP(test_exception_a);

  COPY_PREP(align_top_a);
  if (data->num_used_styles > 6)
  {
    BCI(NPUSHB);
    BCI(data->num_used_styles + 2);
  }
  else
    BCI(PUSHB_1 - 1 + data->num_used_styles + 2);
  /* XXX: make this work for offsets > 255 */
  for (i = TA_STYLE_MAX - 1; i >= 0; i--)
  {
    if (data->style_ids[i] == 0xFFFFU)
      continue;

    *(bufp++) = CVT_X_HEIGHT_BLUE_OFFSET(i) >= 0xFFFFU
                  ? 0
                  : (unsigned char)CVT_X_HEIGHT_BLUE_OFFSET(i);
  }
  *(bufp++) = data->num_used_styles;
  COPY_PREP(align_top_b);

  COPY_PREP(loop_cvt_a);
  if (data->num_used_styles > 3)
  {
    BCI(NPUSHB);
    BCI(2 * data->num_used_styles + 2);
  }
  else
    BCI(PUSHB_1 - 1 + 2 * data->num_used_styles + 2);
  /* XXX: make this work for offsets > 255 */
  for (i = TA_STYLE_MAX - 1; i >= 0; i--)
  {
    if (data->style_ids[i] == 0xFFFFU)
      continue;

    /* don't loop over artificial blue zones */
    *(bufp++) = (unsigned char)CVT_VERT_STANDARD_WIDTH_OFFSET(i);
    *(bufp++) = (unsigned char)(
                  1
                  + CVT_VERT_WIDTHS_SIZE(i)
                  + (CVT_BLUES_SIZE(i) > 1 ? CVT_BLUES_SIZE(i) - 2 : 0));
  }
  *(bufp++) = data->num_used_styles;
  COPY_PREP(loop_cvt_b);
  if (data->num_used_styles > 3)
  {
    BCI(NPUSHB);
    BCI(2 * data->num_used_styles + 2);
  }
  else
    BCI(PUSHB_1 - 1 + 2 * data->num_used_styles + 2);
  /* XXX: make this work for offsets > 255 */
  for (i = TA_STYLE_MAX - 1; i >= 0; i--)
  {
    if (data->style_ids[i] == 0xFFFFU)
      continue;

    /* don't loop over artificial blue zones */
    *(bufp++) = (unsigned char)CVT_BLUE_SHOOTS_OFFSET(i);
    *(bufp++) = (unsigned char)(
                  CVT_BLUES_SIZE(i) > 1 ? CVT_BLUES_SIZE(i) - 2 : 0);
  }
  *(bufp++) = data->num_used_styles;
  COPY_PREP(loop_cvt_c);

  if (font->x_height_snapping_exceptions)
    COPY_PREP(test_exception_b);

  COPY_PREP(store_vwidth_data_a);
  *(bufp++) = (unsigned char)CVT_VWIDTH_OFFSET_DATA(0);
  COPY_PREP(store_vwidth_data_b);
  if (data->num_used_styles > 6)
  {
    BCI(NPUSHW);
    BCI(data->num_used_styles + 2);
  }
  else
    BCI(PUSHW_1 - 1 + data->num_used_styles + 2);
  for (i = TA_STYLE_MAX - 1; i >= 0; i--)
  {
    if (data->style_ids[i] == 0xFFFFU)
      continue;

    *(bufp++) = HIGH(CVT_VERT_WIDTHS_OFFSET(i) * 64);
    *(bufp++) = LOW(CVT_VERT_WIDTHS_OFFSET(i) * 64);
  }
  *(bufp++) = HIGH(data->num_used_styles);
  *(bufp++) = LOW(data->num_used_styles);
  COPY_PREP(store_vwidth_data_c);
  *(bufp++) = (unsigned char)CVT_VWIDTH_SIZE_DATA(0);
  COPY_PREP(store_vwidth_data_d);
  if (data->num_used_styles > 6)
  {
    BCI(NPUSHW);
    BCI(data->num_used_styles + 2);
  }
  else
    BCI(PUSHW_1 - 1 + data->num_used_styles + 2);
  for (i = TA_STYLE_MAX - 1; i >= 0; i--)
  {
    if (data->style_ids[i] == 0xFFFFU)
      continue;

    *(bufp++) = HIGH(CVT_VERT_WIDTHS_SIZE(i) * 64);
    *(bufp++) = LOW(CVT_VERT_WIDTHS_SIZE(i) * 64);
  }
  *(bufp++) = HIGH(data->num_used_styles);
  *(bufp++) = LOW(data->num_used_styles);
  COPY_PREP(store_vwidth_data_e);

  COPY_PREP(set_smooth_or_strong_a);
  *(bufp++) = font->gray_strong_stem_width ? 100 : 0;
  COPY_PREP(set_smooth_or_strong_b);
  *(bufp++) = font->gdi_cleartype_strong_stem_width ? 100 : 0;
  COPY_PREP(set_smooth_or_strong_c);
  *(bufp++) = font->dw_cleartype_strong_stem_width ? 100 : 0;
  COPY_PREP(set_smooth_or_strong_d);

  if (data->num_used_styles > 3)
  {
    BCI(NPUSHB);
    BCI(2 * data->num_used_styles + 2);
  }
  else
    BCI(PUSHB_1 - 1 + 2 * data->num_used_styles + 2);
  /* XXX: make this work for offsets > 255 */
  for (i = TA_STYLE_MAX - 1; i >= 0; i--)
  {
    if (data->style_ids[i] == 0xFFFFU)
      continue;

    *(bufp++) = (unsigned char)CVT_BLUE_REFS_OFFSET(i);
    *(bufp++) = (unsigned char)CVT_BLUES_SIZE(i);
  }
  *(bufp++) = data->num_used_styles;
  COPY_PREP(round_blues);

  COPY_PREP(set_dropout_mode);
  COPY_PREP(reset_component_counter);
  if (font->control_data_head)
    COPY_PREP(adjust_delta_exceptions);
  COPY_PREP(do_iup_y);

  *prep = buf;
  *prep_len = buf_new_len;

  return FT_Err_Ok;
}


FT_Error
TA_sfnt_build_prep_table(SFNT* sfnt,
                         FONT* font)
{
  FT_Error error;

  SFNT_Table* glyf_table = &font->tables[sfnt->glyf_idx];
  glyf_Data* data = (glyf_Data*)glyf_table->data;

  FT_Byte* prep_buf;
  FT_ULong prep_len;


  error = TA_sfnt_add_table_info(sfnt);
  if (error)
    goto Exit;

  /* `glyf', `cvt', `fpgm', and `prep' are always used in parallel */
  if (glyf_table->processed)
  {
    sfnt->table_infos[sfnt->num_table_infos - 1] = data->prep_idx;
    goto Exit;
  }

  error = TA_table_build_prep(&prep_buf, &prep_len, sfnt, font);
  if (error)
    goto Exit;

#if 0
  /* ttfautohint's bytecode in `fpgm' is larger */
  /* than the bytecode in `prep'; */
  /* this commented out code here is just for completeness */
  if (prep_len > sfnt->max_instructions)
    sfnt->max_instructions = prep_len;
#endif

  /* in case of success, `prep_buf' gets linked */
  /* and is eventually freed in `TA_font_unload' */
  error = TA_font_add_table(font,
                            &sfnt->table_infos[sfnt->num_table_infos - 1],
                            TTAG_prep, prep_len, prep_buf);
  if (error)
    free(prep_buf);
  else
    data->prep_idx = sfnt->table_infos[sfnt->num_table_infos - 1];

Exit:
  return error;
}

/* end of taprep.c */
