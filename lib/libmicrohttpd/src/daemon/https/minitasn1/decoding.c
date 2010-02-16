/*
 *      Copyright (C) 2004, 2006 Free Software Foundation
 *      Copyright (C) 2002 Fabio Fiorina
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */


/*****************************************************/
/* File: decoding.c                                  */
/* Description: Functions to manage DER decoding     */
/*****************************************************/

#include <int.h>
#include "parser_aux.h"
#include <gstr.h>
#include "structure.h"
#include "element.h"


static void
MHD__asn1_error_description_tag_error (node_asn * node,
                                       char *ErrorDescription)
{

  Estrcpy (ErrorDescription, ":: tag error near element '");
  MHD__asn1_hierarchical_name (node,
                               ErrorDescription + strlen (ErrorDescription),
                               MAX_ERROR_DESCRIPTION_SIZE - 40);
  Estrcat (ErrorDescription, "'");

}

/**
 * MHD__asn1_get_length_der:
 * @der: DER data to decode.
 * @der_len: Length of DER data to decode.
 * @len: Output variable containing the length of the DER length field.
 *
 * Extract a length field from DER data.
 *
 * Return value: Return the decoded length value, or -1 on indefinite
 *   length, or -2 when the value was too big.
 **/
signed long
MHD__asn1_get_length_der (const unsigned char *der, int der_len, int *len)
{
  unsigned long ans;
  int k, punt;

  *len = 0;
  if (der_len <= 0)
    return 0;

  if (!(der[0] & 128))
    {
      /* short form */
      *len = 1;
      return der[0];
    }
  else
    {
      /* Long form */
      k = der[0] & 0x7F;
      punt = 1;
      if (k)
        {                       /* definite length method */
          ans = 0;
          while (punt <= k && punt < der_len)
            {
              unsigned long last = ans;

              ans = ans * 256 + der[punt++];
              if (ans < last)
                /* we wrapped around, no bignum support... */
                return -2;
            }
        }
      else
        {                       /* indefinite length method */
          ans = -1;
        }

      *len = punt;
      return ans;
    }
}




/**
 * MHD__asn1_get_tag_der:
 * @der: DER data to decode.
 * @der_len: Length of DER data to decode.
 * @cls: Output variable containing decoded class.
 * @len: Output variable containing the length of the DER TAG data.
 * @tag: Output variable containing the decoded tag.
 *
 * Decode the class and TAG from DER code.
 *
 * Return value: Returns ASN1_SUCCESS on success, or an error.
 **/
int
MHD__asn1_get_tag_der (const unsigned char *der, int der_len,
                       unsigned char *cls, int *len, unsigned long *tag)
{
  int punt, ris;

  if (der == NULL || der_len <= 0 || len == NULL)
    return ASN1_DER_ERROR;

  *cls = der[0] & 0xE0;
  if ((der[0] & 0x1F) != 0x1F)
    {
      /* short form */
      *len = 1;
      ris = der[0] & 0x1F;
    }
  else
    {
      /* Long form */
      punt = 1;
      ris = 0;
      while (punt <= der_len && der[punt] & 128)
        {
          int last = ris;
          ris = ris * 128 + (der[punt++] & 0x7F);
          if (ris < last)
            /* wrapper around, and no bignums... */
            return ASN1_DER_ERROR;
        }
      if (punt >= der_len)
        return ASN1_DER_ERROR;
      {
        int last = ris;
        ris = ris * 128 + (der[punt++] & 0x7F);
        if (ris < last)
          /* wrapper around, and no bignums... */
          return ASN1_DER_ERROR;
      }
      *len = punt;
    }
  if (tag)
    *tag = ris;
  return ASN1_SUCCESS;
}




/**
 * MHD__asn1_get_octet_der:
 * @der: DER data to decode containing the OCTET SEQUENCE.
 * @der_len: Length of DER data to decode.
 * @ret_len: Output variable containing the length of the DER data.
 * @str: Pre-allocated output buffer to put decoded OCTET SEQUENCE in.
 * @str_size: Length of pre-allocated output buffer.
 * @str_len: Output variable containing the length of the OCTET SEQUENCE.
 *
 * Extract an OCTET SEQUENCE from DER data.
 *
 * Return value: Returns ASN1_SUCCESS on success, or an error.
 **/
int
MHD__asn1_get_octet_der (const unsigned char *der, int der_len,
                         int *ret_len, unsigned char *str, int str_size,
                         int *str_len)
{
  int len_len;

  if (der_len <= 0)
    return ASN1_GENERIC_ERROR;

  /* if(str==NULL) return ASN1_SUCCESS; */
  *str_len = MHD__asn1_get_length_der (der, der_len, &len_len);

  if (*str_len < 0)
    return ASN1_DER_ERROR;

  *ret_len = *str_len + len_len;
  if (str_size >= *str_len)
    memcpy (str, der + len_len, *str_len);
  else
    {
      return ASN1_MEM_ERROR;
    }

  return ASN1_SUCCESS;
}



/* Returns ASN1_SUCCESS on success or an error code on error.
 */
static int
MHD__asn1_get_time_der (const unsigned char *der, int der_len, int *ret_len,
                        char *str, int str_size)
{
  int len_len, str_len;

  if (der_len <= 0 || str == NULL)
    return ASN1_DER_ERROR;
  str_len = MHD__asn1_get_length_der (der, der_len, &len_len);
  if (str_len < 0 || str_size < str_len)
    return ASN1_DER_ERROR;
  memcpy (str, der + len_len, str_len);
  str[str_len] = 0;
  *ret_len = str_len + len_len;

  return ASN1_SUCCESS;
}



