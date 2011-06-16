/* tacvt.c */

/* written 2011 by Werner Lemberg <wl@gnu.org> */

#include "ta.h"


static FT_Error
TA_sfnt_compute_global_hints(SFNT* sfnt,
                             FONT* font)
{
  FT_Error error;
  FT_Face face = sfnt->face;
  FT_UInt enc;
  FT_UInt idx;

  static const FT_Encoding latin_encs[] =
  {
    FT_ENCODING_UNICODE,
    FT_ENCODING_APPLE_ROMAN,
    FT_ENCODING_ADOBE_STANDARD,
    FT_ENCODING_ADOBE_LATIN_1,

    FT_ENCODING_NONE /* end of list */
  };


  error = ta_loader_init(font->loader);
  if (error)
    return error;

  /* try to select a latin charmap */
  for (enc = 0; latin_encs[enc] != FT_ENCODING_NONE; enc++)
  {
    error = FT_Select_Charmap(face, latin_encs[enc]);
    if (!error)
      break;
  }

  /* load latin glyph `a' to trigger all initializations */
  idx = FT_Get_Char_Index(face, 'a');
  error = ta_loader_load_glyph(font->loader, face, idx, 0);

  return error;
}


static FT_Error
TA_table_build_cvt(FT_Byte** cvt,
                   FT_ULong* cvt_len,
                   SFNT* sfnt,
                   FONT* font)
{
  TA_LatinAxis haxis;
  TA_LatinAxis vaxis;

  FT_UInt i;
  FT_UInt buf_len;
  FT_UInt len;
  FT_Byte* buf;
  FT_Byte* buf_p;

  FT_Error error;


  error = TA_sfnt_compute_global_hints(sfnt, font);
  if (error)
    return error;

  haxis = &((TA_LatinMetrics)font->loader->hints.metrics)->axis[0];
  vaxis = &((TA_LatinMetrics)font->loader->hints.metrics)->axis[1];

  buf_len = 2 * (3
                 + haxis->width_count
                 + vaxis->width_count
                 + 2 * vaxis->blue_count);

  /* buffer length must be a multiple of four */
  len = (buf_len + 3) & ~3;
  buf = (FT_Byte*)malloc(len);
  if (!buf)
    return FT_Err_Out_Of_Memory;

  /* pad end of buffer with zeros */
  buf[len - 1] = 0x00;
  buf[len - 2] = 0x00;
  buf[len - 3] = 0x00;

  buf_p = buf;

  /* CVT index 0 is reserved for value 1 in 16.16 format */
  /* (which gets automatically scaled, and this scaling factor we need); */
  /* however, it is too large to be stored directly via the `cvt ' table */
  *(buf_p++) = 0;
  *(buf_p++) = 0;

  if (haxis->width_count > 0)
  {
    *(buf_p++) = HIGH(haxis->widths[0].org);
    *(buf_p++) = LOW(haxis->widths[0].org);
  }
  else
  {
    *(buf_p++) = 0;
    *(buf_p++) = 50;
  }
  if (vaxis->width_count > 0)
  {
    *(buf_p++) = HIGH(vaxis->widths[0].org);
    *(buf_p++) = LOW(vaxis->widths[0].org);
  }
  else
  {
    *(buf_p++) = 0;
    *(buf_p++) = 50;
  }

  for (i = 0; i < haxis->width_count; i++)
  {
    if (haxis->widths[i].org > 0xFFFF)
      goto Err;
    *(buf_p++) = HIGH(haxis->widths[i].org);
    *(buf_p++) = LOW(haxis->widths[i].org);
  }

  for (i = 0; i < vaxis->width_count; i++)
  {
    if (vaxis->widths[i].org > 0xFFFF)
      goto Err;
    *(buf_p++) = HIGH(vaxis->widths[i].org);
    *(buf_p++) = LOW(vaxis->widths[i].org);
  }

  for (i = 0; i < vaxis->blue_count; i++)
  {
    if (vaxis->blues[i].ref.org > 0xFFFF)
      goto Err;
    *(buf_p++) = HIGH(vaxis->blues[i].ref.org);
    *(buf_p++) = LOW(vaxis->blues[i].ref.org);
  }

  for (i = 0; i < vaxis->blue_count; i++)
  {
    if (vaxis->blues[i].shoot.org > 0xFFFF)
      goto Err;
    *(buf_p++) = HIGH(vaxis->blues[i].shoot.org);
    *(buf_p++) = LOW(vaxis->blues[i].shoot.org);
  }

#if 0
  TA_LOG(("--------------------------------------------------\n"));
  TA_LOG(("glyph %d:\n", idx));
  ta_glyph_hints_dump_edges(_ta_debug_hints);
  ta_glyph_hints_dump_segments(_ta_debug_hints);
  ta_glyph_hints_dump_points(_ta_debug_hints);
#endif

  *cvt = buf;
  *cvt_len = buf_len;

  return FT_Err_Ok;

Err:
  free(buf);
  return TA_Err_Hinter_Overflow;
}


FT_Error
TA_sfnt_build_cvt_table(SFNT* sfnt,
                        FONT* font)
{
  FT_Error error;

  FT_Byte* cvt_buf;
  FT_ULong cvt_len;


  error = TA_sfnt_add_table_info(sfnt);
  if (error)
    return error;

  error = TA_table_build_cvt(&cvt_buf, &cvt_len, sfnt, font);
  if (error)
    return error;

  /* in case of success, `cvt_buf' gets linked */
  /* and is eventually freed in `TA_font_unload' */
  error = TA_font_add_table(font,
                            &sfnt->table_infos[sfnt->num_table_infos - 1],
                            TTAG_cvt, cvt_len, cvt_buf);
  if (error)
  {
    free(cvt_buf);
    return error;
  }

  return FT_Err_Ok;
}

/* end of tacvt.c */
