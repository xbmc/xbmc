/*
 * Copyright 2009-2012 10gen, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BSON_ENCODING_H_
#define BSON_ENCODING_H_

MONGO_EXTERN_C_START

/**
 * Check that a field name is valid UTF8, does not start with a '$',
 * and contains no '.' characters. Set bson bit field appropriately.
 * Note that we don't need to check for '\0' because we're using
 * strlen(3), which stops at '\0'.
 *
 * @param b The bson object to which field name will be appended.
 * @param string The field name as char*.
 * @param length The length of the field name.
 *
 * @return BSON_OK if valid UTF8 and BSON_ERROR if not. All BSON strings must be
 *     valid UTF8. This function will also check whether the string
 *     contains '.' or starts with '$', since the validity of this depends on context.
 *     Set the value of b->err appropriately.
 */
int bson_check_field_name( bson *b, const char *string,
                           const int length );

/**
 * Check that a string is valid UTF8. Sets the buffer bit field appropriately.
 *
 * @param b The bson object to which string will be appended.
 * @param string The string to check.
 * @param length The length of the string.
 *
 * @return BSON_OK if valid UTF-8; otherwise, BSON_ERROR.
 *     Sets b->err on error.
 */
bson_bool_t bson_check_string( bson *b, const char *string,
                               const int length );

MONGO_EXTERN_C_END
#endif
