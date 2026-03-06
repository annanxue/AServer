# AServer

A cross-platform game server framework written in C++ with Lua scripting support.

## Features

- **Cross-platform**: Supports Windows and Linux
- **Modular Design**: Various modules for different functionalities
- **Lua Integration**: Built-in Lua 5.1 support for scripting
- **Network Management**: TCP socket handling with multi-client support
- **Timer System**: Built-in timer management
- **Logging**: Flexible logging system with multiple levels

## Modules

- **Game Server Module**: Core game logic
- **DB Client Module**: Database client connection
- **BF Client Module**: Battle Front client connection
- **Login Client Module**: Login server connection
- **GM Client Module**: GM tool connection
- **Log Client Module**: Log server connection
- **GM Server Module**: GM server functionality
- **Lua Module**: Lua scripting integration
- **Timer Module**: Timer management
- **Net Module**: Network management
- **Log Module**: Logging functionality

## Requirements

- C++17 compatible compiler
- CMake 3.15+
- Windows SDK or Linux development tools

## Build

### Windows (Visual Studio)

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Linux

```bash
mkdir build
cd build
cmake ..
make
```

## Project Structure

```
AServer/
├── include/          # Header files
│   ├── core/         # Core components
│   └── modules/      # Module headers
├── src/              # Source files
│   ├── core/         # Core implementations
│   └── modules/      # Module implementations
├── lua51/            # Lua 5.1 source code
├── build/            # Build output
└── CMakeLists.txt    # CMake build configuration
```

## Usage

The server can be started after building. Configuration and scripting can be customized through Lua files.

## License

MIT License
