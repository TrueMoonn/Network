#include <iostream>
#include <cstring>
#include "Network/Server.hpp"
#include "Network/Packet.hpp"
#include "Network/PacketSerializer.hpp"

int main() {
    std::cout << "I'm the server" << std::endl;
    Server server("UDP", 8080);
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    std::cout << "Started on port 8080" << std::endl;
    std::cout << "Waiting for clients..." << std::endl;

    while (true) {
        char buffer[1024];
        Address clientAddr;
        int received = server.receive(buffer, sizeof(buffer), clientAddr);
        if (received > 0) {
            std::cout << "\nReceived " << received << " bytes from "
                      << clientAddr.getIP() << ":" << clientAddr.getPort() << std::endl;

            std::vector<uint8_t> receivedData(buffer, buffer + received);
            PlayerJoinPacket packet;
            if (!PacketSerializer::deserialize(receivedData, packet)) {
                std::cerr << "Failed to deserialize packet" << std::endl;
                continue;
            }
            std::cout << "Player name: " << packet.player_name << std::endl;
            std::cout << "Sequence: " << packet.header.sequence_number << std::endl;
            // On envoie la position du joueur après qu'il ait join (c'est juste pour tester)
            PlayerPositionPacket response;
            response.header.type = PacketType::PLAYER_POSITION;
            response.header.sequence_number = packet.header.sequence_number + 1;
            response.header.timestamp = 99999;
            response.player_id = 1;
            response.x = 100.0f;
            response.y = 200.0f;
            response.rotation = 45.0f;
            response.velocity_x = 5.0f;
            response.velocity_y = 3.0f;
            auto responseData = PacketSerializer::serialize(response);
            if (server.send(clientAddr, responseData)) {
                std::cout << "Sent PlayerPosition to client" << std::endl;
            }
        } else if (received < 0) {
            std::cerr << "Error receiving data" << std::endl;
        }
    }
    server.stop();
    return 0;
}
