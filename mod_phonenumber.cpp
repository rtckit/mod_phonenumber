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

#include <stdexcept>
#include <stdio.h>

using namespace std;

#include <switch.h>

#include "mod_phonenumber.h"

/**
 * FreeSWITCH module definitions
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_phonenumber_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_phonenumber_shutdown);
SWITCH_MODULE_DEFINITION(mod_phonenumber, mod_phonenumber_load, mod_phonenumber_shutdown, NULL);

/**
 * Default configuration
 *
 * This configuration (or portions of it) will be applied in all circumstances
 * where explicit action configuration options are not passed.
 */
phonenumber_config_t mod_phonenumber_config;

/**
 * Configure hooks (state handling)
 *
 * Any valid hook configuration defined in phonenumber.conf.xml will be parsed
 * out and appended to the mod_phonenumber_hooks list.
 */
phonenumber_hook_t *mod_phonenumber_hooks = NULL;

/**
 * PhoneNumberUtil singleton
 */
const PhoneNumberUtil &phone_util = *PhoneNumberUtil::GetInstance();

/**
 * Application interface function
 *
 * Implements the phonenumber dialplan application.
 */
SWITCH_STANDARD_APP(phonenumber_app_function)
{
  char *mycmd = NULL;
  int argc = 0;
  char *argv[10] = { 0 };

  char *number = NULL;
  char *number_caller = NULL;
  char *number_destination = NULL;

  phonenumber_request_t request;
  phonenumber_action_t *actions = NULL;

  switch_channel_t *channel = switch_core_session_get_channel(session);

  if (zstr(data)) {
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Invalid syntax!\n");
  }

  switch_strdup(mycmd, data);

  if ((argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) < 1) {
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Invalid syntax!\n");
  }

  actions = pn_util_parse_actions(argv[0]);

  if (argc >= 2) {
    number = argv[1];

    if (!strncasecmp(number, PN_CALLER, PN_LEN_CALLER)) {
      switch_caller_profile_t *profile = switch_channel_get_caller_profile(channel);
      switch_strdup(number_caller, profile->orig_caller_id_number);
      number = NULL;
    } else if (!strncasecmp(number, PN_DESTINATION, PN_LEN_DESTINATION)) {
      switch_caller_profile_t *profile = switch_channel_get_caller_profile(channel);
      switch_strdup(number_destination, profile->destination_number);
      number = NULL;
    } else if (!strncasecmp(number, PN_ALL, PN_LEN_ALL)) {
      switch_caller_profile_t *profile = switch_channel_get_caller_profile(channel);
      switch_strdup(number_caller, profile->orig_caller_id_number);
      switch_strdup(number_destination, profile->destination_number);
      number = NULL;
    }
  } else {
    switch_caller_profile_t *profile = switch_channel_get_caller_profile(channel);
    switch_strdup(number_caller, profile->orig_caller_id_number);
    switch_strdup(number_destination, profile->destination_number);
  }

  if (actions) {
    request.config = pn_util_parse_config(argv[2]);
    request.channel = channel;
    request.stream = NULL;
    request.prefix = NULL;

    if (!zstr(number)) {
      request.number = number;
      switch_strdup(request.prefix, PN_NUMBER);

      pn_util_exec(actions, &request);

      switch_safe_free(request.prefix);
    }

    if (!zstr(number_caller)) {
      request.number = number_caller;
      switch_strdup(request.prefix, PN_CALLER);

      pn_util_exec(actions, &request);

      switch_safe_free(request.prefix);
      switch_safe_free(number_caller);
    }

    if (!zstr(number_destination)) {
      request.number = number_destination;
      switch_strdup(request.prefix, PN_DESTINATION);

      pn_util_exec(actions, &request);

      switch_safe_free(request.prefix);
      switch_safe_free(number_destination);
    }

    switch_safe_free(request.config);
    switch_safe_free(actions);
  }

  switch_safe_free(mycmd);
}

/**
 * API interface function
 *
 * Implements the phonenumber API command.
 */
SWITCH_STANDARD_API(phonenumber_api_function)
{
  char *mycmd = NULL;
  int argc = 0;
  char *argv[10] = { 0 };

  phonenumber_request_t request;
  phonenumber_action_t *actions = NULL;

  if (zstr(cmd)) {
    goto usage;
  }

  switch_strdup(mycmd, cmd);

  if ((argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) < 2) {
    goto usage;
  }

  actions = pn_util_parse_actions(argv[0]);
  if (!actions[0]) {
    goto usage;
  }

  request.number = argv[1];
  request.config = pn_util_parse_config(argv[2]);
  request.channel = NULL;
  request.stream = stream;
  request.prefix = NULL;

  pn_util_exec(actions, &request);
  switch_safe_free(request.config);
  switch_safe_free(actions);

  goto done;

usage:
  switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "Invalid syntax, correct usage: %s\n", PN_SYNTAX);
  stream->write_function(stream, "-ERR: Invalid syntax, correct usage: %s\n", PN_SYNTAX);

done:
  switch_safe_free(mycmd);

  return SWITCH_STATUS_SUCCESS;
}

/**
 * State handler
 *
 * The handler is invoked whenever a channel enters the CS_INIT state, in
 * order to power hooks defined in phonenumber.conf.xml.
 */
