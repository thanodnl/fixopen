# Fixopen
Fixopen is a small tool to fix interrupted systemcalls on OSX Big Sur when using postgres.

It works by installing a dynamic linked library in the postgres bin directory together with a small bash script to spawn postgres with the linked library preloaded.

The linked library has a wrapper around the `open` function from the standard library to keep retrying the open call as long as the error is `EINTR` (upto 1000 times for safety).

## Use
With postgres on your path run the following:

```
make install
```

Now when invoking `pg_ctl` add a `-p /path/to/postgres/bin/postgres.fixopen`, or if `postgres` is on your path add `-p $(which postgres.fixopen)`. Tooling could check for the existance of `postgres.fixopen` and automatically add the `-p ... ` flag to its invocation of `pg_ctl`.