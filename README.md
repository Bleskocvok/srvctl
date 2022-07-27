# srvctl

## Installation

```bash
git clone https://github.com/Bleskocvok/srvctl

cd srvctl

make install

srvctl help
```

## Uninstall

To uninstall, you only need to remove the two executable files and
the `.srvctl` directory.

```bash
rm /usr/local/bin/srvctl /usr/local/bin/srvd
rm -r ~/.srvctl
```

## Usage

The following snippet demonstrates the basic functionality. If you perform these
commands, the last one should output `hello world` in the terminal.

```bash
# create a single-entry config file
echo '{ "echo": { "dir": ".", "start": "echo hello world", "update": "echo update" } }' > ~/.srvctl/.apps.json

# start daemon, which loads the config file
srvd

# tell the daemon to start the app named ‹echo›
srvctl start echo

# view the output of the app
cat ~/.srvctl/echo.stdout.log
```

## Dependencies

- `deps/json.hpp`: https://github.com/nlohmann/json