static void
MHD__asn1_get_objectid_der (const unsigned char *der, int der_len,
                            int *ret_len, char *str, int str_size)
{
  int len_len, len, k;
  char temp[20];
  unsigned long val, val1;

  *ret_len = 0;
  if (str && str_size > 0)
    str[0] = 0;                 /* no oid */

  if (str == NULL || der_len <= 0)
    return;
  len = MHD__asn1_get_length_der (der, der_len, &len_len);

  if (len < 0 || len > der_len || len_len > der_len)
    return;

  val1 = der[len_len] / 40;
  val = der[len_len] - val1 * 40;

  MHD__asn1_str_cpy (str, str_size, MHD__asn1_ltostr (val1, temp));
  MHD__asn1_str_cat (str, str_size, ".");
  MHD__asn1_str_cat (str, str_size, MHD__asn1_ltostr (val, temp));

  val = 0;
  for (k = 1; k < len; k++)
    {
      val = val << 7;
      val |= der[len_len + k] & 0x7F;
      if (!(der[len_len + k] & 0x80))
        {
          MHD__asn1_str_cat (str, str_size, ".");
          MHD__asn1_str_cat (str, str_size, MHD__asn1_ltostr (val, temp));
          val = 0;
        }
    }
  *ret_len = len + len_len;
}




/**
 * MHD__asn1_get_bit_der:
 * @der: DER data to decode containing the BIT SEQUENCE.
 * @der_len: Length of DER data to decode.
 * @ret_len: Output variable containing the length of the DER data.
 * @str: Pre-allocated output buffer to put decoded BIT SEQUENCE in.
 * @str_size: Length of pre-allocated output buffer.
 * @bit_len: Output variable containing the size of the BIT SEQUENCE.
 *
 * Extract a BIT SEQUENCE from DER data.
 *
 * Return value: Return ASN1_SUCCESS on success, or an error.
 **/
int
MHD__asn1_get_bit_der (const unsigned char *der, int der_len,
                       int *ret_len, unsigned char *str, int str_size,
                       int *bit_len)
{
  int len_len, len_byte;

  if (der_len <= 0)
    return ASN1_GENERIC_ERROR;
  len_byte = MHD__asn1_get_length_der (der, der_len, &len_len) - 1;
  if (len_byte < 0)
    return ASN1_DER_ERROR;

  *ret_len = len_byte + len_len + 1;
  *bit_len = len_byte * 8 - der[len_len];

  if (str_size >= len_byte)
    memcpy (str, der + len_len + 1, len_byte);
  else
    {
      return ASN1_MEM_ERROR;
    }

  return ASN1_SUCCESS;
}




