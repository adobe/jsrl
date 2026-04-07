# JSRL - JSON Serialization and Reading Library

A modern C++ JSON library designed around UTF-8 correctness, immutability, and high-fidelity number representation.

## Overview

JSRL is a production-proven JSON parser and encoder. Originally developed for
Adobe Analytics' high-throughput data processing infrastructure, JSRL
prioritizes correctness, thread safety through immutability, and exact numeric
representation.

**Key Design Principles:**
- **UTF-8 Native**: All strings are UTF-8 with automatic validation
- **Immutable by Design**: JSON entities are immutable and reference-counted
- **High-Fidelity Numbers**: Exact decimal representation preserves precision during round-trips
- **Zero-Copy Semantics**: Cheap-to-copy handles via shared_ptr internally
- **Thread-Safe Reading**: Immutable structures enable concurrent reads without locks

## Usage

Below is some sample usage code. More samples exist in the [quickstart](docs/QUICKSTART.md) docs.

```cpp
#include "jsrl.hpp"
#include <iostream>
#include <sstream>

using namespace jsrl;
using namespace jsrl::literals;

int main() {
    // Parse JSON from string
    std::istringstream iss(R"({"name": "John", "age": 30, "active": true})");
    Json json;
    iss >> json;

    // Or use string literals
    auto json2 = R"({"items": [1, 2, 3]})"_Json;

    // Access data
    std::cout << json["name"].as_string() << std::endl;  // John
    std::cout << json["age"].as_number_sint() << std::endl;  // 30
    std::cout << json["active"].as_bool() << std::endl;  // true

    // Safe access with defaults
    std::string country = json.get_string("country", "US");

    // Iterate arrays
    for (auto const& item : json2["items"].as_array()) {
        std::cout << item.as_number_sint() << " ";
    }
    // Output: 1 2 3

    // Iterate objects
    for (auto const& [key, value] : json.as_object()) {
        std::cout << key << ": " << value << std::endl;
    }

    // Output JSON
    std::cout << json << std::endl;  // Compact: {"name":"John",...}
    std::cout << pretty_print(json).indent("  ") << std::endl;  // Formatted

    return 0;
}
```

### C++20 std::format Support (Optional)

If building with C++20 or later, JSRL provides `std::formatter` specializations for all JSON types:

```cpp
#include "jsrl_format.hpp"  // Include for std::format support
#include <format>

using namespace jsrl;

int main() {
    Json json = R"({"status": "ok", "count": 42})"_Json;

    // Use std::format with Json objects
    std::string msg = std::format("Response: {}", json);
    // msg = "Response: {"status":"ok","count":42}"

    // Works with all jsrl types
    GeneralNumber num(123);
    std::cout << std::format("Value: {}", num) << std::endl;

    // Pretty-print with std::format
    auto pretty = pretty_print(json);
    std::cout << std::format("{}", pretty) << std::endl;

    return 0;
}
```

**Note:** To enable C++20 features, build with:
```bash
cmake -DCMAKE_CXX_STANDARD=20 ..
```

## Building from Source

### Requirements

- C++17 or later
- CMake 3.14 or later
- A C++ compiler (GCC, Clang, MSVC, or Apple Clang)
- Google Test (automatically downloaded if not found)

### Basic Build

```bash
# Clone the repository (or download the source)
cd jsrl

# Create a build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the library and tests
cmake --build . -j

# Run tests
ctest --output-on-failure
```

To build as a shared library instead:

```bash
cmake -DBUILD_SHARED_LIBS=ON ..
cmake --build . -j
ctest --output-on-failure
```

### Installation

To install JSRL system-wide:

```bash
cd build
cmake --build . --target install

# Or specify a custom installation prefix
cmake -DCMAKE_INSTALL_PREFIX=/custom/path ..
cmake --build . --target install
```

The installation will include:
- Headers in `<prefix>/include/jsrl/`
- Library files in `<prefix>/lib/`
- CMake config files in `<prefix>/lib/cmake/jsrl/`

### Using JSRL in Your Project

#### With CMake (Installed)

If you've installed JSRL, you can use it in your CMake project:

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyProject)

find_package(jsrl REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE jsrl::jsrl)
```

#### With CMake (As a Subdirectory)

You can also include JSRL directly as a subdirectory:

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyProject)

# Add JSRL as a subdirectory
add_subdirectory(external/jsrl)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE jsrl::jsrl)
```

## License

Copyright 2025 Adobe

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

---

*Production-proven JSON for modern C++*
