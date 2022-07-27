# srvctl

## Installation

```bash
git clone https://github.com/Bleskocvok/srvctl
cd srvctl
make install
srvctl help
```

## Usage

The following snippet demonstrates the basic functionality. If you perform these
commands, it should output `hello world` in the terminal.

```bash
echo '{ "echo": { "dir": ".", "start": "echo hello world", "update": "echo update" } }' >> ~/.srvctl/.apps.json

srvd

srvctl start echo

cat ~/.srvctl/echo.stdout.log
```

## Dependencies

- `deps/json.hpp`: https://github.com/nlohmann/json
