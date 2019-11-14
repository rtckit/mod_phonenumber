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

#include <stdio.h>

using namespace std;

#include "phonenumbers/geocoding/phonenumber_offline_geocoder.h"
#include "phonenumbers/phonenumber.pb.h"

using i18n::phonenumbers::PhoneNumber;
using i18n::phonenumbers::PhoneNumberOfflineGeocoder;

#include "mod_phonenumber.h"

/**
 * is_alpha_number action
 *
 * Returns true if the number is a valid vanity (alpha) number such as 800
 * MICROSOFT. A valid vanity number will start with at least 3 digits and will
 * have three or more alpha characters. This does not do region-specific
 * checks.
 */
PN_ACTION(is_alpha_number)
{
  char response[6];

  strcpy(response, phone_util.IsAlphaNumber(request->number) ? "true" : "false");

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, response, "phonenumber_%s_is_alpha_number", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_is_alpha_number := %s\n", request->prefix, response);
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", response);
  }
}

/**
 * convert_alpha_characters_in_number action
 *
 * Converts all alpha characters in a number to their respective digits on
 * a keypad, but retains existing formatting.
 */
PN_ACTION(convert_alpha_characters_in_number)
{
  string converted = request->number;

  phone_util.ConvertAlphaCharactersInNumber(&converted);

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, converted.c_str(), "phonenumber_%s_alpha_characters_in_number", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_alpha_characters_in_number := %s\n", request->prefix, converted.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", converted.c_str());
  }
}

/**
 * normalize_digits_only action
 *
 * Normalizes a string of characters representing a phone number. This
 * converts wide-ascii and arabic-indic numerals to European numerals, and
 * strips punctuation and alpha characters.
 */
PN_ACTION(normalize_digits_only)
{
  string normalized = request->number;

  phone_util.NormalizeDigitsOnly(&normalized);

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, normalized.c_str(), "phonenumber_%s_digits_only", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_digits_only := %s\n", request->prefix, normalized.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", normalized.c_str());
  }
}

/**
 * normalize_diallable_chars_only action
 *
 * Normalizes a string of characters representing a phone number. This strips
 * all characters which are not diallable on a mobile phone keypad (including
 * all non-ASCII digits).
 */
PN_ACTION(normalize_diallable_chars_only)
{
  string normalized = request->number;

  phone_util.NormalizeDiallableCharsOnly(&normalized);

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, normalized.c_str(), "phonenumber_%s_diallable_chars_only", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_diallable_chars_only := %s\n", request->prefix, normalized.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", normalized.c_str());
  }
}

/**
 * get_national_significant_number action
 *
 * Gets the national significant number of a phone number. Note a national
 * significant number doesn't contain a national prefix or any formatting.
 */
PN_ACTION(get_national_significant_number)
{
  string national_significant_num;

  phone_util.GetNationalSignificantNumber(*(request->parsed), &national_significant_num);

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, national_significant_num.c_str(), "phonenumber_%s_national_significant_number", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_national_significant_number := %s\n", request->prefix, national_significant_num.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", national_significant_num.c_str());
  }
}

/**
 * format_out_of_country_calling_number action
 *
 * Formats a phone number for out-of-country dialing purposes.
 * Note this function takes care of the case for calling inside of NANPA
 * and between Russia and Kazakhstan (who share the same country calling
 * code). In those cases, no international prefix is used. For regions which
 * have multiple international prefixes, the number in its INTERNATIONAL
 * format will be returned instead.
 */
PN_ACTION(format_out_of_country_calling_number)
{
  string formatted;

  phone_util.FormatOutOfCountryCallingNumber(*(request->parsed), request->config->calling_from, &formatted);

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, formatted.c_str(), "phonenumber_%s_out_of_country_calling_number", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_out_of_country_calling_number := %s\n", request->prefix, formatted.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", formatted.c_str());
  }
}

/**
 * format action
 *
 * Formats a phone number in the specified format using default rules. Note
 * that this does not promise to produce a phone number that the user can
 * dial from where they are - although we do format in either NATIONAL or
 * INTERNATIONAL format depending on what the client asks for, we do not
 * currently support a more abbreviated format, such as for users in the
 * same area who could potentially dial the number without area code.
 */
PN_ACTION(format)
{
  string formatted;

  phone_util.Format(*(request->parsed), request->config->format, &formatted);

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, formatted.c_str(), "phonenumber_%s_format", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_format := %s\n", request->prefix, formatted.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", formatted.c_str());
  }
}

/**
 * get_number_type action
 *
 * Gets the type of a valid phone number, or UNKNOWN if it is invalid.
 */
