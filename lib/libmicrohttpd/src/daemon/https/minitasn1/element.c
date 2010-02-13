/*
 *      Copyright (C) 2004, 2006 Free Software Foundation
 *      Copyright (C) 2000, 2001, 2002, 2003 Fabio Fiorina
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
/* File: element.c                                   */
/* Description: Functions with the read and write    */
/*   functions.                                      */
/*****************************************************/


#include <int.h>
#include "parser_aux.h"
#include <gstr.h>
#include "structure.h"

void
MHD__asn1_hierarchical_name (node_asn * node, char *name, int name_size)
{
  node_asn *p;
  char tmp_name[64];

  p = node;

  name[0] = 0;

  while (p != NULL)
    {
      if (p->name != NULL)
        {
          MHD__asn1_str_cpy (tmp_name, sizeof (tmp_name), name),
            MHD__asn1_str_cpy (name, name_size, p->name);
          MHD__asn1_str_cat (name, name_size, ".");
          MHD__asn1_str_cat (name, name_size, tmp_name);
        }
      p = MHD__asn1_find_up (p);
    }

  if (name[0] == 0)
    MHD__asn1_str_cpy (name, name_size, "ROOT");
}


/******************************************************************/
/* Function : MHD__asn1_convert_integer                               */
/* Description: converts an integer from a null terminated string */
/*              to der decoding. The convertion from a null       */
/*              terminated string to an integer is made with      */
/*              the 'strtol' function.                            */
/* Parameters:                                                    */
/*   value: null terminated string to convert.                    */
/*   value_out: convertion result (memory must be already         */
/*              allocated).                                       */
/*   value_out_size: number of bytes of value_out.                */
/*   len: number of significant byte of value_out.                */
/* Return: ASN1_MEM_ERROR or ASN1_SUCCESS                         */
/******************************************************************/
MHD__asn1_retCode
MHD__asn1_convert_integer (const char *value, unsigned char *value_out,
                           int value_out_size, int *len)
{
  char negative;
  unsigned char val[SIZEOF_UNSIGNED_LONG_INT];
  long valtmp;
  int k, k2;

  valtmp = strtol (value, NULL, 10);

  for (k = 0; k < SIZEOF_UNSIGNED_LONG_INT; k++)
    {
      val[SIZEOF_UNSIGNED_LONG_INT - k - 1] = (valtmp >> (8 * k)) & 0xFF;
    }

  if (val[0] & 0x80)
    negative = 1;
  else
    negative = 0;

  for (k = 0; k < SIZEOF_UNSIGNED_LONG_INT - 1; k++)
    {
      if (negative && (val[k] != 0xFF))
        break;
      else if (!negative && val[k])
        break;
    }

  if ((negative && !(val[k] & 0x80)) || (!negative && (val[k] & 0x80)))
    k--;

  *len = SIZEOF_UNSIGNED_LONG_INT - k;

  if (SIZEOF_UNSIGNED_LONG_INT - k > value_out_size)
    /* VALUE_OUT is too short to contain the value conversion */
    return ASN1_MEM_ERROR;

  for (k2 = k; k2 < SIZEOF_UNSIGNED_LONG_INT; k2++)
    value_out[k2 - k] = val[k2];

  return ASN1_SUCCESS;
}


int
MHD__asn1_append_sequence_set (node_asn * node)
{
  node_asn *p, *p2;
  char temp[10];
  long n;

  if (!node || !(node->down))
    return ASN1_GENERIC_ERROR;

  p = node->down;
  while ((type_field (p->type) == TYPE_TAG)
         || (type_field (p->type) == TYPE_SIZE))
    p = p->right;
  p2 = MHD__asn1_copy_structure3 (p);
  while (p->right)
    p = p->right;
  MHD__asn1_set_right (p, p2);

  if (p->name == NULL)
    MHD__asn1_str_cpy (temp, sizeof (temp), "?1");
  else
    {
      n = strtol (p->name + 1, NULL, 0);
      n++;
      temp[0] = '?';
      MHD__asn1_ltostr (n, temp + 1);
    }
  MHD__asn1_set_name (p2, temp);
  /*  p2->type |= CONST_OPTION; */

  return ASN1_SUCCESS;
}