static int
MHD__asn1_extract_tag_der (node_asn * node, const unsigned char *der,
                           int der_len, int *ret_len)
{
  node_asn *p;
  int counter, len2, len3, is_tag_implicit;
  unsigned long tag, tag_implicit = 0;
  unsigned char class, class2, class_implicit = 0;

  if (der_len <= 0)
    return ASN1_GENERIC_ERROR;

  counter = is_tag_implicit = 0;

  if (node->type & CONST_TAG)
    {
      p = node->down;
      while (p)
        {
          if (type_field (p->type) == TYPE_TAG)
            {
              if (p->type & CONST_APPLICATION)
                class2 = ASN1_CLASS_APPLICATION;
              else if (p->type & CONST_UNIVERSAL)
                class2 = ASN1_CLASS_UNIVERSAL;
              else if (p->type & CONST_PRIVATE)
                class2 = ASN1_CLASS_PRIVATE;
              else
                class2 = ASN1_CLASS_CONTEXT_SPECIFIC;

              if (p->type & CONST_EXPLICIT)
                {
                  if (MHD__asn1_get_tag_der
                      (der + counter, der_len - counter, &class, &len2,
                       &tag) != ASN1_SUCCESS)
                    return ASN1_DER_ERROR;
                  if (counter + len2 > der_len)
                    return ASN1_DER_ERROR;
                  counter += len2;
                  len3 =
                    MHD__asn1_get_length_der (der + counter,
                                              der_len - counter, &len2);
                  if (len3 < 0)
                    return ASN1_DER_ERROR;
                  counter += len2;
                  if (!is_tag_implicit)
                    {
                      if ((class != (class2 | ASN1_CLASS_STRUCTURED)) ||
                          (tag != strtoul ((char *) p->value, NULL, 10)))
                        return ASN1_TAG_ERROR;
                    }
                  else
                    {           /* ASN1_TAG_IMPLICIT */
                      if ((class != class_implicit) || (tag != tag_implicit))
                        return ASN1_TAG_ERROR;
                    }

                  is_tag_implicit = 0;
                }
              else
                {               /* ASN1_TAG_IMPLICIT */
                  if (!is_tag_implicit)
                    {
                      if ((type_field (node->type) == TYPE_SEQUENCE) ||
                          (type_field (node->type) == TYPE_SEQUENCE_OF) ||
                          (type_field (node->type) == TYPE_SET) ||
                          (type_field (node->type) == TYPE_SET_OF))
                        class2 |= ASN1_CLASS_STRUCTURED;
                      class_implicit = class2;
                      tag_implicit = strtoul ((char *) p->value, NULL, 10);
                      is_tag_implicit = 1;
                    }
                }
            }
          p = p->right;
        }
    }

  if (is_tag_implicit)
    {
      if (MHD__asn1_get_tag_der
          (der + counter, der_len - counter, &class, &len2,
           &tag) != ASN1_SUCCESS)
        return ASN1_DER_ERROR;
      if (counter + len2 > der_len)
        return ASN1_DER_ERROR;

      if ((class != class_implicit) || (tag != tag_implicit))
        {
          if (type_field (node->type) == TYPE_OCTET_STRING)
            {
              class_implicit |= ASN1_CLASS_STRUCTURED;
              if ((class != class_implicit) || (tag != tag_implicit))
                return ASN1_TAG_ERROR;
            }
          else
            return ASN1_TAG_ERROR;
        }
    }
  else
    {
      if (type_field (node->type) == TYPE_TAG)
        {
          counter = 0;
          *ret_len = counter;
          return ASN1_SUCCESS;
        }

      if (MHD__asn1_get_tag_der
          (der + counter, der_len - counter, &class, &len2,
           &tag) != ASN1_SUCCESS)
        return ASN1_DER_ERROR;
      if (counter + len2 > der_len)
        return ASN1_DER_ERROR;

      switch (type_field (node->type))
        {
        case TYPE_NULL:
          if ((class != ASN1_CLASS_UNIVERSAL) || (tag != ASN1_TAG_NULL))
            return ASN1_DER_ERROR;
          break;
        case TYPE_BOOLEAN:
          if ((class != ASN1_CLASS_UNIVERSAL) || (tag != ASN1_TAG_BOOLEAN))
            return ASN1_DER_ERROR;
          break;
        case TYPE_INTEGER:
          if ((class != ASN1_CLASS_UNIVERSAL) || (tag != ASN1_TAG_INTEGER))
            return ASN1_DER_ERROR;
          break;
        case TYPE_ENUMERATED:
          if ((class != ASN1_CLASS_UNIVERSAL) || (tag != ASN1_TAG_ENUMERATED))
            return ASN1_DER_ERROR;
          break;
        case TYPE_OBJECT_ID:
          if ((class != ASN1_CLASS_UNIVERSAL) || (tag != ASN1_TAG_OBJECT_ID))
            return ASN1_DER_ERROR;
          break;
        case TYPE_TIME:
          if (node->type & CONST_UTC)
            {
              if ((class != ASN1_CLASS_UNIVERSAL)
                  || (tag != ASN1_TAG_UTCTime))
                return ASN1_DER_ERROR;
            }
          else
            {
              if ((class != ASN1_CLASS_UNIVERSAL)
                  || (tag != ASN1_TAG_GENERALIZEDTime))
                return ASN1_DER_ERROR;
            }
          break;
        case TYPE_OCTET_STRING:
          if (((class != ASN1_CLASS_UNIVERSAL)
               && (class != (ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED)))
              || (tag != ASN1_TAG_OCTET_STRING))
            return ASN1_DER_ERROR;
          break;
        case TYPE_GENERALSTRING:
          if ((class != ASN1_CLASS_UNIVERSAL)
              || (tag != ASN1_TAG_GENERALSTRING))
            return ASN1_DER_ERROR;
          break;
        case TYPE_BIT_STRING:
          if ((class != ASN1_CLASS_UNIVERSAL) || (tag != ASN1_TAG_BIT_STRING))
            return ASN1_DER_ERROR;
          break;
        case TYPE_SEQUENCE:
        case TYPE_SEQUENCE_OF:
          if ((class != (ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED))
              || (tag != ASN1_TAG_SEQUENCE))
            return ASN1_DER_ERROR;
          break;
        case TYPE_SET:
        case TYPE_SET_OF:
          if ((class != (ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED))
              || (tag != ASN1_TAG_SET))
            return ASN1_DER_ERROR;
          break;
        case TYPE_ANY:
          counter -= len2;
          break;
        default:
          return ASN1_DER_ERROR;
          break;
        }
    }

  counter += len2;
  *ret_len = counter;
  return ASN1_SUCCESS;
}


static int
MHD__asn1_delete_not_used (node_asn * node)
{
  node_asn *p, *p2;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  while (p)
    {
      if (p->type & CONST_NOT_USED)
        {
          p2 = NULL;
          if (p != node)
            {
              p2 = MHD__asn1_find_left (p);
              if (!p2)
                p2 = MHD__asn1_find_up (p);
            }
          MHD__asn1_delete_structure (&p);
          p = p2;
        }

      if (!p)
        break;                  /* reach node */

      if (p->down)
        {
          p = p->down;
        }
      else
        {
          if (p == node)
            p = NULL;
          else if (p->right)
            p = p->right;
          else
            {
              while (1)
                {
                  p = MHD__asn1_find_up (p);
                  if (p == node)
                    {
                      p = NULL;
                      break;
                    }
                  if (p->right)
                    {
                      p = p->right;
                      break;
                    }
                }
            }
        }
    }
  return ASN1_SUCCESS;
}


