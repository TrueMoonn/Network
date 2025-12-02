#include <iostream>
#include <cstring>
#include <chrono>
#include "Network/Client.hpp"
#include "Network/Packet.hpp"
#include "Network/PacketSerializer.hpp"

int main() {
    std::cout << "I'm the client" << std::endl;
    Client client("UDP");
    if (!client.connect("127.0.0.1", 8080)) {
        std::cerr << "Failed to connect" << std::endl;
        return 1;
    }
    std::cout << "Connected to server at 127.0.0.1:8080" << std::endl;
    // créer et envoyer un paquet PlayerJoin
    PlayerJoinPacket joinPacket;
    joinPacket.header.type = PacketType::PLAYER_JOIN;
    joinPacket.header.sequence_number = 1;
    joinPacket.header.timestamp = 12345;
    std::strncpy(joinPacket.player_name, "TestPlayer123", sizeof(joinPacket.player_name) - 1);
    joinPacket.player_name[sizeof(joinPacket.player_name) - 1] = '\0';
    std::cout << "Sending PlayerJoin packet..." << std::endl;
    auto packetData = PacketSerializer::serialize(joinPacket);
    if (client.send(packetData)) {
        std::cout << "PlayerJoin sent successfully" << std::endl;
    } else {
        std::cerr << "Failed to send packet" << std::endl;
        return 1;
    }
    std::cout << "Waiting for server response..." << std::endl;

    char buffer[1024];
    int received = client.receive(buffer, sizeof(buffer));

    if (received > 0) {
        std::cout << "Received " << received << " bytes" << std::endl;

        std::vector<uint8_t> receivedData(buffer, buffer + received);
        PlayerPositionPacket response;
        if (!PacketSerializer::deserialize(receivedData, response)) {
            std::cerr << "Failed to deserialize packet" << std::endl;
            return 1;
        }
        std::cout << "PlayerPosition packet received:" << std::endl;
        std::cout << "  Player ID: " << response.player_id << std::endl;
        std::cout << "  Position: (" << response.x << ", " << response.y << ")" << std::endl;
        std::cout << "  Rotation: " << response.rotation << "°" << std::endl;
        std::cout << "  Velocity: (" << response.velocity_x << ", " << response.velocity_y << ")" << std::endl;
    } else {
        std::cerr << "Failed to receive response: " << received << std::endl;
    }
    client.disconnect();
    std::cout << "Disconnected" << std::endl;
    return 0;
}
