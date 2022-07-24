/* This is the history logger module
 * Copyright (c) Matheus Fillipe
 * Its purpose is to keep a log of all channels and mark the points the users left the channels
 * so that a per user basis log can be sent to the user.
 * It was made so i wouldn't need to use a bouncer or proxy for the app
 * https://github.com/matheusfillipe/chat-apropo
 */

#include <stdarg.h>
#include "unrealircd.h"

#define MAX_LENGTH 2000 /* Default Max length of a history per channel to keep */
#define ENABLE_DEBUG true
#define LOGTO_USER "mattf" /* User to send the log to */

ModuleHeader MOD_HEADER
= {
        "third/histlogger", /* name */
        "0.0.1", /* version */
        "Logger for channels", /* description */
        "Matheus Fillipe", /* author */
        "unrealircd-6", /* do not change this, it indicates module API version */
};

struct HistLoggerConfig {
  NameList *whitelist;
  NameList *blacklist;
  char *dbpath;
  int regonly;
  int maxlength;
} config;

int config_dbpath_set = 0;
int config_regonly_set = 0;
int config_maxlength_set = 0;


// Message user
void vmessage_user(char *user, char *msg, va_list args) {
  Client *client = find_client(user, NULL);
  if(!client){
    return;
  }
  vsendto_one(client, NULL, msg, args);
}

void message_user(char *user, char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vmessage_user(user, msg, args);
  va_end(args);
}

// Stupid print debug that will send messages to a user
void mdebug(char *msg, ...){
  if(!ENABLE_DEBUG){
    return;
  }
  va_list args;
  va_start(args, msg);
  vmessage_user(LOGTO_USER, msg, args);
  va_end(args);
}

// History command
CMD_FUNC(command);

int on_channel_send(Client *client, Channel *channel, MessageTag *mtags, const char *text, SendType	sendtype);
int on_join(Client *client, Channel *channel, MessageTag *mtags);
int on_part(Client *client, Channel *channel, MessageTag *mtags, const char *comment);
int on_quit(Client *client, MessageTag *mtags, const char *comment);

int hist_logger_config_test(ConfigFile *cf, ConfigEntry *ce, int type, int *errs);
int hist_logger_config_posttest(int *errs);


MOD_TEST()
{
    /* Test here */
    HookAdd(modinfo->handle, HOOKTYPE_CONFIGTEST, 0, hist_logger_config_test);
    HookAdd(modinfo->handle, HOOKTYPE_CONFIGPOSTTEST, 0, hist_logger_config_posttest);
    return MOD_SUCCESS;
}

MOD_INIT()
{
    /* Called on module initalization. Most module objects are added here: HookAdd()/CommandAdd()/EventAdd()/etc. */
    CommandAdd(modinfo->handle, "LOG", command, MAXPARA, CMD_USER);
    HookAdd(modinfo->handle, HOOKTYPE_PRE_CHANMSG, 0, on_channel_send);
    HookAdd(modinfo->handle, HOOKTYPE_LOCAL_JOIN, 0, on_join);
    HookAdd(modinfo->handle, HOOKTYPE_LOCAL_PART, 0, on_part);
    HookAdd(modinfo->handle, HOOKTYPE_LOCAL_QUIT, 0, on_quit);
    return MOD_SUCCESS;
}

MOD_LOAD()
{
    /* Do necessary initialization for when module is loaded */
    /* For example: CommandOverrideAdd() */
    return MOD_SUCCESS; /* returning anything else is not really supported here */
}

MOD_UNLOAD()
{
	// Perform any cleanup for unload
	return MOD_SUCCESS;
}

CMD_FUNC(command){
  mdebug(":%s hello world %s", me.name, client->name);
}

