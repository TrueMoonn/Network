# Protocol Generator - JSON Format Documentation

## Basic Structure

```json
{
  "endianness": "little",
  "preambule": {
    "active": true,
    "characters": "\r\t\r\t"
  },
  "packet_length": {
    "active": true,
    "length": 4
  },
  "datetime": {
    "active": false
  },
  "end_of_packet": {
    "active": true,
    "characters": "\r\n"
  },
  "structs": {},
  "messages": {}
}
```

## Protocol Configuration

### Endianness

Specify byte order for multi-byte values.

```json
"endianness": "little"
```

Valid values: `"big"` or `"little"`

### Protocol Envelope

Optional protocol framing elements.

```json
"preambule": {
  "active": true,
  "characters": "\r\t\r\t"
},
"packet_length": {
  "active": true,
  "length": 4
},
"datetime": {
  "active": false
},
"end_of_packet": {
  "active": true,
  "characters": "\r\n"
}
```

## Data Types

### Primitive Types

#### Unsigned Integers

```json
{"name": "player_id", "type": "uint8"}
{"name": "port", "type": "uint16"}
{"name": "session_id", "type": "uint32"}
{"name": "large_value", "type": "uint64"}
```

#### Signed Integers

```json
{"name": "offset", "type": "int8"}
{"name": "delta", "type": "int16"}
{"name": "balance", "type": "int32"}
{"name": "timestamp", "type": "int64"}
```

#### Floating Point

```json
{"name": "temperature", "type": "float"}
{"name": "precise_value", "type": "double"}
```

### Strings

Fixed-size character arrays.

```json
{"name": "username", "type": "string", "max_length": 32}
{"name": "message", "type": "string", "max_length": 256}
```

## Structs

Define reusable data structures.

```json
"structs": {
  "PlayerData": {
    "fields": [
      {"name": "entity_id", "type": "uint32"},
      {"name": "pos_x", "type": "float"},
      {"name": "pos_y", "type": "float"},
      {"name": "health", "type": "uint32"},
      {"name": "name", "type": "string", "max_length": 32}
    ]
  }
}
```

Generated C++ code:

```cpp
struct PlayerData {
    uint32_t entity_id;
    float pos_x;
    float pos_y;
    uint32_t health;
    char name[32];
};
```

## Arrays

### Fixed Array

Array with maximum size, serializes only the count specified by a corresponding count field.

```json
{
  "name": "player_count", "type": "uint8"
},
{
  "name": "players",
  "type": "fixed_array",
  "element_type": "PlayerData",
  "max_size": 4
}
```

Generated C++ code:

```cpp
uint8_t player_count;
PlayerData players[4];
```

Usage:

```cpp
msg.player_count = 2;
msg.players[0].entity_id = 1;
msg.players[1].entity_id = 2;
// Only 2 elements are serialized
```

Count field naming convention:
- `players` requires `player_count`
- `enemies` requires `enemy_count`

### Fixed Array of Primitives

```json
{
  "name": "value_count", "type": "uint8"
},
{
  "name": "values",
  "type": "fixed_array",
  "element_type": "uint32",
  "max_size": 10
}
```

### Dynamic Array

Array with variable size using std::vector.

```json
{
  "name": "ids",
  "type": "dynamic_array",
  "element_type": "uint32"
}
```

Generated C++ code:

```cpp
std::vector<uint32_t> ids;
```

Usage:

```cpp
msg.ids.push_back(10);
msg.ids.push_back(20);
msg.ids.push_back(30);
```

### Dynamic Array of Structs

```json
{
  "name": "scores",
  "type": "dynamic_array",
  "element_type": "Score"
}
```

### Dynamic Array of Strings

```json
{
  "name": "tags",
  "type": "dynamic_array",
  "element_type": "string"
}
```

Generated C++ code:

```cpp
std::vector<std::string> tags;
```

## Messages

Define network message types with unique IDs.

### Simple Message

```json
"messages": {
  "PING": {
    "id": 1,
    "fields": [
      {"name": "timestamp", "type": "uint64"}
    ]
  }
}
```

### Message with Struct Array

```json
"PLAYERS_DATA": {
  "id": 40,
  "fields": [
    {"name": "player_count", "type": "uint8"},
    {"name": "players", "type": "fixed_array", "element_type": "PlayerData", "max_size": 4}
  ]
}
```

### Complex Message

```json
"SERVER_INFO": {
  "id": 100,
  "fields": [
    {"name": "session_id", "type": "uint64"},
    {"name": "server_name", "type": "string", "max_length": 64},
    {"name": "player_count", "type": "uint8"},
    {"name": "players", "type": "fixed_array", "element_type": "PlayerData", "max_size": 4},
    {"name": "max_score", "type": "uint32"}
  ]
}
```

# Protocol Generator - Complete Example

## Input: protocol.json