/**
  * MHD__asn1_write_value - Set the value of one element inside a structure.
  * @node_root: pointer to a structure
  * @name: the name of the element inside the structure that you want to set.
  * @ivalue: vector used to specify the value to set. If len is >0,
  *   VALUE must be a two's complement form integer.  if len=0 *VALUE
  *   must be a null terminated string with an integer value.
  * @len: number of bytes of *value to use to set the value:
  *   value[0]..value[len-1] or 0 if value is a null terminated string
  *
  * Set the value of one element inside a structure.
  *
  * If an element is OPTIONAL and you want to delete it, you must use
  * the value=NULL and len=0.  Using "pkix.asn":
  *
  * result=MHD__asn1_write_value(cert, "tbsCertificate.issuerUniqueID",
  * NULL, 0);
  *
  * Description for each type:
  *
  * INTEGER: VALUE must contain a two's complement form integer.
  *
  *            value[0]=0xFF ,               len=1 -> integer=-1.
  *            value[0]=0xFF value[1]=0xFF , len=2 -> integer=-1.
  *            value[0]=0x01 ,               len=1 -> integer= 1.
  *            value[0]=0x00 value[1]=0x01 , len=2 -> integer= 1.
  *            value="123"                 , len=0 -> integer= 123.
  *
  * ENUMERATED: As INTEGER (but only with not negative numbers).
  *
  * BOOLEAN: VALUE must be the null terminated string "TRUE" or
  *   "FALSE" and LEN != 0.
  *
  *            value="TRUE" , len=1 -> boolean=TRUE.
  *            value="FALSE" , len=1 -> boolean=FALSE.
  *
  * OBJECT IDENTIFIER: VALUE must be a null terminated string with
  *   each number separated by a dot (e.g. "1.2.3.543.1").  LEN != 0.
  *
  *            value="1 2 840 10040 4 3" , len=1 -> OID=dsa-with-sha.
  *
  * UTCTime: VALUE must be a null terminated string in one of these
  *   formats: "YYMMDDhhmmssZ", "YYMMDDhhmmssZ",
  *   "YYMMDDhhmmss+hh'mm'", "YYMMDDhhmmss-hh'mm'",
  *   "YYMMDDhhmm+hh'mm'", or "YYMMDDhhmm-hh'mm'".  LEN != 0.
  *
  *            value="9801011200Z" , len=1 -> time=Jannuary 1st, 1998
  *            at 12h 00m Greenwich Mean Time
  *
  * GeneralizedTime: VALUE must be in one of this format:
  *   "YYYYMMDDhhmmss.sZ", "YYYYMMDDhhmmss.sZ",
  *   "YYYYMMDDhhmmss.s+hh'mm'", "YYYYMMDDhhmmss.s-hh'mm'",
  *   "YYYYMMDDhhmm+hh'mm'", or "YYYYMMDDhhmm-hh'mm'" where ss.s
  *   indicates the seconds with any precision like "10.1" or "01.02".
  *   LEN != 0
  *
  *            value="2001010112001.12-0700" , len=1 -> time=Jannuary
  *            1st, 2001 at 12h 00m 01.12s Pacific Daylight Time
  *
  * OCTET STRING: VALUE contains the octet string and LEN is the
  *   number of octets.
  *
  *            value="$\backslash$x01$\backslash$x02$\backslash$x03" ,
  *            len=3 -> three bytes octet string
  *
  * GeneralString: VALUE contains the generalstring and LEN is the
  *   number of octets.
  *
  *            value="$\backslash$x01$\backslash$x02$\backslash$x03" ,
  *            len=3 -> three bytes generalstring
  *
  * BIT STRING: VALUE contains the bit string organized by bytes and
  *   LEN is the number of bits.
  *
  *   value="$\backslash$xCF" , len=6 -> bit string="110011" (six
  *   bits)
  *
  * CHOICE: if NAME indicates a choice type, VALUE must specify one of
  *   the alternatives with a null terminated string. LEN != 0. Using
  *   "pkix.asn"\:
  *
  *           result=MHD__asn1_write_value(cert,
  *           "certificate1.tbsCertificate.subject", "rdnSequence",
  *           1);
  *
  * ANY: VALUE indicates the der encoding of a structure.  LEN != 0.
  *
  * SEQUENCE OF: VALUE must be the null terminated string "NEW" and
  *   LEN != 0. With this instruction another element is appended in
  *   the sequence. The name of this element will be "?1" if it's the
  *   first one, "?2" for the second and so on.
  *
  *   Using "pkix.asn"\:
  *
  *   result=MHD__asn1_write_value(cert,
  *   "certificate1.tbsCertificate.subject.rdnSequence", "NEW", 1);
  *
  * SET OF: the same as SEQUENCE OF.  Using "pkix.asn":
  *
  *           result=MHD__asn1_write_value(cert,
  *           "tbsCertificate.subject.rdnSequence.?LAST", "NEW", 1);
  *
  * Returns:
  *
  *   ASN1_SUCCESS: Set value OK.
  *
  *   ASN1_ELEMENT_NOT_FOUND: NAME is not a valid element.
  *
  *   ASN1_VALUE_NOT_VALID: VALUE has a wrong format.
  *
  **/
