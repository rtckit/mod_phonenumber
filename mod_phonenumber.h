/*
 * Copyright (c) 2019 Ciprian Dosoftei
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MOD_PHONENUMBER_H
#define MOD_PHONENUMBER_H

#include <switch.h>

#include "phonenumbers/phonenumberutil.h"

using i18n::phonenumbers::PhoneNumber;
using i18n::phonenumbers::PhoneNumberUtil;

/**
 * Maximum actions per run
 */
#define PN_MAX_ACTIONS 20

/**
 * Application/API syntax
 */
#define PN_SYNTAX "<action(s)> <number> [argument(s)]"

/**
 * Action function helper
 *
 * @param request Request to action on.
 */
#define PN_ACTION(name) void name(phonenumber_request_t *request)

/**
 * Default configuration
 *
 * Values to be used if there is no specific default configuration defined in
 * phonenumber.conf.xml.
 */
#define PN_DEFAULT_REGION "US"
#define PN_DEFAULT_FORMAT PhoneNumberUtil::E164
#define PN_DEFAULT_LOCALE "en_US"
#define PN_DEFAULT_CALLING_FROM "US"

/**
 * Various string-oriented constants for internal use
 */
#define PN_EMPTY ""
#define PN_NUMBER "number"
#define PN_CALLER "caller"
#define PN_DESTINATION "destination"
#define PN_INBOUND "inbound"
#define PN_OUTBOUND "outbound"
#define PN_ALL "all"

#define PN_LEN_EMPTY 0
#define PN_LEN_NUMBER 6
#define PN_LEN_CALLER 6
#define PN_LEN_DESTINATION 11
#define PN_LEN_INBOUND 7
#define PN_LEN_OUTBOUND 8
#define PN_LEN_ALL 3

#define PN_PARAM_DEFAULT_REGION "default_region"
#define PN_PARAM_FORMAT "format"
#define PN_PARAM_LOCALE "locale"
#define PN_PARAM_CALLING_FROM "calling_from"
#define PN_PARAM_DIRECTION "direction"
#define PN_PARAM_CONTEXT "context"
#define PN_PARAM_SCOPE "scope"
#define PN_PARAM_ACTIONS "actions"

#define PN_PARAM_LEN_DEFAULT_REGION 14
#define PN_PARAM_LEN_FORMAT 6
#define PN_PARAM_LEN_LOCALE 6
#define PN_PARAM_LEN_CALLING_FROM 12
#define PN_PARAM_LEN_DIRECTION 9
#define PN_PARAM_LEN_CONTEXT 7
#define PN_PARAM_LEN_SCOPE 5
#define PN_PARAM_LEN_ACTIONS 7

#define PN_ACTION_IS_ALPHA_NUMBER "is_alpha_number"
#define PN_ACTION_CONVERT_ALPHA_CHARACTERS_IN_NUMBER "convert_alpha_characters_in_number"
#define PN_ACTION_NORMALIZE_DIGITS_ONLY "normalize_digits_only"
#define PN_ACTION_NORMALIZE_DIALLABLE_CHARS_ONLY "normalize_diallable_chars_only"
#define PN_ACTION_GET_NATIONAL_SIGNIFICANT_NUMBER "get_national_significant_number"
#define PN_ACTION_FORMAT_OUT_OF_COUNTRY_CALLING_NUMBER "format_out_of_country_calling_number"
#define PN_ACTION_FORMAT "format"
#define PN_ACTION_GET_NUMBER_TYPE "get_number_type"
#define PN_ACTION_IS_VALID_NUMBER_FOR_REGION "is_valid_number_for_region"
#define PN_ACTION_GET_REGION_CODE "get_region_code"
#define PN_ACTION_IS_POSSIBLE_NUMBER_WITH_REASON "is_possible_number_with_reason"
#define PN_ACTION_IS_POSSIBLE_NUMBER "is_possible_number"
#define PN_ACTION_GET_DESCRIPTION_FOR_NUMBER "get_description_for_number"

