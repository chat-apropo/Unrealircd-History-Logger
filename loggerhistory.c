/* This is the history logger module
 * Copyright (c) Matheus Fillipe
 * Its purpose is to keep a log of all channels and mark the points the users left the channels
 * so that a per user basis log can be sent to the user.
 * It was made so i wouldn't need to use a bouncer or proxy for the app
 * https://github.com/matheusfillipe/chat-apropo
 */

#include "unrealircd.h"

#define MAX_LENGTH               2000 /* Max length of a history per channel to keep */

ModuleHeader MOD_HEADER
= {
        "third/loggerhistory", /* name */
        "0.0.1", /* version */
        "Logger for channels", /* description */
        "Matheus Fillipe", /* author */
        "unrealircd-6", /* do not change this, it indicates module API version */
};

// History command
CMD_FUNC(command);

int on_channel_send(Client *client, Channel *channel, MessageTag *mtags, const char *text, SendType	sendtype);
int on_join(Client *client, Channel *channel, MessageTag *mtags);
int on_part(Client *client, Channel *channel, MessageTag *mtags, const char *comment);
int on_quit(Client *client, MessageTag *mtags, const char *comment);

MOD_TEST()
{
    /* Test here */
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
  sendto_one(client, NULL, ":%s hello world %s", me.name, client->name);
}


int on_channel_send(Client *client, Channel *channel, MessageTag *mtags, const char *text, SendType	sendtype) {
  // Admin is mattf user
  Client *admin = find_client("mattf", NULL);
  sendto_one(admin, NULL, "%s:%s - %s", channel->name, client->name, text);
  return 0;
}

int on_join(Client *client, Channel *channel, MessageTag *mtags) {
  // Admin is mattf user
  Client *admin = find_client("mattf", NULL);
  sendto_one(admin, NULL, "%s joined %s", client->name, channel->name);
  return 0;
}

int on_part(Client *client, Channel *channel, MessageTag *mtags, const char *comment) {
  // Admin is mattf user
  Client *admin = find_client("mattf", NULL);
  sendto_one(admin, NULL, "%s left %s", client->name, channel->name);
  return 0;
}

int on_quit(Client *client, MessageTag *mtags, const char *comment) {
  // Admin is mattf user
  Client *admin = find_client("mattf", NULL);
  sendto_one(admin, NULL, "%s quit", client->name);
  // Also send channels he was in
  User *user = client->user;
  if (user) {
    Membership *membership = user->channel;
    while (membership!=NULL) {
      Channel *channel = membership->channel;
      sendto_one(admin, NULL, "%s left %s", client->name, channel->name);
      membership = membership->next;
    }
  }
  return 0;
}