MHD__asn1_retCode
MHD__asn1_write_value (ASN1_TYPE node_root, const char *name,
                       const void *ivalue, int len)
{
  node_asn *node, *p, *p2;
  unsigned char *temp, *value_temp = NULL, *default_temp = NULL;
  int len2, k, k2, negative;
  const char *value = ivalue;

  node = MHD__asn1_find_node (node_root, name);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  if ((node->type & CONST_OPTION) && (value == NULL) && (len == 0))
    {
      MHD__asn1_delete_structure (&node);
      return ASN1_SUCCESS;
    }

  if ((type_field (node->type) == TYPE_SEQUENCE_OF) && (value == NULL)
      && (len == 0))
    {
      p = node->down;
      while ((type_field (p->type) == TYPE_TAG)
             || (type_field (p->type) == TYPE_SIZE))
        p = p->right;

      while (p->right)
        MHD__asn1_delete_structure (&p->right);

      return ASN1_SUCCESS;
    }

  switch (type_field (node->type))
    {
    case TYPE_BOOLEAN:
      if (!strcmp (value, "TRUE"))
        {
          if (node->type & CONST_DEFAULT)
            {
              p = node->down;
              while (type_field (p->type) != TYPE_DEFAULT)
                p = p->right;
              if (p->type & CONST_TRUE)
                MHD__asn1_set_value (node, NULL, 0);
              else
                MHD__asn1_set_value (node, "T", 1);
            }
          else
            MHD__asn1_set_value (node, "T", 1);
        }
      else if (!strcmp (value, "FALSE"))
        {
          if (node->type & CONST_DEFAULT)
            {
              p = node->down;
              while (type_field (p->type) != TYPE_DEFAULT)
                p = p->right;
              if (p->type & CONST_FALSE)
                MHD__asn1_set_value (node, NULL, 0);
              else
                MHD__asn1_set_value (node, "F", 1);
            }
          else
            MHD__asn1_set_value (node, "F", 1);
        }
      else
        return ASN1_VALUE_NOT_VALID;
      break;
    case TYPE_INTEGER:
    case TYPE_ENUMERATED:
      if (len == 0)
        {
          if ((isdigit (value[0])) || (value[0] == '-'))
            {
              value_temp =
                (unsigned char *) MHD__asn1_alloca (SIZEOF_UNSIGNED_LONG_INT);
              if (value_temp == NULL)
                return ASN1_MEM_ALLOC_ERROR;

              MHD__asn1_convert_integer (value, value_temp,
                                         SIZEOF_UNSIGNED_LONG_INT, &len);
            }
          else
            {                   /* is an identifier like v1 */
              if (!(node->type & CONST_LIST))
                return ASN1_VALUE_NOT_VALID;
              p = node->down;
              while (p)
                {
                  if (type_field (p->type) == TYPE_CONSTANT)
                    {
                      if ((p->name) && (!strcmp (p->name, value)))
                        {
                          value_temp =
                            (unsigned char *)
                            MHD__asn1_alloca (SIZEOF_UNSIGNED_LONG_INT);
                          if (value_temp == NULL)
                            return ASN1_MEM_ALLOC_ERROR;

                          MHD__asn1_convert_integer ((const char *) p->value,
                                                     value_temp,
                                                     SIZEOF_UNSIGNED_LONG_INT,
                                                     &len);
                          break;
                        }
                    }
                  p = p->right;
                }
              if (p == NULL)
                return ASN1_VALUE_NOT_VALID;
            }
        }
      else
        {                       /* len != 0 */
          value_temp = (unsigned char *) MHD__asn1_alloca (len);
          if (value_temp == NULL)
            return ASN1_MEM_ALLOC_ERROR;
          memcpy (value_temp, value, len);
        }


      if (value_temp[0] & 0x80)
        negative = 1;
      else
        negative = 0;

      if (negative && (type_field (node->type) == TYPE_ENUMERATED))
        {
          MHD__asn1_afree (value_temp);
          return ASN1_VALUE_NOT_VALID;
        }

      for (k = 0; k < len - 1; k++)
        if (negative && (value_temp[k] != 0xFF))
          break;
        else if (!negative && value_temp[k])
          break;

      if ((negative && !(value_temp[k] & 0x80)) ||
          (!negative && (value_temp[k] & 0x80)))
        k--;

      MHD__asn1_length_der (len - k, NULL, &len2);
      temp = (unsigned char *) MHD__asn1_alloca (len - k + len2);
      if (temp == NULL)
        {
          MHD__asn1_afree (value_temp);
          return ASN1_MEM_ALLOC_ERROR;
        }

      MHD__asn1_octet_der (value_temp + k, len - k, temp, &len2);
      MHD__asn1_set_value (node, temp, len2);

      MHD__asn1_afree (temp);


      if (node->type & CONST_DEFAULT)
        {
          p = node->down;
          while (type_field (p->type) != TYPE_DEFAULT)
            p = p->right;
          if ((isdigit (p->value[0])) || (p->value[0] == '-'))
            {
              default_temp =
                (unsigned char *) MHD__asn1_alloca (SIZEOF_UNSIGNED_LONG_INT);
              if (default_temp == NULL)
                {
                  MHD__asn1_afree (value_temp);
                  return ASN1_MEM_ALLOC_ERROR;
                }

              MHD__asn1_convert_integer ((const char *) p->value,
                                         default_temp,
                                         SIZEOF_UNSIGNED_LONG_INT, &len2);
            }
          else
            {                   /* is an identifier like v1 */
              if (!(node->type & CONST_LIST))
                {
                  MHD__asn1_afree (value_temp);
                  return ASN1_VALUE_NOT_VALID;
                }
              p2 = node->down;
              while (p2)
                {
                  if (type_field (p2->type) == TYPE_CONSTANT)
                    {
                      if ((p2->name)
                          && (!strcmp (p2->name, (const char *) p->value)))
                        {
                          default_temp =
                            (unsigned char *)
                            MHD__asn1_alloca (SIZEOF_UNSIGNED_LONG_INT);
                          if (default_temp == NULL)
                            {
                              MHD__asn1_afree (value_temp);
                              return ASN1_MEM_ALLOC_ERROR;
                            }

                          MHD__asn1_convert_integer ((const char *) p2->value,
                                                     default_temp,
                                                     SIZEOF_UNSIGNED_LONG_INT,
                                                     &len2);
                          break;
                        }
                    }
                  p2 = p2->right;
                }
              if (p2 == NULL)
                {
                  MHD__asn1_afree (value_temp);
                  return ASN1_VALUE_NOT_VALID;
                }
            }


          if ((len - k) == len2)
            {
              for (k2 = 0; k2 < len2; k2++)
                if (value_temp[k + k2] != default_temp[k2])
                  {
                    break;
                  }
              if (k2 == len2)
                MHD__asn1_set_value (node, NULL, 0);
            }
          MHD__asn1_afree (default_temp);
        }
      MHD__asn1_afree (value_temp);
      break;
    case TYPE_OBJECT_ID:
      for (k = 0; k < strlen (value); k++)
        if ((!isdigit (value[k])) && (value[k] != '.') && (value[k] != '+'))
          return ASN1_VALUE_NOT_VALID;
      if (node->type & CONST_DEFAULT)
        {
          p = node->down;
          while (type_field (p->type) != TYPE_DEFAULT)
            p = p->right;
          if (!strcmp (value, (const char *) p->value))
            {
              MHD__asn1_set_value (node, NULL, 0);
              break;
            }
        }
      MHD__asn1_set_value (node, value, strlen (value) + 1);
      break;
    case TYPE_TIME:
      if (node->type & CONST_UTC)
        {
          if (strlen (value) < 11)
            return ASN1_VALUE_NOT_VALID;
          for (k = 0; k < 10; k++)
            if (!isdigit (value[k]))
              return ASN1_VALUE_NOT_VALID;
          switch (strlen (value))
            {
            case 11:
              if (value[10] != 'Z')
                return ASN1_VALUE_NOT_VALID;
              break;
            case 13:
              if ((!isdigit (value[10])) || (!isdigit (value[11])) ||
                  (value[12] != 'Z'))
                return ASN1_VALUE_NOT_VALID;
              break;
            case 15:
              if ((value[10] != '+') && (value[10] != '-'))
                return ASN1_VALUE_NOT_VALID;
              for (k = 11; k < 15; k++)
                if (!isdigit (value[k]))
                  return ASN1_VALUE_NOT_VALID;
              break;
            case 17:
              if ((!isdigit (value[10])) || (!isdigit (value[11])))
                return ASN1_VALUE_NOT_VALID;
              if ((value[12] != '+') && (value[12] != '-'))
                return ASN1_VALUE_NOT_VALID;
              for (k = 13; k < 17; k++)
                if (!isdigit (value[k]))
                  return ASN1_VALUE_NOT_VALID;
              break;
            default:
              return ASN1_VALUE_NOT_FOUND;
            }
          MHD__asn1_set_value (node, value, strlen (value) + 1);
        }
      else
        {                       /* GENERALIZED TIME */
          if (value)
            MHD__asn1_set_value (node, value, strlen (value) + 1);
        }
      break;
    case TYPE_OCTET_STRING:
      if (len == 0)
        len = strlen (value);
      MHD__asn1_length_der (len, NULL, &len2);
      temp = (unsigned char *) MHD__asn1_alloca (len + len2);
      if (temp == NULL)
        return ASN1_MEM_ALLOC_ERROR;

      MHD__asn1_octet_der ((const unsigned char *) value, len, temp, &len2);
      MHD__asn1_set_value (node, temp, len2);
      MHD__asn1_afree (temp);
      break;
    case TYPE_GENERALSTRING:
      if (len == 0)
        len = strlen (value);
      MHD__asn1_length_der (len, NULL, &len2);
      temp = (unsigned char *) MHD__asn1_alloca (len + len2);
      if (temp == NULL)
        return ASN1_MEM_ALLOC_ERROR;

      MHD__asn1_octet_der ((const unsigned char *) value, len, temp, &len2);
      MHD__asn1_set_value (node, temp, len2);
      MHD__asn1_afree (temp);
      break;
    case TYPE_BIT_STRING:
      if (len == 0)
        len = strlen (value);
      MHD__asn1_length_der ((len >> 3) + 2, NULL, &len2);
      temp = (unsigned char *) MHD__asn1_alloca ((len >> 3) + 2 + len2);
      if (temp == NULL)
        return ASN1_MEM_ALLOC_ERROR;

      MHD__asn1_bit_der ((const unsigned char *) value, len, temp, &len2);
      MHD__asn1_set_value (node, temp, len2);
      MHD__asn1_afree (temp);
      break;
    case TYPE_CHOICE:
      p = node->down;
      while (p)
        {
          if (!strcmp (p->name, value))
            {
              p2 = node->down;
              while (p2)
                {
                  if (p2 != p)
                    {
                      MHD__asn1_delete_structure (&p2);
                      p2 = node->down;
                    }
                  else
                    p2 = p2->right;
                }
              break;
            }
          p = p->right;
        }
      if (!p)
        return ASN1_ELEMENT_NOT_FOUND;
      break;
    case TYPE_ANY:
      MHD__asn1_length_der (len, NULL, &len2);
      temp = (unsigned char *) MHD__asn1_alloca (len + len2);
      if (temp == NULL)
        return ASN1_MEM_ALLOC_ERROR;

      MHD__asn1_octet_der ((const unsigned char *) value, len, temp, &len2);
      MHD__asn1_set_value (node, temp, len2);
      MHD__asn1_afree (temp);
      break;
    case TYPE_SEQUENCE_OF:
    case TYPE_SET_OF:
      if (strcmp (value, "NEW"))
        return ASN1_VALUE_NOT_VALID;
      MHD__asn1_append_sequence_set (node);
      break;
    default:
      return ASN1_ELEMENT_NOT_FOUND;
      break;
    }

  return ASN1_SUCCESS;
}