static MHD__asn1_retCode
MHD__asn1_get_octet_string (const unsigned char *der, node_asn * node,
                            int *len)
{
  int len2, len3, counter, counter2, counter_end, tot_len, indefinite;
  unsigned char *temp, *temp2;

  counter = 0;

  if (*(der - 1) & ASN1_CLASS_STRUCTURED)
    {
      tot_len = 0;
      indefinite = MHD__asn1_get_length_der (der, *len, &len3);
      if (indefinite < -1)
        return ASN1_DER_ERROR;

      counter += len3;
      if (indefinite >= 0)
        indefinite += len3;

      while (1)
        {
          if (counter > (*len))
            return ASN1_DER_ERROR;

          if (indefinite == -1)
            {
              if ((der[counter] == 0) && (der[counter + 1] == 0))
                {
                  counter += 2;
                  break;
                }
            }
          else if (counter >= indefinite)
            break;

          if (der[counter] != ASN1_TAG_OCTET_STRING)
            return ASN1_DER_ERROR;

          counter++;

          len2 =
            MHD__asn1_get_length_der (der + counter, *len - counter, &len3);
          if (len2 <= 0)
            return ASN1_DER_ERROR;

          counter += len3 + len2;
          tot_len += len2;
        }

      /* copy */
      if (node)
        {
          MHD__asn1_length_der (tot_len, NULL, &len2);
          temp = MHD__asn1_alloca (len2 + tot_len);
          if (temp == NULL)
            {
              return ASN1_MEM_ALLOC_ERROR;
            }

          MHD__asn1_length_der (tot_len, temp, &len2);
          tot_len += len2;
          temp2 = temp + len2;
          len2 = MHD__asn1_get_length_der (der, *len, &len3);
          if (len2 < -1)
            {
              MHD__asn1_afree (temp);
              return ASN1_DER_ERROR;
            }
          counter2 = len3 + 1;

          if (indefinite == -1)
            counter_end = counter - 2;
          else
            counter_end = counter;

          while (counter2 < counter_end)
            {
              len2 =
                MHD__asn1_get_length_der (der + counter2, *len - counter,
                                          &len3);
              if (len2 < -1)
                {
                  MHD__asn1_afree (temp);
                  return ASN1_DER_ERROR;
                }

              /* FIXME: to be checked. Is this ok? Has the
               * size been checked before?
               */
              memcpy (temp2, der + counter2 + len3, len2);
              temp2 += len2;
              counter2 += len2 + len3 + 1;
            }

          MHD__asn1_set_value (node, temp, tot_len);
          MHD__asn1_afree (temp);
        }
    }
  else
    {                           /* NOT STRUCTURED */
      len2 = MHD__asn1_get_length_der (der, *len, &len3);
      if (len2 < 0)
        return ASN1_DER_ERROR;
      if (len3 + len2 > *len)
        return ASN1_DER_ERROR;
      if (node)
        MHD__asn1_set_value (node, der, len3 + len2);
      counter = len3 + len2;
    }

  *len = counter;
  return ASN1_SUCCESS;

}


static MHD__asn1_retCode
MHD__asn1_get_indefinite_length_string (const unsigned char *der, int *len)
{
  int len2, len3, counter, indefinite;
  unsigned long tag;
  unsigned char class;

  counter = indefinite = 0;

  while (1)
    {
      if ((*len) < counter)
        return ASN1_DER_ERROR;

      if ((der[counter] == 0) && (der[counter + 1] == 0))
        {
          counter += 2;
          indefinite--;
          if (indefinite <= 0)
            break;
          else
            continue;
        }

      if (MHD__asn1_get_tag_der
          (der + counter, *len - counter, &class, &len2,
           &tag) != ASN1_SUCCESS)
        return ASN1_DER_ERROR;
      if (counter + len2 > *len)
        return ASN1_DER_ERROR;
      counter += len2;
      len2 = MHD__asn1_get_length_der (der + counter, *len - counter, &len3);
      if (len2 < -1)
        return ASN1_DER_ERROR;
      if (len2 == -1)
        {
          indefinite++;
          counter += 1;
        }
      else
        {
          counter += len2 + len3;
        }
    }

  *len = counter;
  return ASN1_SUCCESS;

}


/**
  * MHD__asn1_der_decoding - Fill the structure *ELEMENT with values of a DER encoding string.
  * @element: pointer to an ASN1 structure.
  * @ider: vector that contains the DER encoding.
  * @len: number of bytes of *@ider: @ider[0]..@ider[len-1].
  * @errorDescription: null-terminated string contains details when an
  *   error occurred.
  *
  * Fill the structure *ELEMENT with values of a DER encoding
  * string. The sructure must just be created with function
  * 'create_stucture'.  If an error occurs during the decoding
  * procedure, the *ELEMENT is deleted and set equal to
  * %ASN1_TYPE_EMPTY.
  *
  * Returns:
  *
  *   ASN1_SUCCESS: DER encoding OK.
  *
  *   ASN1_ELEMENT_NOT_FOUND: ELEMENT is ASN1_TYPE_EMPTY.
  *
  *   ASN1_TAG_ERROR,ASN1_DER_ERROR: The der encoding doesn't match
  *     the structure NAME. *ELEMENT deleted.
  **/

