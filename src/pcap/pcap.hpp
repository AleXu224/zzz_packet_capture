#pragma once

#include "../crypto/session.hpp"
#include "../crypto/xorProtoFields.hpp"
#include "../data/agent.hpp"
#include "../data/disc.hpp"
#include "../data/engine.hpp"
#include "../kcp/kcp.hpp"
#include "../kcp/messageHeader.hpp"
#include "../serialization/datamine.hpp"
#include "../serialization/packets.hpp"
#include "../serialization/proto.hpp"
#include "glaze/glaze.hpp"// IWYU pragma: keep
#include "google/protobuf/unknown_field_set.h"
#include "observer.hpp"
#include "print"
#include <fstream>
#include <memory>
#include <optional>
#include <pcapplusplus/Packet.h>
#include <pcapplusplus/UdpLayer.h>
#ifdef _WIN32
#include <pcapplusplus/WinDivertDevice.h>
#else
#include <pcapplusplus/PcapFilter.h>
#include <pcapplusplus/PcapLiveDevice.h>
#include <pcapplusplus/PcapLiveDeviceList.h>
#endif
#include <ranges>
#include <thread>


// #include "util/proto.hpp"

struct Pcap {
	serialization::PacketList capturedPackets;
	std::vector<std::shared_ptr<google::protobuf::UnknownFieldSet>> parsedPackets;
	std::vector<data::DiscInfo> discs;
	std::vector<data::WeaponInfo> engines;
	std::vector<data::AgentInfo> agents;
	crypto::Session session{};
	KCP::KCP kcp{};
	squi::VoidObservable onEventUpdate{};

#ifdef _WIN32
	std::unique_ptr<pcpp::WinDivertDevice> device;
#else
	pcpp::PcapLiveDevice *device = nullptr;
#endif
	std::jthread captureThread;

	inline void storeCapturePacketsToFile() {
		std::ofstream file("captured_packets.json");
		if (!file.is_open()) {
			std::println("Failed to open file for writing.");
			return;
		}
		std::string buffer;
		auto ec = glz::write_json(capturedPackets, buffer);
		if (ec) {
			std::println("Failed to serialize packets: {}", static_cast<int>(ec));
			return;
		}
		file << buffer;
		std::println("Captured packets stored to captured_packets.json");
		file.close();
	}

	inline void loadCapturePacketsFromFile() {
		std::ifstream file("captured_packets.json");
		if (!file.is_open()) {
			std::println("Failed to open file for reading.");
			return;
		}
		std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		auto ec = glz::read_json(capturedPackets, buffer);
		if (ec) {
			std::println("Failed to deserialize packets: {}", static_cast<int>(ec));
			return;
		}
		std::println("Captured packets loaded from captured_packets.json");
		file.close();

		for (const auto &packet: capturedPackets.packets) {
			processPacket(packet.data, packet.timestamp, packet.direction);
		}
	}

	inline void processMessageBody(std::span<const uint8_t> messageBytes, int64_t unixSeconds) {
		if (messageBytes.size() < KCP::MessageHeader::size) return;
		KCP::MessageHeader messageHeader = KCP::MessageHeader::fromBytes(messageBytes);

		auto body = messageBytes
				  | std::views::drop(KCP::MessageHeader::size + messageHeader.headLength)
				  | std::views::take(messageHeader.bodyLength);

		auto decrypted = session.decryptBody(body);

		auto &datamine = serialization::Datamine::get();

		if (!session.serverRandKey && messageHeader.commandId == datamine.cmdPlayerGetTokenScRsp) {
			try {
				session.serverRandKey = session.extractServerRandKey(std::span<const uint8_t>{decrypted});
				std::println("dispatch: extracted server_rand_key={:016X}", *session.serverRandKey);
			} catch (const std::exception &e) {
				std::println("dispatch: failed to extract server_rand_key: {}", e.what());
			}
			return;
		}

		if (session.serverRandKey && !session.sessionPadReady && messageHeader.bodyLength >= 32) {
			session.deriveSessionKey(body, unixSeconds);
			onEventUpdate.notify();
		}

		if (decrypted.empty()) return;
		auto fields = std::make_shared<google::protobuf::UnknownFieldSet>();
		if (!fields->ParseFromArray(decrypted.data(), static_cast<int>(decrypted.size()))) {
			std::println("Failed to parse decrypted body as protobuf for cmd {} ({} bytes)", messageHeader.commandId, decrypted.size());
			return;
		}
		crypto::xorProtoFields(*fields, serialization::Proto::get().getEntryById(messageHeader.commandId));
		std::println("cmd {} ({} bytes):", messageHeader.commandId, decrypted.size());
		parsedPackets.emplace_back(fields);

		if (messageHeader.commandId == datamine.cmdGetEquipDataScRsp) {
			// util::printUFS(*fields);
			for (int i = 0; i < fields->field_count(); ++i) {
				const auto &f = fields->field(i);
				if (f.number() != datamine.equipData.discs) continue;
				google::protobuf::UnknownFieldSet nested;
				if (nested.ParseFromString(f.length_delimited())) {
					discs.push_back(data::DiscInfo::fromUFS(nested));
				}
			}
			onEventUpdate.notify();
			std::println("  decoded {} discs", discs.size());
		}

		if (messageHeader.commandId == datamine.cmdGetWeaponDataScRsp) {
			// util::printUFS(*fields);
			for (int i = 0; i < fields->field_count(); ++i) {
				const auto &f = fields->field(i);
				if (f.number() != datamine.weaponData.weapons) continue;
				google::protobuf::UnknownFieldSet nested;
				if (nested.ParseFromString(f.length_delimited()))
					engines.push_back(data::WeaponInfo::fromUFS(nested));
			}
			onEventUpdate.notify();
			std::println("  decoded {} weapons", engines.size());
		}

		if (messageHeader.commandId == datamine.cmdGetAvatarDataScRsp) {
			// util::printUFS(*fields);
			for (int i = 0; i < fields->field_count(); ++i) {
				const auto &f = fields->field(i);
				if (f.number() != datamine.agentData.agents) continue;
				google::protobuf::UnknownFieldSet nested;
				if (nested.ParseFromString(f.length_delimited()))
					agents.push_back(data::AgentInfo::fromUFS(nested));
			}
			onEventUpdate.notify();
			std::println("  decoded {} avatars", agents.size());
		}
	}

