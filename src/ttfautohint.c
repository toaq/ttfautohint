/* ttfautohint.c */

/* written 2011 by Werner Lemberg <wl@gnu.org> */

/* This file needs FreeType 2.4.5 or newer. */


#include <config.h>
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TAGS_H

#include "ttfautohint.h"


typedef struct SFNT_Table_ {
  FT_ULong tag;
  FT_ULong len;
  FT_Byte* buf;
} SFNT_Table;

typedef struct SFNT_ {
  FT_Face face;
  SFNT_Table* table_infos;
  FT_ULong num_table_infos;
} SFNT;


TA_Error
TTF_autohint(FILE *in,
             FILE *out)
{
  FT_Library lib = NULL;
  FT_Face face = NULL;
  FT_Long num_faces = 0;

  FT_Byte* in_buf;
  size_t in_len;

  SFNT sfnt;

  SFNT_Table* SFNT_Tables = NULL;
  FT_ULong num_tables = 0;

  FT_ULong glyf_idx;

  FT_Error error;

  FT_ULong j;


  /*** load font into memory ***/

  fseek(in, 0, SEEK_END);
  in_len = ftell(in);
  fseek(in, 0, SEEK_SET);

  /* a TTF can never be that small */
  if (in_len < 100)
    return FT_Err_Invalid_Argument;

  in_buf = (FT_Byte*)malloc(in_len);
  if (!in_buf)
    return FT_Err_Out_Of_Memory;

  if (fread(in_buf, 1, in_len, in) != in_len)
  {
    error = FT_Err_Invalid_Stream_Read;
    goto Err;
  }

  error = FT_Init_FreeType(&lib);
  if (error)
    goto Err;

  error = FT_New_Memory_Face(lib, in_buf, in_len, -1, &face);
  if (error)
    goto Err;
  num_faces = face->num_faces;
  FT_Done_Face(face);

  error = FT_New_Memory_Face(lib, in_buf, in_len, 0, &face);
  if (error)
    goto Err;

  /* check that font is TTF */
  if (!FT_IS_SFNT(face))
  {
    error = FT_Err_Invalid_Argument;
    goto Err;
  }

  error = FT_Sfnt_Table_Info(face, 0, NULL, &sfnt.num_table_infos);
  if (error)
    goto Err;

  sfnt.table_infos =
    (SFNT_Table*)calloc(1, sfnt.num_table_infos * sizeof (SFNT_Table));
  if (!sfnt.table_infos)
  {
    error = FT_Err_Out_Of_Memory;
    goto Err;
  }

  /* collect SFNT table data */
  glyf_idx = sfnt.num_table_infos;
  for (j = 0; j < sfnt.num_table_infos; j++)
  {
    FT_ULong tag, len;


    error = FT_Sfnt_Table_Info(face, j, &tag, &len);
    if (error && error != FT_Err_Table_Missing)
      goto Err;

    if (!error)
    {
      if (tag == TTAG_glyf)
        glyf_idx = j;

      /* ignore tables which we are going to create by ourselves */
      if (!(tag == TTAG_fpgm
            || tag == TTAG_prep
            || tag == TTAG_cvt))
      {
        sfnt.table_infos[j].tag = tag;
        sfnt.table_infos[j].len = len;
      }
    }
  }

  /* no (non-empty) `glyf' table; this can't be a TTF with outlines */
  if (glyf_idx == sfnt.num_table_infos)
  {
    error = FT_Err_Invalid_Argument;
    goto Err;
  }


  /*** split font into SFNT tables ***/

  {
    SFNT_Table* sti_p = sfnt.table_infos;


    for (j = 0; j < sfnt.num_table_infos; j++)
    {
      if (sti_p->len)
      {
        SFNT_Table* st_new;
        SFNT_Table* st_p;


        /* add one element to table array */
        num_tables++;
        st_new = (SFNT_Table*)realloc(SFNT_Tables,
                                      num_tables * sizeof (SFNT_Table));
        if (!st_new)
        {
          error = FT_Err_Out_Of_Memory;
          goto Err;
        }
        else
          SFNT_Tables = st_new;

        st_p = &SFNT_Tables[num_tables - 1];

        st_p->tag = sti_p->tag;
        st_p->len = sti_p->len;
        st_p->buf = (FT_Byte*)malloc(st_p->len);
        if (!st_p->buf)
        {
          error = FT_Err_Out_Of_Memory;
          goto Err;
        }

        /* link buffer pointer */
        sti_p->buf = st_p->buf;

        error = FT_Load_Sfnt_Table(face, st_p->tag, 0,
                                   st_p->buf, &st_p->len);
        if (error)
          goto Err;
      }

      sti_p++;
    }
  }

  /* compute global hints */
  /* construct `fpgm' table */
  /* construct `prep' table */
  /* construct `cvt ' table */

  /* split `glyf' table */
  /* handle all glyphs in a loop */
    /* strip bytecode */
    /* hint the glyph */
    /* construct bytecode */

  /* construct `glyf' table */
  /* build font from SFNT tables */
  /* write font from memory */

  error = TA_Err_Ok;

Err:
  /* in case of error it is expected that the unallocated pointers */
  /* are NULL (and counters are zero) */
  if (SFNT_Tables)
  {
    for (j = 0; j < num_tables; j++)
      free(SFNT_Tables[j].buf);
    free(SFNT_Tables);
  }
  free(sfnt.table_infos);
  FT_Done_Face(face);
  FT_Done_FreeType(lib);
  free(in_buf);

  return error;
}

/* end of ttfautohint.c */