MHD__asn1_retCode
MHD__asn1_der_decoding (ASN1_TYPE * element, const void *ider, int len,
                        char *errorDescription)
{
  node_asn *node, *p, *p2, *p3;
  char temp[128];
  int counter, len2, len3, len4, move, ris, tlen;
  unsigned char class, *temp2;
  unsigned long tag;
  int indefinite, result;
  const unsigned char *der = ider;

  node = *element;

  if (node == ASN1_TYPE_EMPTY)
    return ASN1_ELEMENT_NOT_FOUND;

  if (node->type & CONST_OPTION)
    {
      MHD__asn1_delete_structure (element);
      return ASN1_GENERIC_ERROR;
    }

  counter = 0;
  move = DOWN;
  p = node;
  while (1)
    {
      ris = ASN1_SUCCESS;
      if (move != UP)
        {
          if (p->type & CONST_SET)
            {
              p2 = MHD__asn1_find_up (p);
              len2 = strtol ((const char *) p2->value, NULL, 10);
              if (len2 == -1)
                {
                  if (!der[counter] && !der[counter + 1])
                    {
                      p = p2;
                      move = UP;
                      counter += 2;
                      continue;
                    }
                }
              else if (counter == len2)
                {
                  p = p2;
                  move = UP;
                  continue;
                }
              else if (counter > len2)
                {
                  MHD__asn1_delete_structure (element);
                  return ASN1_DER_ERROR;
                }
              p2 = p2->down;
              while (p2)
                {
                  if ((p2->type & CONST_SET) && (p2->type & CONST_NOT_USED))
                    {
                      if (type_field (p2->type) != TYPE_CHOICE)
                        ris =
                          MHD__asn1_extract_tag_der (p2, der + counter,
                                                     len - counter, &len2);
                      else
                        {
                          p3 = p2->down;
                          while (p3)
                            {
                              ris =
                                MHD__asn1_extract_tag_der (p3, der + counter,
                                                           len - counter,
                                                           &len2);
                              if (ris == ASN1_SUCCESS)
                                break;
                              p3 = p3->right;
                            }
                        }
                      if (ris == ASN1_SUCCESS)
                        {
                          p2->type &= ~CONST_NOT_USED;
                          p = p2;
                          break;
                        }
                    }
                  p2 = p2->right;
                }
              if (p2 == NULL)
                {
                  MHD__asn1_delete_structure (element);
                  return ASN1_DER_ERROR;
                }
            }

          if ((p->type & CONST_OPTION) || (p->type & CONST_DEFAULT))
            {
              p2 = MHD__asn1_find_up (p);
              len2 = strtol ((const char *) p2->value, NULL, 10);
              if (counter == len2)
                {
                  if (p->right)
                    {
                      p2 = p->right;
                      move = RIGHT;
                    }
                  else
                    move = UP;

                  if (p->type & CONST_OPTION)
                    MHD__asn1_delete_structure (&p);

                  p = p2;
                  continue;
                }
            }

          if (type_field (p->type) == TYPE_CHOICE)
            {
              while (p->down)
                {
                  if (counter < len)
                    ris =
                      MHD__asn1_extract_tag_der (p->down, der + counter,
                                                 len - counter, &len2);
                  else
                    ris = ASN1_DER_ERROR;
                  if (ris == ASN1_SUCCESS)
                    {
                      while (p->down->right)
                        {
                          p2 = p->down->right;
                          MHD__asn1_delete_structure (&p2);
                        }
                      break;
                    }
                  else if (ris == ASN1_ERROR_TYPE_ANY)
                    {
                      MHD__asn1_delete_structure (element);
                      return ASN1_ERROR_TYPE_ANY;
                    }
                  else
                    {
                      p2 = p->down;
                      MHD__asn1_delete_structure (&p2);
                    }
                }

              if (p->down == NULL)
                {
                  if (!(p->type & CONST_OPTION))
                    {
                      MHD__asn1_delete_structure (element);
                      return ASN1_DER_ERROR;
                    }
                }
              else
                p = p->down;
            }

          if ((p->type & CONST_OPTION) || (p->type & CONST_DEFAULT))
            {
              p2 = MHD__asn1_find_up (p);
              len2 = strtol ((const char *) p2->value, NULL, 10);
              if ((len2 != -1) && (counter > len2))
                ris = ASN1_TAG_ERROR;
            }

          if (ris == ASN1_SUCCESS)
            ris =
              MHD__asn1_extract_tag_der (p, der + counter, len - counter,
                                         &len2);
          if (ris != ASN1_SUCCESS)
            {
              if (p->type & CONST_OPTION)
                {
                  p->type |= CONST_NOT_USED;
                  move = RIGHT;
                }
              else if (p->type & CONST_DEFAULT)
                {
                  MHD__asn1_set_value (p, NULL, 0);
                  move = RIGHT;
                }
              else
                {
                  if (errorDescription != NULL)
                    MHD__asn1_error_description_tag_error (p,
                                                           errorDescription);

                  MHD__asn1_delete_structure (element);
                  return ASN1_TAG_ERROR;
                }
            }
          else
            counter += len2;
        }

      if (ris == ASN1_SUCCESS)
        {
          switch (type_field (p->type))
            {
            case TYPE_NULL:
              if (der[counter])
                {
                  MHD__asn1_delete_structure (element);
                  return ASN1_DER_ERROR;
                }
              counter++;
              move = RIGHT;
              break;
            case TYPE_BOOLEAN:
              if (der[counter++] != 1)
                {
                  MHD__asn1_delete_structure (element);
                  return ASN1_DER_ERROR;
                }
              if (der[counter++] == 0)
                MHD__asn1_set_value (p, "F", 1);
              else
                MHD__asn1_set_value (p, "T", 1);
              move = RIGHT;
              break;
            case TYPE_INTEGER:
            case TYPE_ENUMERATED:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              if (len2 + len3 > len - counter)
                return ASN1_DER_ERROR;
              MHD__asn1_set_value (p, der + counter, len3 + len2);
              counter += len3 + len2;
              move = RIGHT;
              break;
            case TYPE_OBJECT_ID:
              MHD__asn1_get_objectid_der (der + counter, len - counter, &len2,
                                          temp, sizeof (temp));
              tlen = strlen (temp);
              if (tlen > 0)
                MHD__asn1_set_value (p, temp, tlen + 1);
              counter += len2;
              move = RIGHT;
              break;
            case TYPE_TIME:
              result =
                MHD__asn1_get_time_der (der + counter, len - counter, &len2,
                                        temp, sizeof (temp) - 1);
              if (result != ASN1_SUCCESS)
                {
                  MHD__asn1_delete_structure (element);
                  return result;
                }
              tlen = strlen (temp);
              if (tlen > 0)
                MHD__asn1_set_value (p, temp, tlen + 1);
              counter += len2;
              move = RIGHT;
              break;
            case TYPE_OCTET_STRING:
              len3 = len - counter;
              ris = MHD__asn1_get_octet_string (der + counter, p, &len3);
              if (ris != ASN1_SUCCESS)
                return ris;
              counter += len3;
              move = RIGHT;
              break;
            case TYPE_GENERALSTRING:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              if (len3 + len2 > len - counter)
                return ASN1_DER_ERROR;
              MHD__asn1_set_value (p, der + counter, len3 + len2);
              counter += len3 + len2;
              move = RIGHT;
              break;
            case TYPE_BIT_STRING:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              if (len3 + len2 > len - counter)
                return ASN1_DER_ERROR;
              MHD__asn1_set_value (p, der + counter, len3 + len2);
              counter += len3 + len2;
              move = RIGHT;
              break;
            case TYPE_SEQUENCE:
            case TYPE_SET:
              if (move == UP)
                {
                  len2 = strtol ((const char *) p->value, NULL, 10);
                  MHD__asn1_set_value (p, NULL, 0);
                  if (len2 == -1)
                    {           /* indefinite length method */
                      if (len - counter + 1 > 0)
                        {
                          if ((der[counter]) || der[counter + 1])
                            {
                              MHD__asn1_delete_structure (element);
                              return ASN1_DER_ERROR;
                            }
                        }
                      else
                        return ASN1_DER_ERROR;
                      counter += 2;
                    }
                  else
                    {           /* definite length method */
                      if (len2 != counter)
                        {
                          MHD__asn1_delete_structure (element);
                          return ASN1_DER_ERROR;
                        }
                    }
                  move = RIGHT;
                }
              else
                {               /* move==DOWN || move==RIGHT */
                  len3 =
                    MHD__asn1_get_length_der (der + counter, len - counter,
                                              &len2);
                  if (len3 < -1)
                    return ASN1_DER_ERROR;
                  counter += len2;
                  if (len3 > 0)
                    {
                      MHD__asn1_ltostr (counter + len3, temp);
                      tlen = strlen (temp);
                      if (tlen > 0)
                        MHD__asn1_set_value (p, temp, tlen + 1);
                      move = DOWN;
                    }
                  else if (len3 == 0)
                    {
                      p2 = p->down;
                      while (p2)
                        {
                          if (type_field (p2->type) != TYPE_TAG)
                            {
                              p3 = p2->right;
                              MHD__asn1_delete_structure (&p2);
                              p2 = p3;
                            }
                          else
                            p2 = p2->right;
                        }
                      move = RIGHT;
                    }
                  else
                    {           /* indefinite length method */
                      MHD__asn1_set_value (p, "-1", 3);
                      move = DOWN;
                    }
                }
              break;
            case TYPE_SEQUENCE_OF:
            case TYPE_SET_OF:
              if (move == UP)
                {
                  len2 = strtol ((const char *) p->value, NULL, 10);
                  if (len2 == -1)
                    {           /* indefinite length method */
                      if ((counter + 2) > len)
                        return ASN1_DER_ERROR;
                      if ((der[counter]) || der[counter + 1])
                        {
                          MHD__asn1_append_sequence_set (p);
                          p = p->down;
                          while (p->right)
                            p = p->right;
                          move = RIGHT;
                          continue;
                        }
                      MHD__asn1_set_value (p, NULL, 0);
                      counter += 2;
                    }
                  else
                    {           /* definite length method */
                      if (len2 > counter)
                        {
                          MHD__asn1_append_sequence_set (p);
                          p = p->down;
                          while (p->right)
                            p = p->right;
                          move = RIGHT;
                          continue;
                        }
                      MHD__asn1_set_value (p, NULL, 0);
                      if (len2 != counter)
                        {
                          MHD__asn1_delete_structure (element);
                          return ASN1_DER_ERROR;
                        }
                    }
                }
              else
                {               /* move==DOWN || move==RIGHT */
                  len3 =
                    MHD__asn1_get_length_der (der + counter, len - counter,
                                              &len2);
                  if (len3 < -1)
                    return ASN1_DER_ERROR;
                  counter += len2;
                  if (len3)
                    {
                      if (len3 > 0)
                        {       /* definite length method */
                          MHD__asn1_ltostr (counter + len3, temp);
                          tlen = strlen (temp);

                          if (tlen > 0)
                            MHD__asn1_set_value (p, temp, tlen + 1);
                        }
                      else
                        {       /* indefinite length method */
                          MHD__asn1_set_value (p, "-1", 3);
                        }
                      p2 = p->down;
                      while ((type_field (p2->type) == TYPE_TAG)
                             || (type_field (p2->type) == TYPE_SIZE))
                        p2 = p2->right;
                      if (p2->right == NULL)
                        MHD__asn1_append_sequence_set (p);
                      p = p2;
                    }
                }
              move = RIGHT;
              break;
            case TYPE_ANY:
              if (MHD__asn1_get_tag_der
                  (der + counter, len - counter, &class, &len2,
                   &tag) != ASN1_SUCCESS)
                return ASN1_DER_ERROR;
              if (counter + len2 > len)
                return ASN1_DER_ERROR;
              len4 =
                MHD__asn1_get_length_der (der + counter + len2,
                                          len - counter - len2, &len3);
              if (len4 < -1)
                return ASN1_DER_ERROR;
              if (len4 > len - counter + len2 + len3)
                return ASN1_DER_ERROR;
              if (len4 != -1)
                {
                  len2 += len4;
                  MHD__asn1_length_der (len2 + len3, NULL, &len4);
                  temp2 =
                    (unsigned char *) MHD__asn1_alloca (len2 + len3 + len4);
                  if (temp2 == NULL)
                    {
                      MHD__asn1_delete_structure (element);
                      return ASN1_MEM_ALLOC_ERROR;
                    }

                  MHD__asn1_octet_der (der + counter, len2 + len3, temp2,
                                       &len4);
                  MHD__asn1_set_value (p, temp2, len4);
                  MHD__asn1_afree (temp2);
                  counter += len2 + len3;
                }
              else
                {               /* indefinite length */
                  /* Check indefinite lenth method in an EXPLICIT TAG */
                  if ((p->type & CONST_TAG) && (der[counter - 1] == 0x80))
                    indefinite = 1;
                  else
                    indefinite = 0;

                  len2 = len - counter;
                  ris =
                    MHD__asn1_get_indefinite_length_string (der + counter,
                                                            &len2);
                  if (ris != ASN1_SUCCESS)
                    {
                      MHD__asn1_delete_structure (element);
                      return ris;
                    }
                  MHD__asn1_length_der (len2, NULL, &len4);
                  temp2 = (unsigned char *) MHD__asn1_alloca (len2 + len4);
                  if (temp2 == NULL)
                    {
                      MHD__asn1_delete_structure (element);
                      return ASN1_MEM_ALLOC_ERROR;
                    }

                  MHD__asn1_octet_der (der + counter, len2, temp2, &len4);
                  MHD__asn1_set_value (p, temp2, len4);
                  MHD__asn1_afree (temp2);
                  counter += len2;

                  /* Check if a couple of 0x00 are present due to an EXPLICIT TAG with
                     an indefinite length method. */
                  if (indefinite)
                    {
                      if (!der[counter] && !der[counter + 1])
                        {
                          counter += 2;
                        }
                      else
                        {
                          MHD__asn1_delete_structure (element);
                          return ASN1_DER_ERROR;
                        }
                    }
                }
              move = RIGHT;
              break;
            default:
              move = (move == UP) ? RIGHT : DOWN;
              break;
            }
        }

      if (p == node && move != DOWN)
        break;

      if (move == DOWN)
        {
          if (p->down)
            p = p->down;
          else
            move = RIGHT;
        }
      if ((move == RIGHT) && !(p->type & CONST_SET))
        {
          if (p->right)
            p = p->right;
          else
            move = UP;
        }
      if (move == UP)
        p = MHD__asn1_find_up (p);
    }

  MHD__asn1_delete_not_used (*element);

  if (counter != len)
    {
      MHD__asn1_delete_structure (element);
      return ASN1_DER_ERROR;
    }

  return ASN1_SUCCESS;
}