```json
{
  "endianness": "little",
  "preambule": {
    "active": true,
    "characters": "\r\t\r\t"
  },
  "packet_length": {
    "active": true,
    "length": 4
  },
  "datetime": {
    "active": false
  },
  "end_of_packet": {
    "active": true,
    "characters": "\r\n"
  },
  
  "structs": {
    "PlayerData": {
      "fields": [
        {"name": "entity_id", "type": "uint32"},
        {"name": "pos_x", "type": "float"},
        {"name": "pos_y", "type": "float"},
        {"name": "health", "type": "uint32"}
      ]
    }
  },
  
  "messages": {
    "LOGIN": {
      "id": 1,
      "fields": [
        {"name": "username", "type": "string", "max_length": 32},
        {"name": "password", "type": "string", "max_length": 64}
      ]
    },
    "GAME_STATE": {
      "id": 10,
      "fields": [
        {"name": "player_count", "type": "uint8"},
        {"name": "players", "type": "fixed_array", "element_type": "PlayerData", "max_size": 4}
      ]
    }
  }
}
```

## Generate Code

```bash
python3 tools/generate_protocol.py config/protocol.json
```

## Output: generated_messages.hpp

```cpp
#pragma once
#include <cstdint>
#include <vector>
#include <cstring>

namespace net {

// ===== Structs =====

struct PlayerData {
    uint32_t entity_id;
    float pos_x;
    float pos_y;
    uint32_t health;
};

// ===== Messages =====

struct LOGIN {
    static constexpr uint32_t ID = 1;

    char username[32];
    char password[64];

    std::vector<uint8_t> serialize() const;
    static LOGIN deserialize(const std::vector<uint8_t>& data);
};

struct GAME_STATE {
    static constexpr uint32_t ID = 10;

    uint8_t player_count;
    PlayerData players[4];

    std::vector<uint8_t> serialize() const;
    static GAME_STATE deserialize(const std::vector<uint8_t>& data);
};

}  // namespace net
```

## Output: generated_messages.cpp

```cpp
#include "Network/generated_messages.hpp"

namespace net {

std::vector<uint8_t> LOGIN::serialize() const {
    std::vector<uint8_t> buffer;

    // Write message ID
    buffer.push_back(ID & 0xFF);
    buffer.push_back((ID >> 8) & 0xFF);
    buffer.push_back((ID >> 16) & 0xFF);
    buffer.push_back((ID >> 24) & 0xFF);

    // Write username
    for (uint32_t i = 0; i < 32; ++i) {
        buffer.push_back(username[i]);
    }

    // Write password
    for (uint32_t i = 0; i < 64; ++i) {
        buffer.push_back(password[i]);
    }

    return buffer;
}

LOGIN LOGIN::deserialize(const std::vector<uint8_t>& data) {
    LOGIN msg;
    size_t offset = 0;

    // Skip message ID
    offset += 4;

    // Read username
    std::memcpy(msg.username, data.data() + offset, 32);
    offset += 32;

    // Read password
    std::memcpy(msg.password, data.data() + offset, 64);
    offset += 64;

    return msg;
}

std::vector<uint8_t> GAME_STATE::serialize() const {
    std::vector<uint8_t> buffer;

    // Write message ID
    buffer.push_back(ID & 0xFF);
    buffer.push_back((ID >> 8) & 0xFF);
    buffer.push_back((ID >> 16) & 0xFF);
    buffer.push_back((ID >> 24) & 0xFF);

    // Write player_count
    buffer.push_back(player_count);

    // Write players
    for (uint32_t i = 0; i < player_count && i < 4; ++i) {
        buffer.push_back(players[i].entity_id & 0xFF);
        buffer.push_back((players[i].entity_id >> 8) & 0xFF);
        buffer.push_back((players[i].entity_id >> 16) & 0xFF);
        buffer.push_back((players[i].entity_id >> 24) & 0xFF);
        {
            uint32_t temp;
            std::memcpy(&temp, &players[i].pos_x, sizeof(float));
            buffer.push_back(temp & 0xFF);
            buffer.push_back((temp >> 8) & 0xFF);
            buffer.push_back((temp >> 16) & 0xFF);
            buffer.push_back((temp >> 24) & 0xFF);
        }
        {
            uint32_t temp;
            std::memcpy(&temp, &players[i].pos_y, sizeof(float));
            buffer.push_back(temp & 0xFF);
            buffer.push_back((temp >> 8) & 0xFF);
            buffer.push_back((temp >> 16) & 0xFF);
            buffer.push_back((temp >> 24) & 0xFF);
        }
        buffer.push_back(players[i].health & 0xFF);
        buffer.push_back((players[i].health >> 8) & 0xFF);
        buffer.push_back((players[i].health >> 16) & 0xFF);
        buffer.push_back((players[i].health >> 24) & 0xFF);
    }

    return buffer;
}

GAME_STATE GAME_STATE::deserialize(const std::vector<uint8_t>& data) {
    GAME_STATE msg;
    size_t offset = 0;

    // Skip message ID
    offset += 4;

    // Read player_count
    msg.player_count = data[offset];
    offset += 1;

    // Read players
    for (uint32_t i = 0; i < msg.player_count && i < 4; ++i) {
        msg.players[i].entity_id = 
            static_cast<uint32_t>(data[offset]) |
            (static_cast<uint32_t>(data[offset + 1]) << 8) |
            (static_cast<uint32_t>(data[offset + 2]) << 16) |
            (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;
        {
            uint32_t temp;
            temp = static_cast<uint32_t>(data[offset]) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                (static_cast<uint32_t>(data[offset + 2]) << 16) |
                (static_cast<uint32_t>(data[offset + 3]) << 24);
            offset += 4;
            std::memcpy(&msg.players[i].pos_x, &temp, sizeof(float));
        }
        {
            uint32_t temp;
            temp = static_cast<uint32_t>(data[offset]) |
                (static_cast<uint32_t>(data[offset + 1]) << 8) |
                (static_cast<uint32_t>(data[offset + 2]) << 16) |
                (static_cast<uint32_t>(data[offset + 3]) << 24);
            offset += 4;
            std::memcpy(&msg.players[i].pos_y, &temp, sizeof(float));
        }
        msg.players[i].health = 
            static_cast<uint32_t>(data[offset]) |
            (static_cast<uint32_t>(data[offset + 1]) << 8) |
            (static_cast<uint32_t>(data[offset + 2]) << 16) |
            (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;
    }

    return msg;
}

}  // namespace net
```