int hist_logger_config_test(ConfigFile *cf, ConfigEntry *ce, int type, int *errs) {
  int errors = 0;
  ConfigEntry *cep, *cep2, *value;
  if (type != CONFIG_SET)
      return 0;

  /* We are only interrested in set::histlogger... */
  if (!ce || !ce->name || strcmp(ce->name, "histlogger"))
      return 0; // return 0 to indicate: we don't know / we don't handle this!

  for (cep = ce->items; cep; cep = cep->next) {
    if (!cep->name) {
      config_error("%s:%i: blank set::mymod item",
          cep->file->filename, cep->line_number);
      errors++;
      continue;
    }
    else if (!strcmp(cep->name, "whitelist")) {
      for (value = cep->items; value; value = value->next) {
        add_name_list(config.whitelist, value->name);
      }
    }
    else if (!strcmp(cep->name, "blacklist")) {
      for (value = cep->items; value; value = value->next) {
        add_name_list(config.blacklist, value->name);
      }
    }
    else if (!strcmp(cep->name, "dbpath")) {
      if (!cep->value) {
        config_error("%s:%i: set::histlogger::dbpath is empty",
            cep->file->filename, cep->line_number);
        errors++;
      }
      UnrealDB *db = NULL;
      db = unrealdb_open(cep->value, UNREALDB_MODE_READ, NULL);
      if (!db) {
        // File doesn't exist, lets create it
        if (unrealdb_get_error_code() == UNREALDB_ERROR_FILENOTFOUND) {
          db = unrealdb_open(cep->value, UNREALDB_MODE_WRITE, NULL);
          if (!db) {
            config_error("%s:%i: could not open database at set::histlogger::dbpath: %s",
              cep->file->filename, cep->line_number, unrealdb_get_error_string());
            errors++;
          } else {
            unrealdb_close(db);
          }
        } else {
          config_error("%s:%i: could not open database at set::histlogger::dbpath: %s",
            cep->file->filename, cep->line_number, unrealdb_get_error_string());
          errors++;
        }
      } else {
        unrealdb_close(db);
      }
      config_dbpath_set = 1;
      config.dbpath = cep->value;
    }
    else if (!strcmp(cep->name, "regonly")) {
      config.regonly = atoi(cep->value);
      if (config.regonly == 0) {
        config_error("%s:%i: set::histlogger::regonly is not a number greater than 0",
            cep->file->filename, cep->line_number);
        errors++;
      } else {
        config_regonly_set = 1;
      }
    }
    else if (!strcmp(cep->name, "maxlength")) {
      config.maxlength = atoi(cep->value);
      if (config.maxlength == 0) {
        config_error("%s:%i: set::histlogger::maxlength is not a number greater than 0",
            cep->file->filename, cep->line_number);
        errors++;
      } else {
        config_maxlength_set = 1;
      }
    }
    else {
      config_error("%s:%i: set::histlogger has unknown item '%s'",
          cep->file->filename, cep->line_number, cep->name);
      errors++;
    }
  }

  *errs = errors;
  return errors ? -1 : 1;
}

int hist_logger_config_posttest(int *errs) {
  int errors = 0;

  if (!config_dbpath_set) {
    config_error("set::mymod::dbpath is not set");
    errors++; 
  }
  if (!config_regonly_set) {
    config.regonly = 1;
  }
  if (!config_maxlength_set) {
    config.maxlength = MAX_LENGTH;
  }
  return errors;
}

int on_channel_send(Client *client, Channel *channel, MessageTag *mtags, const char *text, SendType	sendtype) {
  mdebug("%s:%s - %s", channel->name, client->name, text);
  return 0;
}

int on_join(Client *client, Channel *channel, MessageTag *mtags) {
  mdebug("%s joined %s", client->name, channel->name);
  return 0;
}

int on_part(Client *client, Channel *channel, MessageTag *mtags, const char *comment) {
  mdebug("%s left %s  -  isReg: %d", client->name, channel->name, IsRegNick(client));
  return 0;
}

int on_quit(Client *client, MessageTag *mtags, const char *comment) {
  mdebug("%s quit", client->name);
  // Also send channels he was in
  User *user = client->user;
  if (user) {
    Membership *membership = user->channel;
    while (membership!=NULL) {
      Channel *channel = membership->channel;
      mdebug("%s left %s", client->name, channel->name);
      membership = membership->next;
    }
  }
  return 0;
}

