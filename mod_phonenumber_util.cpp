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

#include "phonenumbers/phonenumber.pb.h"
#include "phonenumbers/phonenumberutil.h"

using i18n::phonenumbers::PhoneNumberUtil;

#include "mod_phonenumber.h"

/**
 * Configuration parser
 *
 * Parses phonenumber.conf.xml, creates the default configuration and sets up
 * hooks (to be used by the CS_INIT state handler).
 *
 * @return Whether or not we succeeded configuring the module.
 */
switch_status_t pn_util_do_config()
{
  const char *cf = "phonenumber.conf";
  switch_xml_t cfg, xml, settings, param, hooks, hook_cfg;
  phonenumber_hook_t *hook = NULL;

  strcpy(mod_phonenumber_config.default_region, PN_DEFAULT_REGION);
  mod_phonenumber_config.format = PN_DEFAULT_FORMAT;
  strcpy(mod_phonenumber_config.locale, PN_DEFAULT_LOCALE);
  strcpy(mod_phonenumber_config.calling_from, PN_DEFAULT_CALLING_FROM);

  if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot open %s\n", cf);
    return SWITCH_STATUS_TERM;
  }

  if ((settings = switch_xml_child(cfg, "settings"))) {
    for (param = switch_xml_child(settings, "param"); param; param = param->next) {
      char *var = (char *)switch_xml_attr_soft(param, "name");
      char *val = (char *)switch_xml_attr_soft(param, "value");

      if (!strncmp(var, PN_PARAM_DEFAULT_REGION, PN_PARAM_LEN_DEFAULT_REGION)) {
        if (zstr(val) || (strlen(val) != 2)) {
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid default region: %s\n", val);
        } else {
          strcpy(mod_phonenumber_config.default_region, val);
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured default region: %s\n", mod_phonenumber_config.default_region);
        }
      } else if (!strncmp(var, PN_PARAM_FORMAT, PN_PARAM_LEN_FORMAT)) {
        mod_phonenumber_config.format = pn_util_str_to_format(val);
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured format: %s\n", pn_util_format_to_str(mod_phonenumber_config.format));
      } else if (!strncmp(var, PN_PARAM_LOCALE, PN_PARAM_LEN_LOCALE)) {
        if (zstr(val) || (strlen(val) != 5)) {
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid locale: %s\n", val);
        } else {
          strcpy(mod_phonenumber_config.locale, val);
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured locale: %s\n", mod_phonenumber_config.locale);
        }
      } else if (!strncmp(var, PN_PARAM_CALLING_FROM, PN_PARAM_LEN_CALLING_FROM)) {
        if (zstr(val) || (strlen(val) != 2)) {
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid calling from region: %s\n", val);
        } else {
          strcpy(mod_phonenumber_config.calling_from, val);
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured calling from region: %s\n", mod_phonenumber_config.calling_from);
        }
      } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unknown configuration parameter %s\n", var);
      }
    }
  }

  if ((hooks = switch_xml_child(cfg, "hooks"))) {
    for (hook_cfg = switch_xml_child(hooks, "hook"); hook_cfg; hook_cfg = hook_cfg->next) {
      if (!mod_phonenumber_hooks) {
        mod_phonenumber_hooks = (phonenumber_hook_t *)malloc(sizeof(phonenumber_hook_t));
        hook = mod_phonenumber_hooks;
      } else {
        hook->next = (phonenumber_hook_t *)malloc(sizeof(phonenumber_hook_t));
        hook = hook->next;
      }

      if (!hook) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot create phonenumber hook, possibly OOM!\n");
        return SWITCH_STATUS_TERM;
      }

      hook->context = NULL;
      hook->direction = phonenumber_direction::DIRECTION_ALL;
      hook->scope = phonenumber_scope::SCOPE_ALL;
      hook->config = mod_phonenumber_config;
      hook->actions = NULL;
      hook->next = NULL;

      for (param = switch_xml_child(hook_cfg, "param"); param; param = param->next) {
        char *var = (char *)switch_xml_attr_soft(param, "name");
        char *val = (char *)switch_xml_attr_soft(param, "value");

        if (!strncmp(var, PN_PARAM_DIRECTION, PN_PARAM_LEN_DIRECTION)) {
          hook->direction = pn_util_str_to_direction(val);
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured hook direction: %s\n", pn_util_direction_to_str(hook->direction));
        } else if (!strncmp(var, PN_PARAM_CONTEXT, PN_PARAM_LEN_CONTEXT)) {
          switch_strdup(hook->context, val);
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured hook context: %s\n", hook->context);
        } else if (!strncmp(var, PN_PARAM_SCOPE, PN_PARAM_LEN_SCOPE)) {
          hook->scope = pn_util_str_to_scope(val);
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured hook scope: %s\n", pn_util_scope_to_str(hook->scope));
        } else if (!strncmp(var, PN_PARAM_ACTIONS, PN_PARAM_LEN_ACTIONS)) {
          hook->actions = pn_util_parse_actions(val);
        } else if (!strncmp(var, PN_PARAM_DEFAULT_REGION, PN_PARAM_LEN_DEFAULT_REGION)) {
          if (zstr(val) || (strlen(val) != 2)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid hook default region: %s\n", val);
          } else {
            strcpy(hook->config.default_region, val);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured hook default region: %s\n", hook->config.default_region);
          }
        } else if (!strncmp(var, PN_PARAM_FORMAT, PN_PARAM_LEN_FORMAT)) {
          hook->config.format = pn_util_str_to_format(val);
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured hook format: %s\n", pn_util_format_to_str(hook->config.format));
        } else if (!strncmp(var, PN_PARAM_LOCALE, PN_PARAM_LEN_LOCALE)) {
          if (zstr(val) || (strlen(val) != 5)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid hook locale: %s\n", val);
          } else {
            strcpy(hook->config.locale, val);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured hook locale: %s\n", hook->config.locale);
          }
        } else if (!strncmp(var, PN_PARAM_CALLING_FROM, PN_PARAM_LEN_CALLING_FROM)) {
          if (zstr(val) || (strlen(val) != 2)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid hook calling from region: %s\n", val);
          } else {
            strcpy(hook->config.calling_from, val);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Configured hook calling from region: %s\n", hook->config.calling_from);
          }
        } else {
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unknown hook configuration parameter %s\n", var);
        }
      }
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

/**
 * String configuration parser
 *
 * Parses out any configuration parameters which may have been passed when
 * the module is invoked via the dialplan application or through the API. It
 * eventually falls back to any configured default values for any undefined
 * parameters.
 *
 * @param str String to be parsed
 * @return Parsed configuration
 */
phonenumber_config_t *pn_util_parse_config(char *str)
{
  int i, argc = 0;
  char *argv[10] = { 0 }, *tuple[2] = { 0 };
  phonenumber_config_t *config;

  config = (phonenumber_config_t *)malloc(sizeof(*config));
  if (!config) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unable to parse configuration, possibly OOM!\n");
    return NULL;
  }

  *config = mod_phonenumber_config;

  if (!zstr(str)) {
    argc = switch_separate_string(str, ',', argv, 4);
    for (i = 0; i < argc; i++) {
      if (switch_separate_string(argv[i], '=', tuple, 2) == 2) {
        if (!strncasecmp(tuple[0], PN_PARAM_DEFAULT_REGION, PN_PARAM_LEN_DEFAULT_REGION)) {
          if (zstr(tuple[1]) || (strlen(tuple[1]) != 2)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid default region: %s\n", tuple[1]);
          } else {
            strcpy(config->default_region, tuple[1]);
          }
        } else if (!strncasecmp(tuple[0], PN_PARAM_FORMAT, PN_PARAM_LEN_FORMAT)) {
          config->format = pn_util_str_to_format(tuple[1]);
        } else if (!strncmp(tuple[0], PN_PARAM_LOCALE, PN_PARAM_LEN_LOCALE)) {
          if (zstr(tuple[1]) || (strlen(tuple[1]) != 5)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid locale: %s\n", tuple[1]);
          } else {
            strcpy(config->locale, tuple[1]);
          }
        } else if (!strncmp(tuple[0], PN_PARAM_CALLING_FROM, PN_PARAM_LEN_CALLING_FROM)) {
          if (zstr(tuple[1]) || (strlen(tuple[1]) != 2)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid calling from region: %s\n", tuple[1]);
          } else {
            strcpy(config->calling_from, tuple[1]);
          }
        } else {
          switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unknown configuration argument %s\n", tuple[0]);
        }
      } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unable to parse configuration argument %s\n", argv[i]);
      }
    }
  }

  return config;
}