/**
  * MHD__asn1_der_decoding_startEnd - Find the start and end point of an element in a DER encoding string.
  * @element: pointer to an ASN1 element
  * @ider: vector that contains the DER encoding.
  * @len: number of bytes of *@ider: @ider[0]..@ider[len-1]
  * @name_element: an element of NAME structure.
  * @start: the position of the first byte of NAME_ELEMENT decoding
  *   (@ider[*start])
  * @end: the position of the last byte of NAME_ELEMENT decoding
  *  (@ider[*end])
  *
  * Find the start and end point of an element in a DER encoding
  * string. I mean that if you have a der encoding and you have
  * already used the function "MHD__asn1_der_decoding" to fill a structure,
  * it may happen that you want to find the piece of string concerning
  * an element of the structure.
  *
  * Example: the sequence "tbsCertificate" inside an X509 certificate.
  *
  * Returns:
  *
  *   ASN1_SUCCESS: DER encoding OK.
  *
  *   ASN1_ELEMENT_NOT_FOUND: ELEMENT is ASN1_TYPE EMPTY or
  *   NAME_ELEMENT is not a valid element.
  *
  *   ASN1_TAG_ERROR,ASN1_DER_ERROR: the der encoding doesn't match
  *   the structure ELEMENT.
  *
  **/
