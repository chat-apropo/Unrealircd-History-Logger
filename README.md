# Work in progress


# Chat History logger

Per user chat history. Only retrieve the history of what you missed in certain channel. Only allow registered users to have this feature.

## Building

Copy `loggerhistory.c` into unrealircd's `src/module/thirdy/loggerhistory.c` and build it with something like:

```bash
make && make install && chown -R irc /home/irc/unrealircd
```

## Configuring

Add this to your unrealircd.conf:

```config
loadmodule "third/loggerhistory";
```

Then run `/rehash` as an oper on the irc server. No need for restart!