#define PUT_VALUE( ptr, ptr_size, data, data_size) \
	*len = data_size; \
	if (ptr_size < data_size) { \
		return ASN1_MEM_ERROR; \
	} else { \
		memcpy( ptr, data, data_size); \
	}

#define PUT_STR_VALUE( ptr, ptr_size, data) \
	*len = strlen(data) + 1; \
	if (ptr_size < *len) { \
		return ASN1_MEM_ERROR; \
	} else { \
		/* this strcpy is checked */ \
		strcpy(ptr, data); \
	}

#define ADD_STR_VALUE( ptr, ptr_size, data) \
	*len = strlen(data) + 1; \
	if (ptr_size < strlen(ptr)+(*len)) { \
		return ASN1_MEM_ERROR; \
	} else { \
		/* this strcat is checked */ \
		strcat(ptr, data); \
	}

/**
  * MHD__asn1_read_value - Returns the value of one element inside a structure
  * @root: pointer to a structure.
  * @name: the name of the element inside a structure that you want to read.
  * @ivalue: vector that will contain the element's content, must be a
  *   pointer to memory cells already allocated.
  * @len: number of bytes of *value: value[0]..value[len-1]. Initialy
  *   holds the sizeof value.
  *
  * Returns the value of one element inside a structure.
  *
  * If an element is OPTIONAL and the function "read_value" returns
  * %ASN1_ELEMENT_NOT_FOUND, it means that this element wasn't present
  * in the der encoding that created the structure.  The first element
  * of a SEQUENCE_OF or SET_OF is named "?1". The second one "?2" and
  * so on.
  *
  * INTEGER: VALUE will contain a two's complement form integer.
  *
  *            integer=-1  -> value[0]=0xFF , len=1.
  *            integer=1   -> value[0]=0x01 , len=1.
  *
  * ENUMERATED: As INTEGER (but only with not negative numbers).
  *
  * BOOLEAN: VALUE will be the null terminated string "TRUE" or
  *   "FALSE" and LEN=5 or LEN=6.
  *
  * OBJECT IDENTIFIER: VALUE will be a null terminated string with
  *   each number separated by a dot (i.e. "1.2.3.543.1").
  *
  *                      LEN = strlen(VALUE)+1
  *
  * UTCTime: VALUE will be a null terminated string in one of these
  *   formats: "YYMMDDhhmmss+hh'mm'" or "YYMMDDhhmmss-hh'mm'".
  *   LEN=strlen(VALUE)+1.
  *
  * GeneralizedTime: VALUE will be a null terminated string in the
  *   same format used to set the value.
  *
  * OCTET STRING: VALUE will contain the octet string and LEN will be
  *   the number of octets.
  *
  * GeneralString: VALUE will contain the generalstring and LEN will
  *   be the number of octets.
  *
  * BIT STRING: VALUE will contain the bit string organized by bytes
  *   and LEN will be the number of bits.
  *
  * CHOICE: If NAME indicates a choice type, VALUE will specify the
  *   alternative selected.
  *
  * ANY: If NAME indicates an any type, VALUE will indicate the DER
  *   encoding of the structure actually used.
  *
  * Returns:
  *
  *   ASN1_SUCCESS: Set value OK.
  *
  *   ASN1_ELEMENT_NOT_FOUND: NAME is not a valid element.
  *
  *   ASN1_VALUE_NOT_FOUND: There isn't any value for the element selected.
  *
  *   ASN1_MEM_ERROR: The value vector isn't big enough to store the result.
  *   In this case LEN will contain the number of bytes needed.
  *
  **/
