## Flags

* `-q` \
Don't output any messages. \
Note that if this is used, but stdout/stderr is passed to any other option that takes a path, that option will still output to stdout/stderr.

* `--dump-tokens <path>` \
Dump tokens to `<path>` \
`<path>` may be a character stream such as /std/out \
Options: \
`-human` output the tokens in a human readable format \
`-exit` stop the compiler after outputting tokens

* `--dump-diagnostics <path>` \
Dump diagnostics in a machine readable format to `<path>` \
`<path>` may be a character stream such as /std/out \
Options: \
`-source <path(s)>` specify a path or list of paths delimited by commas to retrieve diagnostics from
  