/**
 * Action parser
 *
 * Parses out actions passed when the module is invoked via the dialplan\
 * application or through the API.
 *
 * @param str String to be parsed
 * @return Array of parsed actions
 */
phonenumber_action_t *pn_util_parse_actions(char *str)
{
  int actc, i, j = 0;
  char *actv[PN_MAX_ACTIONS] = { NULL };
  phonenumber_action_t *actions = (phonenumber_action_t *)malloc(sizeof(phonenumber_action_t) * PN_MAX_ACTIONS);

  if (!actions) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unable to parse the actions, possibly OOM!\n");
    return NULL;
  }

  if (!(actc = switch_separate_string(str, ',', actv, PN_MAX_ACTIONS))) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot parse out any actions: %s\n", str);
  } else {
    for (i = 0; i < actc; i++) {
      actions[j] = pn_util_match_action_function(actv[i]);
      if (!actions[j]) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unknown action: %s\n", actv[i]);
      } else {
        j++;
      }
    }
  }

  if (j < PN_MAX_ACTIONS) {
    actions[j] = NULL;
  }

  return actions;
}

/**
 * Action executor
 *
 * Executes an array of actions over a request.
 *
 * @param actions Array of parsed actions
 * @param request Request to action on
 */
void pn_util_exec(phonenumber_action_t *actions, phonenumber_request_t *request)
{
  int actc = 0;

  if (request->channel && request->prefix) {
    switch_channel_set_variable_name_printf(request->channel, request->number, "phonenumber_%s_input", request->prefix);
  }

  if (actions) {
    request->parsed = new PhoneNumber;
    phone_util.Parse(request->number, request->config->default_region, request->parsed);

    while (actions[actc] && (actc < PN_MAX_ACTIONS)) {
      actions[actc++](request);
    }

    delete request->parsed;
  }
}

