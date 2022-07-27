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

## Commands

```
Usage: srvctl CMD [ARG]

COMMANDS

srvctl list 
    List each apps loaded from the configuration file
    If an instance is running, PID is listed.
    If the app had been stopped, information about
    signal/return is listed.

srvctl signal ‹APP› ‹SIGNAL› 
    Send given signal to given running app.

srvctl start ‹APP› 
    Start app by given name.
    This app name must be present in 
    the respective configuration file.

srvctl stop ‹APP› 
    Stop a running instance of app of the given name.
    It must be running.
    The app is stopped by sending SIGKILL.
srvctl update ‹APP› 
    Update a given app. If the app is currently running,
    it is first stopped as if by command ‹stop›.

CONFIGURATION FORMAT

The configuration is located in ‹~/.srvctl›.
It uses the ‹json› format. Each application entry should
look as following:

"APP_NAME": {
    "dir": "/ABS/PATH/TO/DIR",
    "start": "[CMD] /ABS/PATH/TO/EXECUTABLE",
    "update": "UPDATE CMD"
}
```

## Dependencies

- `deps/json.hpp`: https://github.com/nlohmann/json
