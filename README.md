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

set {
    histlogger {
        whitelist {}; /* If set to any will only allow these channels to have hisotry */
        blacklist {}; /* If set to any those channels wont have any history */
        dbpath "/home/irc/histlog.db"; /* Database to store messages and other info for the module */
        regonly 1; /* Allow registered users only to use this feature */
    }
}

```

Then run `/rehash` as an oper on the irc server. No need for restart!