	inline void processPacket(std::span<const uint8_t> data, int64_t unixSeconds, serialization::Direction direction) {
		auto messages = kcp.receive(data, direction);
		for (auto &message: messages) {
			processMessageBody(message, unixSeconds);
		}
	}

	inline void processRawPacket(pcpp::RawPacket *rawPacket) {
		pcpp::Packet parsedPacket(rawPacket);
		auto *udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
		if (!udpLayer) return;
		if (udpLayer->getSrcPort() != 20501 && udpLayer->getDstPort() != 20501) return;

		auto data = std::span(udpLayer->getLayerPayload(), udpLayer->getLayerPayloadSize());
		if (data.empty()) return;

		bool outgoing = udpLayer->getDstPort() == 20501;
		auto direction = outgoing ? serialization::Direction::outgoing : serialization::Direction::incoming;
		processPacket(data, rawPacket->getPacketTimeStamp().tv_sec, direction);

		capturedPackets.packets.emplace_back(serialization::Packet{
			.direction = direction,
			.timestamp = rawPacket->getPacketTimeStamp().tv_sec,
			.data = std::vector<uint8_t>(data.begin(), data.end()),
		});
	}

	inline void listen() {
		if (captureThread.joinable()) return;

#ifdef _WIN32
		device = std::make_unique<pcpp::WinDivertDevice>();
		if (!device->open("true")) {
			std::println("Failed to open WinDivert device");
			device.reset();
			return;
		}

		std::println("Capture started");

		captureThread = std::jthread([this]() {
			auto result = device->receivePackets(
				[this](const pcpp::WinDivertDevice::WinDivertRawPacketVector &packetVec, const pcpp::WinDivertDevice::WinDivertReceiveCallbackContext &) {
					for (auto *rawPacket: packetVec) {
						processRawPacket(rawPacket);
					}
				},
				0,
				64
			);
			if (result.status != pcpp::WinDivertDevice::ReceiveResult::Status::Completed) {
				std::println("WinDivert capture stopped: {} (code {})", result.error, result.errorCode);
			}
		});
#else
		device = pcpp::PcapLiveDeviceList::getInstance().getDeviceByName("any");
		if (!device) {
			std::println("Failed to find a capture device (are you running as root or with CAP_NET_RAW?)");
			return;
		}
		pcpp::PcapLiveDevice::DeviceConfiguration config{
			pcpp::PcapLiveDevice::DeviceMode::Normal,
		};
		if (!device->open(config)) {
			std::println("Failed to open capture device (are you running as root or with CAP_NET_RAW?)");
			device = nullptr;
			return;
		}
		pcpp::PortFilter portFilter(20501, pcpp::SRC_OR_DST);
		pcpp::ProtoFilter protoFilter(pcpp::UDP);
		pcpp::AndFilter filter;
		filter.addFilter(&portFilter);
		filter.addFilter(&protoFilter);
		if (!device->setFilter(filter)) {
			std::println("Failed to set capture filter, capturing all traffic");
		}

		std::println("Capture started");

		captureThread = std::jthread([this]() {
			device->startCapture(
				[this](pcpp::RawPacket *rawPacket, pcpp::PcapLiveDevice *, void *) {
					processRawPacket(rawPacket);
				},
				nullptr
			);
		});
#endif
	}

	inline void stop() {
#ifdef _WIN32
		if (!device || !device->isOpened()) return;
		device->stopReceive();
#else
		if (!device || !device->isOpened()) return;
		device->stopCapture();
#endif
		if (captureThread.joinable()) {
			captureThread.join();
		}
#ifdef _WIN32
		device->close();
		device.reset();
#else
		device->close();
		device = nullptr;
#endif
		std::println("Capture stopped");
	}
};