MHD__asn1_retCode
MHD__asn1_read_value (ASN1_TYPE root, const char *name, void *ivalue,
                      int *len)
{
  node_asn *node, *p, *p2;
  int len2, len3;
  int value_size = *len;
  char *value = ivalue;

  node = MHD__asn1_find_node (root, name);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  if ((type_field (node->type) != TYPE_NULL) &&
      (type_field (node->type) != TYPE_CHOICE) &&
      !(node->type & CONST_DEFAULT) && !(node->type & CONST_ASSIGN) &&
      (node->value == NULL))
    return ASN1_VALUE_NOT_FOUND;

  switch (type_field (node->type))
    {
    case TYPE_NULL:
      PUT_STR_VALUE (value, value_size, "NULL");
      break;
    case TYPE_BOOLEAN:
      if ((node->type & CONST_DEFAULT) && (node->value == NULL))
        {
          p = node->down;
          while (type_field (p->type) != TYPE_DEFAULT)
            p = p->right;
          if (p->type & CONST_TRUE)
            {
              PUT_STR_VALUE (value, value_size, "TRUE");
            }
          else
            {
              PUT_STR_VALUE (value, value_size, "FALSE");
            }
        }
      else if (node->value[0] == 'T')
        {
          PUT_STR_VALUE (value, value_size, "TRUE");
        }
      else
        {
          PUT_STR_VALUE (value, value_size, "FALSE");
        }
      break;
    case TYPE_INTEGER:
    case TYPE_ENUMERATED:
      if ((node->type & CONST_DEFAULT) && (node->value == NULL))
        {
          p = node->down;
          while (type_field (p->type) != TYPE_DEFAULT)
            p = p->right;
          if ((isdigit (p->value[0])) || (p->value[0] == '-')
              || (p->value[0] == '+'))
            {
              if (MHD__asn1_convert_integer
                  ((const char *) p->value, (unsigned char *) value,
                   value_size, len) != ASN1_SUCCESS)
                return ASN1_MEM_ERROR;
            }
          else
            {                   /* is an identifier like v1 */
              p2 = node->down;
              while (p2)
                {
                  if (type_field (p2->type) == TYPE_CONSTANT)
                    {
                      if ((p2->name)
                          && (!strcmp (p2->name, (const char *) p->value)))
                        {
                          if (MHD__asn1_convert_integer
                              ((const char *) p2->value,
                               (unsigned char *) value, value_size,
                               len) != ASN1_SUCCESS)
                            return ASN1_MEM_ERROR;
                          break;
                        }
                    }
                  p2 = p2->right;
                }
            }
        }
      else
        {
          len2 = -1;
          if (MHD__asn1_get_octet_der
              (node->value, node->value_len, &len2, (unsigned char *) value,
               value_size, len) != ASN1_SUCCESS)
            return ASN1_MEM_ERROR;
        }
      break;
    case TYPE_OBJECT_ID:
      if (node->type & CONST_ASSIGN)
        {
          value[0] = 0;
          p = node->down;
          while (p)
            {
              if (type_field (p->type) == TYPE_CONSTANT)
                {
                  ADD_STR_VALUE (value, value_size, (const char *) p->value);
                  if (p->right)
                    {
                      ADD_STR_VALUE (value, value_size, ".");
                    }
                }
              p = p->right;
            }
          *len = strlen (value) + 1;
        }
      else if ((node->type & CONST_DEFAULT) && (node->value == NULL))
        {
          p = node->down;
          while (type_field (p->type) != TYPE_DEFAULT)
            p = p->right;
          PUT_STR_VALUE (value, value_size, (const char *) p->value);
        }
      else
        {
          PUT_STR_VALUE (value, value_size, (const char *) node->value);
        }
      break;
    case TYPE_TIME:
      PUT_STR_VALUE (value, value_size, (const char *) node->value);
      break;
    case TYPE_OCTET_STRING:
      len2 = -1;
      if (MHD__asn1_get_octet_der
          (node->value, node->value_len, &len2, (unsigned char *) value,
           value_size, len) != ASN1_SUCCESS)
        return ASN1_MEM_ERROR;
      break;
    case TYPE_GENERALSTRING:
      len2 = -1;
      if (MHD__asn1_get_octet_der
          (node->value, node->value_len, &len2, (unsigned char *) value,
           value_size, len) != ASN1_SUCCESS)
        return ASN1_MEM_ERROR;
      break;
    case TYPE_BIT_STRING:
      len2 = -1;
      if (MHD__asn1_get_bit_der
          (node->value, node->value_len, &len2, (unsigned char *) value,
           value_size, len) != ASN1_SUCCESS)
        return ASN1_MEM_ERROR;
      break;
    case TYPE_CHOICE:
      PUT_STR_VALUE (value, value_size, node->down->name);
      break;
    case TYPE_ANY:
      len3 = -1;
      len2 = MHD__asn1_get_length_der (node->value, node->value_len, &len3);
      if (len2 < 0)
        return ASN1_DER_ERROR;
      PUT_VALUE (value, value_size, node->value + len3, len2);
      break;
    default:
      return ASN1_ELEMENT_NOT_FOUND;
      break;
    }
  return ASN1_SUCCESS;
}