/**
 * Action matcher
 *
 * Determines whether a given string represents and implemented action, and
 * returns a pointer to it.
 *
 * @param action String to be matched
 * @return Pointer to matched action
 */
phonenumber_action_t pn_util_match_action_function(char *action)
{
  if (!strncasecmp(action, PN_ACTION_IS_ALPHA_NUMBER, PN_ACTION_LEN_IS_ALPHA_NUMBER)) {
    return is_alpha_number;
  } else if (!strncasecmp(action, PN_ACTION_CONVERT_ALPHA_CHARACTERS_IN_NUMBER, PN_ACTION_LEN_CONVERT_ALPHA_CHARACTERS_IN_NUMBER)) {
    return convert_alpha_characters_in_number;
  } else if (!strncasecmp(action, PN_ACTION_NORMALIZE_DIGITS_ONLY, PN_ACTION_LEN_NORMALIZE_DIGITS_ONLY)) {
    return normalize_digits_only;
  } else if (!strncasecmp(action, PN_ACTION_NORMALIZE_DIALLABLE_CHARS_ONLY, PN_ACTION_LEN_NORMALIZE_DIALLABLE_CHARS_ONLY)) {
    return normalize_diallable_chars_only;
  } else if (!strncasecmp(action, PN_ACTION_GET_NATIONAL_SIGNIFICANT_NUMBER, PN_ACTION_LEN_GET_NATIONAL_SIGNIFICANT_NUMBER)) {
    return get_national_significant_number;
  } else if (!strncasecmp(action, PN_ACTION_FORMAT_OUT_OF_COUNTRY_CALLING_NUMBER, PN_ACTION_LEN_FORMAT_OUT_OF_COUNTRY_CALLING_NUMBER)) {
    return format_out_of_country_calling_number;
  } else if (!strncasecmp(action, PN_ACTION_FORMAT, PN_ACTION_LEN_FORMAT)) {
    return format;
  } else if (!strncasecmp(action, PN_ACTION_GET_NUMBER_TYPE, PN_ACTION_LEN_GET_NUMBER_TYPE)) {
    return get_number_type;
  } else if (!strncasecmp(action, PN_ACTION_IS_VALID_NUMBER_FOR_REGION, PN_ACTION_LEN_IS_VALID_NUMBER_FOR_REGION)) {
    return is_valid_number_for_region;
  } else if (!strncasecmp(action, PN_ACTION_GET_REGION_CODE, PN_ACTION_LEN_GET_REGION_CODE)) {
    return get_region_code;
  } else if (!strncasecmp(action, PN_ACTION_IS_POSSIBLE_NUMBER_WITH_REASON, PN_ACTION_LEN_IS_POSSIBLE_NUMBER_WITH_REASON)) {
    return is_possible_number_with_reason;
  } else if (!strncasecmp(action, PN_ACTION_IS_POSSIBLE_NUMBER, PN_ACTION_LEN_IS_POSSIBLE_NUMBER)) {
    return is_possible_number;
  } else if (!strncasecmp(action, PN_ACTION_GET_DESCRIPTION_FOR_NUMBER, PN_ACTION_LEN_GET_DESCRIPTION_FOR_NUMBER)) {
    return get_description_for_number;
  } else {
    return NULL;
  }
}

