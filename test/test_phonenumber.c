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

#include <test/switch_test.h>

#define PN_EXPECT(cmd, args, expect)                                                                        \
  switch_api_execute(cmd, args, NULL, &stream);                                                             \
  if (stream.data) {                                                                                        \
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s(%s): %s\n", cmd, args, (char *)stream.data); \
    fst_check(strstr(stream.data, expect) == stream.data);                                                  \
  }                                                                                                         \
  stream.end = stream.data;

FST_CORE_BEGIN("conf")
{
  FST_MODULE_BEGIN(mod_phonenumber, mod_phonenumber_test)
  {
    FST_SETUP_BEGIN()
    {
      fst_requires_module("mod_phonenumber");
    }
    FST_SETUP_END()

    FST_TEST_BEGIN(is_alpha_number)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "is_alpha_number +18006427676", "false");
      PN_EXPECT("phonenumber", "is_alpha_number +1800MICROSOFT", "true");
      PN_EXPECT("phonenumber", "is_alpha_number 800FLOWERS", "true");
      PN_EXPECT("phonenumber", "is_alpha_number 034-56&+a#234", "false");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(convert_alpha_characters_in_number)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "convert_alpha_characters_in_number +18006427676", "+18006427676");
      PN_EXPECT("phonenumber", "convert_alpha_characters_in_number +1800MICROSOFT", "+1800642767638");
      PN_EXPECT("phonenumber", "convert_alpha_characters_in_number 800FLOWERS", "8003569377");
      PN_EXPECT("phonenumber", "convert_alpha_characters_in_number 034-56&+a#234", "034-56&+2#234");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(normalize_digits_only)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "normalize_digits_only 034-56&+a#234", "03456234");
      PN_EXPECT("phonenumber", "normalize_digits_only 1800MICROSOFT", "1800");
      PN_EXPECT("phonenumber", "normalize_digits_only 1800MÍCRÓSÖFT", "1800");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(normalize_diallable_chars_only)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "normalize_diallable_chars_only 034-56&+a#234", "03456+234");
      PN_EXPECT("phonenumber", "normalize_diallable_chars_only 03*4-56&+a#234", "03*456+234");
      PN_EXPECT("phonenumber", "normalize_diallable_chars_only 1800MICROSOFT", "1800");
      PN_EXPECT("phonenumber", "normalize_diallable_chars_only 1800MÍCRÓSÖFT", "1800");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(get_national_significant_number)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "get_national_significant_number +16172531000", "6172531000");
      PN_EXPECT("phonenumber", "get_national_significant_number +442076792000", "2076792000");
      PN_EXPECT("phonenumber", "get_national_significant_number 6172531000 default_region=US", "6172531000");
      PN_EXPECT("phonenumber", "get_national_significant_number 02076792000 default_region=GB", "2076792000");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(format)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "format +16172531000 format=E164", "+16172531000");
      PN_EXPECT("phonenumber", "format +16172531000 format=INTERNATIONAL", "+1 617-253-1000");
      PN_EXPECT("phonenumber", "format +16172531000 format=NATIONAL", "(617) 253-1000");
      PN_EXPECT("phonenumber", "format +16172531000 format=RFC3966", "tel:+1-617-253-1000");
      PN_EXPECT("phonenumber", "format 800FLOWERS default_region=US,format=E164", "+18003569377");
      PN_EXPECT("phonenumber", "format 800FLOWERS default_region=US,format=INTERNATIONAL", "+1 800-356-9377");
      PN_EXPECT("phonenumber", "format 800FLOWERS default_region=US,format=NATIONAL", "(800) 356-9377");
      PN_EXPECT("phonenumber", "format 800FLOWERS default_region=US,format=RFC3966", "tel:+1-800-356-9377");
      PN_EXPECT("phonenumber", "format '020 7679 2000' default_region=GB,format=E164", "+442076792000");
      PN_EXPECT("phonenumber", "format '020 7679 2000' default_region=GB,format=INTERNATIONAL", "+44 20 7679 2000");
      PN_EXPECT("phonenumber", "format '020 7679 2000' default_region=GB,format=NATIONAL", "020 7679 2000");
      PN_EXPECT("phonenumber", "format '020 7679 2000' default_region=GB,format=RFC3966", "tel:+44-20-7679-2000");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(format_out_of_country_calling_number)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "format_out_of_country_calling_number '(495) 939-10-00' default_region=RU,calling_from=RU", "8 (495) 939-10-00");
      PN_EXPECT("phonenumber", "format_out_of_country_calling_number '(495) 939-10-00' default_region=RU,calling_from=KZ", "8 (495) 939-10-00");
      PN_EXPECT("phonenumber", "format_out_of_country_calling_number '030 34637000' default_region=DE,calling_from=GB", "00 49 30 34637000");
      PN_EXPECT("phonenumber", "format_out_of_country_calling_number '030 34637000' default_region=DE,calling_from=US", "011 49 30 34637000");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(get_number_type)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "get_number_type +16172531000", "FIXED_LINE_OR_MOBILE");
      PN_EXPECT("phonenumber", "get_number_type +442076792000", "FIXED_LINE");
      PN_EXPECT("phonenumber", "get_number_type +447400982200", "MOBILE");
      PN_EXPECT("phonenumber", "get_number_type +18006427676", "TOLL_FREE");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(is_valid_number_for_region)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "is_valid_number_for_region 6172531000 default_region=US", "true");
      PN_EXPECT("phonenumber", "is_valid_number_for_region 6172531000 default_region=GB", "false");
      PN_EXPECT("phonenumber", "is_valid_number_for_region 07400982200 default_region=GB", "true");
      PN_EXPECT("phonenumber", "is_valid_number_for_region 07400982200 default_region=DE", "true");
      PN_EXPECT("phonenumber", "is_valid_number_for_region 07400982200 default_region=IT", "false");
      PN_EXPECT("phonenumber", "is_valid_number_for_region 800FLOWERS default_region=US", "true");
      PN_EXPECT("phonenumber", "is_valid_number_for_region 800FLOWERS default_region=CA", "true");
      PN_EXPECT("phonenumber", "is_valid_number_for_region 800FLOWERS default_region=JM", "true");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(get_region_code)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "get_region_code +16172531000", "US");
      PN_EXPECT("phonenumber", "get_region_code +16172531000 default_region=GB", "US");
      PN_EXPECT("phonenumber", "get_region_code +442076792000", "GB");
      PN_EXPECT("phonenumber", "get_region_code +442076792000 default_region=US", "GB");
      PN_EXPECT("phonenumber", "get_region_code 6172531000 default_region=US", "US");
      PN_EXPECT("phonenumber", "get_region_code 6172531000 default_region=GB", "ZZ");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(is_possible_number_with_reason)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "is_possible_number_with_reason +16172531000", "IS_POSSIBLE");
      PN_EXPECT("phonenumber", "is_possible_number_with_reason 253-1000", "IS_POSSIBLE");
      PN_EXPECT("phonenumber", "is_possible_number_with_reason +999237000", "INVALID_COUNTRY_CODE");
      PN_EXPECT("phonenumber", "is_possible_number_with_reason +1256300", "TOO_SHORT");
      PN_EXPECT("phonenumber", "is_possible_number_with_reason 077400982200 default_region=IT", "TOO_LONG");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(is_possible_number)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "is_possible_number +16172531000", "true");
      PN_EXPECT("phonenumber", "is_possible_number +999237000", "false");
      PN_EXPECT("phonenumber", "is_possible_number +1256300", "false");
      PN_EXPECT("phonenumber", "is_possible_number 077400982200 default_region=IT", "false");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEST_BEGIN(get_description_for_number)
    {
      switch_stream_handle_t stream = { 0 };

      SWITCH_STANDARD_STREAM(stream);

      PN_EXPECT("phonenumber", "get_description_for_number +16172531000", "Cambridge, MA");
      PN_EXPECT("phonenumber", "get_description_for_number +16172531000 default_region=GB", "Cambridge, MA");
      PN_EXPECT("phonenumber", "get_description_for_number +442076792000", "London");
      PN_EXPECT("phonenumber", "get_description_for_number +442076792000 default_region=US", "London");
      PN_EXPECT("phonenumber", "get_description_for_number 6172531000 default_region=US", "Cambridge, MA");
      PN_EXPECT("phonenumber", "get_description_for_number 6172531000 default_region=GB", "");
      PN_EXPECT("phonenumber", "get_description_for_number +18003569377", "United States");
      PN_EXPECT("phonenumber", "get_description_for_number +18003569377 locale=fr_FR", "États-Unis");
      PN_EXPECT("phonenumber", "get_description_for_number +18003569377 locale=es_ES", "Estados Unidos");
      PN_EXPECT("phonenumber", "get_description_for_number +447400982200", "United Kingdom");
      PN_EXPECT("phonenumber", "get_description_for_number +447400982200 locale=fr_FR", "Royaume-Uni");
      PN_EXPECT("phonenumber", "get_description_for_number +447400982200 locale=es_ES", "Reino Unido");

      switch_safe_free(stream.data);
    }
    FST_TEST_END()

    FST_TEARDOWN_BEGIN()
    {
    }
    FST_TEARDOWN_END()
  }
  FST_MODULE_END()
}
FST_CORE_END()