MHD__asn1_retCode
MHD__asn1_der_decoding_startEnd (ASN1_TYPE element, const void *ider, int len,
                                 const char *name_element, int *start,
                                 int *end)
{
  node_asn *node, *node_to_find, *p, *p2, *p3;
  int counter, len2, len3, len4, move, ris;
  unsigned char class;
  unsigned long tag;
  int indefinite;
  const unsigned char *der = ider;

  node = element;

  if (node == ASN1_TYPE_EMPTY)
    return ASN1_ELEMENT_NOT_FOUND;

  node_to_find = MHD__asn1_find_node (node, name_element);

  if (node_to_find == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  if (node_to_find == node)
    {
      *start = 0;
      *end = len - 1;
      return ASN1_SUCCESS;
    }

  if (node->type & CONST_OPTION)
    return ASN1_GENERIC_ERROR;

  counter = 0;
  move = DOWN;
  p = node;
  while (1)
    {
      ris = ASN1_SUCCESS;

      if (move != UP)
        {
          if (p->type & CONST_SET)
            {
              p2 = MHD__asn1_find_up (p);
              len2 = strtol ((const char *) p2->value, NULL, 10);
              if (len2 == -1)
                {
                  if (!der[counter] && !der[counter + 1])
                    {
                      p = p2;
                      move = UP;
                      counter += 2;
                      continue;
                    }
                }
              else if (counter == len2)
                {
                  p = p2;
                  move = UP;
                  continue;
                }
              else if (counter > len2)
                return ASN1_DER_ERROR;
              p2 = p2->down;
              while (p2)
                {
                  if ((p2->type & CONST_SET) && (p2->type & CONST_NOT_USED))
                    {           /* CONTROLLARE */
                      if (type_field (p2->type) != TYPE_CHOICE)
                        ris =
                          MHD__asn1_extract_tag_der (p2, der + counter,
                                                     len - counter, &len2);
                      else
                        {
                          p3 = p2->down;
                          ris =
                            MHD__asn1_extract_tag_der (p3, der + counter,
                                                       len - counter, &len2);
                        }
                      if (ris == ASN1_SUCCESS)
                        {
                          p2->type &= ~CONST_NOT_USED;
                          p = p2;
                          break;
                        }
                    }
                  p2 = p2->right;
                }
              if (p2 == NULL)
                return ASN1_DER_ERROR;
            }

          if (p == node_to_find)
            *start = counter;

          if (type_field (p->type) == TYPE_CHOICE)
            {
              p = p->down;
              ris =
                MHD__asn1_extract_tag_der (p, der + counter, len - counter,
                                           &len2);
              if (p == node_to_find)
                *start = counter;
            }

          if (ris == ASN1_SUCCESS)
            ris =
              MHD__asn1_extract_tag_der (p, der + counter, len - counter,
                                         &len2);
          if (ris != ASN1_SUCCESS)
            {
              if (p->type & CONST_OPTION)
                {
                  p->type |= CONST_NOT_USED;
                  move = RIGHT;
                }
              else if (p->type & CONST_DEFAULT)
                {
                  move = RIGHT;
                }
              else
                {
                  return ASN1_TAG_ERROR;
                }
            }
          else
            counter += len2;
        }

      if (ris == ASN1_SUCCESS)
        {
          switch (type_field (p->type))
            {
            case TYPE_NULL:
              if (der[counter])
                return ASN1_DER_ERROR;
              counter++;
              move = RIGHT;
              break;
            case TYPE_BOOLEAN:
              if (der[counter++] != 1)
                return ASN1_DER_ERROR;
              counter++;
              move = RIGHT;
              break;
            case TYPE_INTEGER:
            case TYPE_ENUMERATED:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              counter += len3 + len2;
              move = RIGHT;
              break;
            case TYPE_OBJECT_ID:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              counter += len2 + len3;
              move = RIGHT;
              break;
            case TYPE_TIME:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              counter += len2 + len3;
              move = RIGHT;
              break;
            case TYPE_OCTET_STRING:
              len3 = len - counter;
              ris = MHD__asn1_get_octet_string (der + counter, NULL, &len3);
              if (ris != ASN1_SUCCESS)
                return ris;
              counter += len3;
              move = RIGHT;
              break;
            case TYPE_GENERALSTRING:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              counter += len3 + len2;
              move = RIGHT;
              break;
            case TYPE_BIT_STRING:
              len2 =
                MHD__asn1_get_length_der (der + counter, len - counter,
                                          &len3);
              if (len2 < 0)
                return ASN1_DER_ERROR;
              counter += len3 + len2;
              move = RIGHT;
              break;
            case TYPE_SEQUENCE:
            case TYPE_SET:
              if (move != UP)
                {
                  len3 =
                    MHD__asn1_get_length_der (der + counter, len - counter,
                                              &len2);
                  if (len3 < -1)
                    return ASN1_DER_ERROR;
                  counter += len2;
                  if (len3 == 0)
                    move = RIGHT;
                  else
                    move = DOWN;
                }
              else
                {
                  if (!der[counter] && !der[counter + 1])       /* indefinite length method */
                    counter += 2;
                  move = RIGHT;
                }
              break;
            case TYPE_SEQUENCE_OF:
            case TYPE_SET_OF:
              if (move != UP)
                {
                  len3 =
                    MHD__asn1_get_length_der (der + counter, len - counter,
                                              &len2);
                  if (len3 < -1)
                    return ASN1_DER_ERROR;
                  counter += len2;
                  if ((len3 == -1) && !der[counter] && !der[counter + 1])
                    counter += 2;
                  else if (len3)
                    {
                      p2 = p->down;
                      while ((type_field (p2->type) == TYPE_TAG) ||
                             (type_field (p2->type) == TYPE_SIZE))
                        p2 = p2->right;
                      p = p2;
                    }
                }
              else
                {
                  if (!der[counter] && !der[counter + 1])       /* indefinite length method */
                    counter += 2;
                }
              move = RIGHT;
              break;
            case TYPE_ANY:
              if (MHD__asn1_get_tag_der
                  (der + counter, len - counter, &class, &len2,
                   &tag) != ASN1_SUCCESS)
                return ASN1_DER_ERROR;
              if (counter + len2 > len)
                return ASN1_DER_ERROR;

              len4 =
                MHD__asn1_get_length_der (der + counter + len2,
                                          len - counter - len2, &len3);
              if (len4 < -1)
                return ASN1_DER_ERROR;

              if (len4 != -1)
                {
                  counter += len2 + len4 + len3;
                }
              else
                {               /* indefinite length */
                  /* Check indefinite lenth method in an EXPLICIT TAG */
                  if ((p->type & CONST_TAG) && (der[counter - 1] == 0x80))
                    indefinite = 1;
                  else
                    indefinite = 0;

                  len2 = len - counter;
                  ris =
                    MHD__asn1_get_indefinite_length_string (der + counter,
                                                            &len2);
                  if (ris != ASN1_SUCCESS)
                    return ris;
                  counter += len2;

                  /* Check if a couple of 0x00 are present due to an EXPLICIT TAG with
                     an indefinite length method. */
                  if (indefinite)
                    {
                      if (!der[counter] && !der[counter + 1])
                        counter += 2;
                      else
                        return ASN1_DER_ERROR;
                    }
                }
              move = RIGHT;
              break;
            default:
              move = (move == UP) ? RIGHT : DOWN;
              break;
            }
        }

      if ((p == node_to_find) && (move == RIGHT))
        {
          *end = counter - 1;
          return ASN1_SUCCESS;
        }

      if (p == node && move != DOWN)
        break;

      if (move == DOWN)
        {
          if (p->down)
            p = p->down;
          else
            move = RIGHT;
        }
      if ((move == RIGHT) && !(p->type & CONST_SET))
        {
          if (p->right)
            p = p->right;
          else
            move = UP;
        }
      if (move == UP)
        p = MHD__asn1_find_up (p);
    }

  return ASN1_ELEMENT_NOT_FOUND;
}