/**
 * Format matcher
 *
 * Matches a string representing a phone number format to its libphonenumber
 * representation. If no match is found, it falls back to the configured
 * default.
 *
 * @param format String to match
 * @return libphonenumber format
 */
PhoneNumberUtil::PhoneNumberFormat pn_util_str_to_format(char *format)
{
  if (zstr(format))
    return mod_phonenumber_config.format;

  if (!strncasecmp(format, PN_FORMAT_E164, PN_FORMAT_LEN_E164)) {
    return PhoneNumberUtil::E164;
  } else if (!strncasecmp(format, PN_FORMAT_INTERNATIONAL, PN_FORMAT_LEN_INTERNATIONAL)) {
    return PhoneNumberUtil::INTERNATIONAL;
  } else if (!strncasecmp(format, PN_FORMAT_NATIONAL, PN_FORMAT_LEN_NATIONAL)) {
    return PhoneNumberUtil::NATIONAL;
  } else if (!strncasecmp(format, PN_FORMAT_RFC3966, PN_FORMAT_LEN_RFC3966)) {
    return PhoneNumberUtil::RFC3966;
  } else {
    return mod_phonenumber_config.format;
  }
}

/**
 * Format string converter
 *
 * Converts a libphonenumber format to its string representation.
 *
 * @param format libphonenumber format
 * @return String representation
 */
const char *pn_util_format_to_str(PhoneNumberUtil::PhoneNumberFormat format)
{
  switch (format) {
  case PhoneNumberUtil::E164:
    return PN_FORMAT_E164;
  case PhoneNumberUtil::INTERNATIONAL:
    return PN_FORMAT_INTERNATIONAL;
  case PhoneNumberUtil::NATIONAL:
    return PN_FORMAT_NATIONAL;
  case PhoneNumberUtil::RFC3966:
    return PN_FORMAT_RFC3966;
  default:
    return pn_util_format_to_str(mod_phonenumber_config.format);
  }
}

/**
 * Scope matcher
 *
 * Matches a string representing a scope (caller, destination or all) to its
 * internal representation. If no match is possible, it defaults to all.
 *
 * @param scope String to match
 * @return Internal representation
 */
phonenumber_scope pn_util_str_to_scope(char *scope)
{
  if (zstr(scope))
    return phonenumber_scope::SCOPE_ALL;

  if (!strncasecmp(scope, PN_CALLER, PN_LEN_CALLER)) {
    return phonenumber_scope::SCOPE_CALLER;
  } else if (!strncasecmp(scope, PN_DESTINATION, PN_LEN_DESTINATION)) {
    return phonenumber_scope::SCOPE_DESTINATION;
  } else {
    return phonenumber_scope::SCOPE_ALL;
  }
}

/**
 * Scope string converter
 *
 * Converts a scope's internal representation to a string.
 *
 * @param scope Internal scope representation
 * @return String representation
 */
const char *pn_util_scope_to_str(phonenumber_scope scope)
{
  switch (scope) {
  case phonenumber_scope::SCOPE_CALLER:
    return PN_CALLER;
  case phonenumber_scope::SCOPE_DESTINATION:
    return PN_DESTINATION;
  default:
    return PN_ALL;
  }
}

/**
 * Direction matcher
 *
 * Matches a string representing a direction (inbound, outbound or all) to its
 * internal representation. If no match is possible, it defaults to all.
 *
 * @param direction String to match
 * @return Internal representation
 */
phonenumber_direction pn_util_str_to_direction(char *direction)
{
  if (zstr(direction))
    return phonenumber_direction::DIRECTION_ALL;

  if (!strncasecmp(direction, PN_INBOUND, PN_LEN_INBOUND)) {
    return phonenumber_direction::DIRECTION_INBOUND;
  } else if (!strncasecmp(direction, PN_OUTBOUND, PN_LEN_OUTBOUND)) {
    return phonenumber_direction::DIRECTION_OUTBOUND;
  } else {
    return phonenumber_direction::DIRECTION_ALL;
  }
}

/**
 * Direction string converter
 *
 * Converts a direction's internal representation to a string.
 *
 * @param direction Internal direction representation
 * @return String representation
 */
const char *pn_util_direction_to_str(phonenumber_direction direction)
{
  switch (direction) {
  case phonenumber_direction::DIRECTION_INBOUND:
    return PN_INBOUND;
  case phonenumber_direction::DIRECTION_OUTBOUND:
    return PN_OUTBOUND;
  default:
    return PN_ALL;
  }
}
