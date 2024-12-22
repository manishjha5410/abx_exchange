#include <algorithm>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

namespace json = boost::json;

struct Packet {
    std::string symbol;
    char buy_sell;
    int quantity;
    int price;
    int sequence;
};

void convertBigEndianToHostOrder(int32_t &value) { value = ntohl(value); }

Packet parsePacket(const std::vector<char> &data) {
    Packet packet;

    packet.symbol = std::string(data.begin(), data.begin() + 4);
    packet.buy_sell = data[4];

    int32_t quantity;
    std::memcpy(&quantity, &data[5], sizeof(quantity));
    convertBigEndianToHostOrder(quantity);
    packet.quantity = quantity;

    int32_t price;
    std::memcpy(&price, &data[9], sizeof(price));
    convertBigEndianToHostOrder(price);
    packet.price = price;

    int32_t sequence;
    std::memcpy(&sequence, &data[13], sizeof(sequence));
    convertBigEndianToHostOrder(sequence);
    packet.sequence = sequence;

    return packet;
}

void generateJSONFile(const std::vector<Packet> &packets, const std::string &filename) {
    json::array output;

    for (const auto &packet : packets) {
        output.emplace_back(
            json::object{{"symbol", packet.symbol},
                         {"buy_sell", std::string(1, packet.buy_sell)},
                         {"quantity", packet.quantity},
                         {"price", packet.price},
                         {"sequence", packet.sequence}});
    }

    std::ofstream file(filename);
    file << json::serialize(output);
    file.close();
    std::cout << "JSON file generated: " << filename << std::endl;
}

std::vector<int> findMissingSequences(const std::vector<Packet> &packets) {
    std::vector<int> sequences;
    for (const auto &packet : packets) {
        sequences.push_back(packet.sequence);
    }

    std::sort(sequences.begin(), sequences.end());

    std::vector<int> missing;
    int i = 0;

    for (int &x : sequences) {
        i++;
        while (i != x) {
            missing.push_back(i);
            i++;
        }
    }

    return missing;
}

void sendRequest(const std::vector<char> &request, std::vector<char> &response) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "3000");
    boost::asio::connect(socket, endpoints);

    int type = int(request[0]);

    boost::asio::write(socket, boost::asio::buffer(request));

    while (socket.is_open()) {
        char buffer[1024];
        boost::system::error_code error;
        size_t len = socket.read_some(boost::asio::buffer(buffer), error);

        if (error && error != boost::asio::error::eof) {
            throw boost::system::system_error(error);
        }

        if (len == 0) {
            break;
        }

        response.insert(response.end(), buffer, buffer + len);

        if (type == 2) {
            socket.close();
            break;
        }
    }
}

int main() {
    try {
        std::vector<char> request(2, 0);
        request[0] = 1;

        std::vector<char> response;
        sendRequest(request, response);

        std::vector<Packet> packets;
        for (size_t i = 0; i < response.size(); i += 17) {
            packets.push_back(parsePacket({response.begin() + i, response.begin() + i + 17}));
        }

        auto missingSequences = findMissingSequences(packets);
        for (int sequence : missingSequences) {
            request[0] = 2; 
            request[1] = static_cast<char>(sequence);

            response.clear();
            sendRequest(request, response);

            if (!response.empty()) {
                packets.push_back(parsePacket(response));
            }
        }

        std::sort(packets.begin(), packets.end(),[](const Packet &a, const Packet &b) {
			return a.sequence < b.sequence;
		});

        generateJSONFile(packets, "output.json");

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}


// Cmd Run: - g++ -std=c++17 -Wall -I"D:\Program Files\MinGW\include" -L"D:\Program Files\MinGW\lib" app.cpp -o main.exe -lboost_json -lboost_system -lws2_32 && .\main.exe