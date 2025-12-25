# TrueMoon Network module documentation

**The network module is separated in 3 big parts :**
- [SERVER](#server)
- [CLIENT](#client)
- [PROTOCOL](#protocol)

# SERVER

Separated in 2 parts, **TCP** protocol and **UDP** protocol

Some functions are only for one of the 2 protocols :
```
tcpReceive  ->  TCP
tcpSend     ->  TCP
udpReceive  ->  UDP
udpSend     ->  UDP
```
Each of these functions will throw an error if the other protocol is set when you try to use it

`tcpReceive` and `udpReceive` both take `timeout`, which corresponds to milliseconds to wait (blocking) for an input or new connection(s)

`udpReceive` also takes a `maxInputs`, to limit the number of incoming connections

## Main examples

### TCP

```
#include <iostream>
#include <vector>
#include "Network/Server.hpp"

int main() {
    net::Server server("TCP", 5000,
        net::ProtocolManager("config/protocol.json"));

    server.start();
    while (1) {
        std::vector<int> received = server.tcpReceive(5);

        for (auto& r : received) {
            std::vector<std::vector<uint8_t>> packets = server.unpack(r, -1);

            for (auto& p : packets) {
                p.resize(p.size());
                std::cout << "Received "
                          << p.size()
                          << " bytes from "
                          << r
                          << ": '";
                for (uint8_t byte : p)
                    std::cout << static_cast<char>(byte);
                std::cout << "'"
                          << std::endl;
            }
        }
    }
    server.stop();
    return 0;
}
```

### UDP

```
#include <iostream>
#include <vector>
#include "Network/Server.hpp"

int main() {
    net::Server server("UDP", 5000,
        net::ProtocolManager("config/protocol.json"));

    server.start();
    while (1) {
        std::vector<net::Address> received = server.udpReceive(5, 10);

        for (auto& r : received) {
            std::vector<std::vector<uint8_t>> packets = server.unpack(r, -1);

            for (auto& p : packets) {
                p.resize(p.size());
                std::cout << "Received "
                          << p.size()
                          << " bytes from "
                          << r.getIP()
                          << ":"
                          << r.getPort()
                          << " -> '";
                for (uint8_t byte : p)
                    std::cout << static_cast<char>(byte);
                std::cout << "'"
                          << std::endl;
            }
        }
    }
    server.stop();
    return 0;
}
```

# CLIENT

Separated in 2 parts, **TCP** protocol and **UDP** protocol

Some functions are only for one of the 2 protocols :
```
tcpReceive  ->  TCP
udpReceive  ->  UDP
```
Each of these functions will throw an error if the other protocol is set when you try to use it

`tcpReceive` and `udpReceive` both take `timeout`, which corresponds to milliseconds to wait (blocking) for an input or new connection(s)

`udpReceive` also takes a `maxInputs`, to limit the number of incoming connections

## Main examples

### TCP
```
#include <iostream>
#include <vector>
#include <string>
#include "Network/Client.hpp"

int main() {
    net::Client client("TCP");
    std::string str = "Hello";
    std::vector<uint8_t> data(str.begin(), str.end());

    client.connect("127.0.0.1", 5000);
    client.send(data);

    std::cout << "Sent: ";
    for (uint8_t byte : data)
        std::cout << static_cast<char>(byte);
    std::cout << " (" << data.size() << " bytes)" << std::endl;

    client.disconnect();
    return 0;
}
```

### UDP
```
#include <iostream>
#include <vector>
#include <string>
#include "Network/Client.hpp"

int main() {
    net::Client client("UDP");
    std::string str = "Hello";
    std::vector<uint8_t> data(str.begin(), str.end());

    client.connect("127.0.0.1", 5000);
    client.send(data);

    std::cout << "Sent: ";
    for (uint8_t byte : data)
        std::cout << static_cast<char>(byte);
    std::cout << " (" << data.size() << " bytes)" << std::endl;

    client.disconnect();
    return 0;
}
```

# PROTOCOL

The `ProtocolManager` class takes as parameter the path to the `protocol.json` config file

It is needed for the class to set its parameters

The file contains 5 fields:
- endianness
- preamble
- packet_length
- datetime
- end_of_packet

## FILE
```
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
    "active": true,
    "length": 8
  },
  "end_of_packet": {
    "active": true,
    "characters": "\r\n"
  }
}
```

## PARAMETERS

### Endianess

Describes how machine will organise bytes to encode values, in big endian, big values are first (on the left), opposite for little endian

**Exemple :**
```
Int:            12
Big-endian:     [0, 0, 0, 12]
Little-endian:  [12, 0, 0, 0]
```

### Preamble

Packet start marker, usually just 4 empty bytes, at the very beginning of the packet

**Exemple :**
```
[0, 0, 0, 0, ... data]
```

### Packet length

Packet length indicator, 4 bytes, usually following preamble

**Exemple :**
```
[maybe preamble ..., 0, 0, 0, 13, ... 13 bytes data]
```

### Datetime

Packet datetime indicator, 4 bytes, usually following packet length indicator (or preamble)

**Exemple :**
```
[maybe preamble ..., maybe packet length, X, X, X, X, ... 13 bytes data]
```

### End of Packet

Packet end marker, usually a `CRLF (\r\n | 13,10)` or a `LF (\n | 10)`, at the very end of the packet

**Exemple :**
```
[13 bytes data ..., 13, 10]
```