PN_ACTION(get_number_type)
{
  char response[21];

  PhoneNumberUtil::PhoneNumberType type = phone_util.GetNumberType(*(request->parsed));

  switch (type) {
  case PhoneNumberUtil::FIXED_LINE:
    strcpy(response, "FIXED_LINE");
    break;
  case PhoneNumberUtil::FIXED_LINE_OR_MOBILE:
    strcpy(response, "FIXED_LINE_OR_MOBILE");
    break;
  case PhoneNumberUtil::MOBILE:
    strcpy(response, "MOBILE");
    break;
  case PhoneNumberUtil::PAGER:
    strcpy(response, "PAGER");
    break;
  case PhoneNumberUtil::PERSONAL_NUMBER:
    strcpy(response, "PERSONAL_NUMBER");
    break;
  case PhoneNumberUtil::PREMIUM_RATE:
    strcpy(response, "PREMIUM_RATE");
    break;
  case PhoneNumberUtil::SHARED_COST:
    strcpy(response, "SHARED_COST");
    break;
  case PhoneNumberUtil::TOLL_FREE:
    strcpy(response, "TOLL_FREE");
    break;
  case PhoneNumberUtil::UAN:
    strcpy(response, "UAN");
    break;
  case PhoneNumberUtil::UNKNOWN:
    strcpy(response, "UNKNOWN");
    break;
  case PhoneNumberUtil::VOICEMAIL:
    strcpy(response, "VOICEMAIL");
    break;
  case PhoneNumberUtil::VOIP:
    strcpy(response, "VOIP");
    break;
  default:
    strcpy(response, "UNKNOWN");
    break;
  }

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, response, "phonenumber_%s_number_type", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_number_type := %s\n", request->prefix, response);
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", response);
  }
}

/**
 * is_valid_number_for_region action
 *
 * Tests whether a phone number is valid for a certain region.
 */
PN_ACTION(is_valid_number_for_region)
{
  char response[6];

  strcpy(response, phone_util.IsValidNumberForRegion(*(request->parsed), request->config->default_region) ? "true" : "false");

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, response, "phonenumber_%s_valid_number_for_region", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_valid_number_for_region := %s\n", request->prefix, response);
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", response);
  }
}

/**
 * get_region_code action
 *
 * Returns the region where a phone number is from. This could be used for
 * geocoding at the region level. Only guarantees correct results for valid,
 * full numbers (not short-codes, or invalid numbers). Returns "ZZ" when no
 * match is possible.
 */
PN_ACTION(get_region_code)
{
  string region_code;

  phone_util.GetRegionCodeForNumber(*(request->parsed), &region_code);

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, region_code.c_str(), "phonenumber_%s_region_code", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_region_code := %s\n", request->prefix, region_code.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", region_code.c_str());
  }
}

/**
 * is_possible_number_with_reason action
 *
 * Checks whether a phone number is a possible number, with a level of detail.
 */
PN_ACTION(is_possible_number_with_reason)
{
  char response[23];

  PhoneNumberUtil::ValidationResult reason = phone_util.IsPossibleNumberWithReason(*(request->parsed));

  switch (reason) {
  case PhoneNumberUtil::ValidationResult::IS_POSSIBLE:
    strcpy(response, "IS_POSSIBLE");
    break;
  case PhoneNumberUtil::ValidationResult::INVALID_COUNTRY_CODE:
    strcpy(response, "INVALID_COUNTRY_CODE");
    break;
  case PhoneNumberUtil::ValidationResult::TOO_SHORT:
    strcpy(response, "TOO_SHORT");
    break;
  case PhoneNumberUtil::ValidationResult::TOO_LONG:
    strcpy(response, "TOO_LONG");
    break;
  default:
    strcpy(response, "UNKNOWN");
    break;
  }

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, response, "phonenumber_%s_is_possible_number_with_reason", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_is_possible_number_with_reason := %s\n", request->prefix, response);
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", response);
  }
}

/**
 * is_possible_number action
 *
 * Checks whether a phone number is a possible number.
 */
PN_ACTION(is_possible_number)
{
  char response[6];

  strcpy(response, phone_util.IsValidNumber(*(request->parsed)) ? "true" : "false");

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, response, "phonenumber_%s_is_possible_number", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_is_possible_number := %s\n", request->prefix, response);
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", response);
  }
}

/**
 * get_description_for_number action
 *
 * Returns a text description for the given phone number, in the locale
 * provided. The description might consist of the name of the country where
 * the phone number is from, or the name of the geographical area the phone
 * number is from if more detailed information is available. Returns an empty
 * string if the number could come from multiple countries, or the country
 * code is in fact invalid.
 */
PN_ACTION(get_description_for_number)
{
  string description = PhoneNumberOfflineGeocoder().GetDescriptionForNumber(*(request->parsed), icu::Locale(request->config->locale));

  if (request->channel) {
    switch_channel_set_variable_name_printf(request->channel, description.c_str(), "phonenumber_%s_description_for_number", request->prefix);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "phonenumber_%s_description_for_number := %s\n", request->prefix, description.c_str());
  }

  if (request->stream) {
    request->stream->write_function(request->stream, "%s\n", description.c_str());
  }
}