#define PN_ACTION_LEN_IS_ALPHA_NUMBER 15
#define PN_ACTION_LEN_CONVERT_ALPHA_CHARACTERS_IN_NUMBER 34
#define PN_ACTION_LEN_NORMALIZE_DIGITS_ONLY 21
#define PN_ACTION_LEN_NORMALIZE_DIALLABLE_CHARS_ONLY 30
#define PN_ACTION_LEN_GET_NATIONAL_SIGNIFICANT_NUMBER 31
#define PN_ACTION_LEN_FORMAT_OUT_OF_COUNTRY_CALLING_NUMBER 36
#define PN_ACTION_LEN_FORMAT 6
#define PN_ACTION_LEN_GET_NUMBER_TYPE 15
#define PN_ACTION_LEN_IS_VALID_NUMBER_FOR_REGION 26
#define PN_ACTION_LEN_GET_REGION_CODE 15
#define PN_ACTION_LEN_IS_POSSIBLE_NUMBER_WITH_REASON 30
#define PN_ACTION_LEN_IS_POSSIBLE_NUMBER 18
#define PN_ACTION_LEN_GET_DESCRIPTION_FOR_NUMBER 26

#define PN_FORMAT_E164 "E164"
#define PN_FORMAT_INTERNATIONAL "INTERNATIONAL"
#define PN_FORMAT_NATIONAL "NATIONAL"
#define PN_FORMAT_RFC3966 "RFC3966"

#define PN_FORMAT_LEN_E164 4
#define PN_FORMAT_LEN_INTERNATIONAL 13
#define PN_FORMAT_LEN_NATIONAL 8
#define PN_FORMAT_LEN_RFC3966 7

/**
 * Type definitions
 */
struct phonenumber_config {
  char default_region[3];
  PhoneNumberUtil::PhoneNumberFormat format;
  char locale[6];
  char calling_from[3];
};

typedef struct phonenumber_config phonenumber_config_t;

struct phonenumber_request {
  char *number;
  phonenumber_config_t *config;
  PhoneNumber *parsed;
  switch_channel_t *channel;
  switch_stream_handle_t *stream;
  char *prefix;
};

typedef struct phonenumber_request phonenumber_request_t;

typedef void (*phonenumber_action_t)(phonenumber_request_t *request);

enum phonenumber_direction {
  DIRECTION_ALL,
  DIRECTION_INBOUND,
  DIRECTION_OUTBOUND
};

enum phonenumber_scope {
  SCOPE_ALL,
  SCOPE_CALLER,
  SCOPE_DESTINATION
};

struct phonenumber_hook {
  char *context;
  phonenumber_direction direction;
  phonenumber_scope scope;
  phonenumber_config config;
  phonenumber_action_t *actions;
  struct phonenumber_hook *next;
};

typedef struct phonenumber_hook phonenumber_hook_t;

/**
 * All implemented actions
 */
PN_ACTION(is_alpha_number);
PN_ACTION(convert_alpha_characters_in_number);
PN_ACTION(normalize_digits_only);
PN_ACTION(normalize_diallable_chars_only);
PN_ACTION(get_national_significant_number);
PN_ACTION(format_out_of_country_calling_number);
PN_ACTION(format);
PN_ACTION(get_number_type);
PN_ACTION(is_valid_number_for_region);
PN_ACTION(get_region_code);
PN_ACTION(is_possible_number_with_reason);
PN_ACTION(is_possible_number);
PN_ACTION(get_description_for_number);

/**
 * Globals
 */
extern phonenumber_config_t mod_phonenumber_config;
extern phonenumber_hook_t *mod_phonenumber_hooks;
extern const PhoneNumberUtil &phone_util;

/**
 * Helper functions
 */
switch_status_t pn_util_do_config();
phonenumber_config_t *pn_util_parse_config(char *str);
phonenumber_action_t *pn_util_parse_actions(char *str);
void pn_util_exec(phonenumber_action_t *actions, phonenumber_request_t *request);
phonenumber_action_t pn_util_match_action_function(char *action);
PhoneNumberUtil::PhoneNumberFormat pn_util_str_to_format(char *format);
const char *pn_util_format_to_str(PhoneNumberUtil::PhoneNumberFormat format);
phonenumber_scope pn_util_str_to_scope(char *scope);
const char *pn_util_scope_to_str(phonenumber_scope scope);
phonenumber_direction pn_util_str_to_direction(char *direction);
const char *pn_util_direction_to_str(phonenumber_direction direction);

#endif /* MOD_PHONENUMBER_H */