switch_status_t mod_phonenumber_on_init_handler(switch_core_session_t *session)
{
  switch_channel_t *channel = switch_core_session_get_channel(session);
  switch_caller_profile_t *profile = switch_channel_get_caller_profile(channel);

  phonenumber_hook_t *hook = mod_phonenumber_hooks;

  while (hook) {
    if (hook->context && strcmp(hook->context, profile->context)) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Context %s not covered by hook\n", profile->context);
      hook = hook->next;
      continue;
    }

    if (hook->direction != phonenumber_direction::DIRECTION_ALL) {
      if ((profile->direction == SWITCH_CALL_DIRECTION_INBOUND) && (hook->direction != phonenumber_direction::DIRECTION_INBOUND)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Direction inbound not covered by hook\n");
        hook = hook->next;
        continue;
      }

      if ((profile->direction == SWITCH_CALL_DIRECTION_OUTBOUND) && (hook->direction != phonenumber_direction::DIRECTION_OUTBOUND)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Direction outbound not covered by hook\n");
        hook = hook->next;
        continue;
      }
    }

    phonenumber_request_t request;

    request.config = &hook->config;
    request.channel = channel;
    request.stream = NULL;
    request.prefix = NULL;

    if ((hook->scope == phonenumber_scope::SCOPE_ALL) || (hook->scope == phonenumber_scope::SCOPE_CALLER)) {
      request.number = (char *)profile->orig_caller_id_number;
      switch_strdup(request.prefix, PN_CALLER);

      pn_util_exec(hook->actions, &request);

      switch_safe_free(request.prefix);
    }

    request.config = &hook->config;

    if ((hook->scope == phonenumber_scope::SCOPE_ALL) || (hook->scope == phonenumber_scope::SCOPE_DESTINATION)) {
      request.number = (char *)profile->destination_number;
      switch_strdup(request.prefix, PN_DESTINATION);

      pn_util_exec(hook->actions, &request);

      switch_safe_free(request.prefix);
    }

    hook = hook->next;
  }

  return SWITCH_STATUS_SUCCESS;
}

/**
 * State handler table
 */
switch_state_handler_table_t mod_phonenumber_state_handlers = {
  /*.on_init */ mod_phonenumber_on_init_handler,
  /*.on_routing */ NULL,
  /*.on_execute */ NULL,
  /*.on_hangup */ NULL,
  /*.on_exchange_media */ NULL,
  /*.on_soft_execute */ NULL,
  /*.on_consume_media */ NULL,
  /*.on_hibernate */ NULL,
  /*.on_reset */ NULL,
  /*.on_park */ NULL,
  /*.on_reporting */ NULL
};

/**
 * Module load routine
 *
 * Handles module's initialization:
 * - sets up the dialplan application;
 * - sets up the API interface;
 * - configures the API autocomplete;
 * - populates the default configuration;
 * - installs the state handler (if there are defined hooks);
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_phonenumber_load)
{
  switch_application_interface_t *app_interface;
  switch_api_interface_t *api_interface;

  *module_interface = switch_loadable_module_create_module_interface(pool, modname);

  SWITCH_ADD_APP(app_interface, "phonenumber", "Look up phone number", "Look up phone number", phonenumber_app_function, PN_SYNTAX, SAF_ROUTING_EXEC | SAF_SUPPORT_NOMEDIA);
  SWITCH_ADD_API(api_interface, "phonenumber", "phonenumber", phonenumber_api_function, PN_SYNTAX);

  switch_console_set_complete("add phonenumber");
  switch_console_set_complete("add phonenumber is_alpha_number");
  switch_console_set_complete("add phonenumber convert_alpha_characters_in_number");
  switch_console_set_complete("add phonenumber normalize_digits_only");
  switch_console_set_complete("add phonenumber normalize_diallable_chars_only");
  switch_console_set_complete("add phonenumber get_national_significant_number");
  switch_console_set_complete("add phonenumber format_out_of_country_calling_number");
  switch_console_set_complete("add phonenumber format");
  switch_console_set_complete("add phonenumber get_number_type");
  switch_console_set_complete("add phonenumber is_valid_number_for_region");
  switch_console_set_complete("add phonenumber get_region_code");
  switch_console_set_complete("add phonenumber is_possible_number_with_reason");
  switch_console_set_complete("add phonenumber is_possible_number");
  switch_console_set_complete("add phonenumber get_description_for_number");

  if (pn_util_do_config() != SWITCH_STATUS_SUCCESS) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot configure module!\n");
    return SWITCH_STATUS_TERM;
  }

  if (mod_phonenumber_hooks) {
    if (switch_core_add_state_handler(&mod_phonenumber_state_handlers) == -1) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot setup state hanlder!\n");
      return SWITCH_STATUS_TERM;
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

/**
 * Module unload routine
 *
 * Prepares the module for shutdown:
 * - removes the state handler (if installed);
 * - flushes the hook list;
 */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_phonenumber_shutdown)
{
  phonenumber_hook_t *curr = mod_phonenumber_hooks, *next = NULL;

  if (mod_phonenumber_hooks) {
    switch_core_remove_state_handler(&mod_phonenumber_state_handlers);
  }

  while (curr) {
    next = curr->next;

    if (curr->context)
      switch_safe_free(curr->context);

    if (curr->actions)
      switch_safe_free(curr->actions);

    switch_safe_free(curr);
    curr = next;
  }
  mod_phonenumber_hooks = NULL;

  return SWITCH_STATUS_SUCCESS;
}