## Usage Example: Client Side

```cpp
#include "Network/generated_messages.hpp"
#include <iostream>
#include <cstring>

int main() {
    // Create LOGIN message
    net::LOGIN login;
    std::strcpy(login.username, "player1");
    std::strcpy(login.password, "mypassword123");
    
    // Serialize to bytes
    auto loginData = login.serialize();
    
    std::cout << "LOGIN message serialized to " 
              << loginData.size() << " bytes" << std::endl;
    // Output: LOGIN message serialized to 100 bytes
    // (4 bytes ID + 32 bytes username + 64 bytes password)
    
    // Send over network
    // send(socket, loginData.data(), loginData.size());
    
    // Receive and deserialize
    auto receivedLogin = net::LOGIN::deserialize(loginData);
    
    std::cout << "Received username: " << receivedLogin.username << std::endl;
    std::cout << "Received password: " << receivedLogin.password << std::endl;
    
    return 0;
}
```

## Usage Example: Server Side

```cpp
#include "Network/generated_messages.hpp"
#include <iostream>

void sendGameState(int clientSocket) {
    // Create GAME_STATE message
    net::GAME_STATE state;
    state.player_count = 3;
    
    // Player 1
    state.players[0].entity_id = 1;
    state.players[0].pos_x = 100.0f;
    state.players[0].pos_y = 200.0f;
    state.players[0].health = 100;
    
    // Player 2
    state.players[1].entity_id = 2;
    state.players[1].pos_x = 150.0f;
    state.players[1].pos_y = 250.0f;
    state.players[1].health = 75;
    
    // Player 3
    state.players[2].entity_id = 3;
    state.players[2].pos_x = 200.0f;
    state.players[2].pos_y = 300.0f;
    state.players[2].health = 50;
    
    // Serialize (only 3 players are serialized, not all 4)
    auto stateData = state.serialize();
    
    std::cout << "GAME_STATE serialized to " 
              << stateData.size() << " bytes" << std::endl;
    // Output: GAME_STATE serialized to 53 bytes
    // (4 bytes ID + 1 byte count + 3 * 16 bytes per player)
    
    // Send over network
    // send(clientSocket, stateData.data(), stateData.size());
}

void handleReceivedState(const std::vector<uint8_t>& data) {
    auto state = net::GAME_STATE::deserialize(data);
    
    std::cout << "Received " << (int)state.player_count << " players:" << std::endl;
    
    for (uint8_t i = 0; i < state.player_count; ++i) {
        std::cout << "  Player " << state.players[i].entity_id 
                  << " at (" << state.players[i].pos_x 
                  << ", " << state.players[i].pos_y << ")"
                  << " HP: " << state.players[i].health << std::endl;
    }
}
```

## Wire Format

### LOGIN message (100 bytes total)

```
Offset | Size | Field
-------|------|-------
0      | 4    | Message ID (1, little-endian)
4      | 32   | username (null-padded)
36     | 64   | password (null-padded)
```

### GAME_STATE message (variable size)

```
Offset | Size      | Field
-------|-----------|-------
0      | 4         | Message ID (10, little-endian)
4      | 1         | player_count (n)
5      | n * 16    | players array
```

Each PlayerData element (16 bytes):

```
Offset | Size | Field
-------|------|-------
0      | 4    | entity_id (little-endian)
4      | 4    | pos_x (float, little-endian)
8      | 4    | pos_y (float, little-endian)
12     | 4    | health (little-endian)
```

Example with 3 players: 4 + 1 + (3 * 16) = 53 bytes
