#pragma once

#ifdef _WIN32

#include <pcapplusplus/PointerVector.h>
#include <pcapplusplus/RawPacket.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace pcap {
	class PktMonRawPacket : public pcpp::RawPacket {
	public:
		PktMonRawPacket(const uint8_t *rawData, int rawDataLen, timespec timestamp, bool takeOwnership, pcpp::LinkLayerType layerType, uint16_t componentId, uint16_t packetType, uint16_t direction, uint64_t pktGroupId)
			: pcpp::RawPacket(rawData, rawDataLen, timestamp, takeOwnership, layerType),
			  m_componentId(componentId),
			  m_packetType(packetType),
			  m_direction(direction),
			  m_pktGroupId(pktGroupId) {}

		[[nodiscard]] uint16_t getComponentId() const {
			return m_componentId;
		}

		[[nodiscard]] uint16_t getPacketType() const {
			return m_packetType;
		}

		[[nodiscard]] uint16_t getDirection() const {
			return m_direction;
		}

		[[nodiscard]] uint64_t getPktGroupId() const {
			return m_pktGroupId;
		}

	private:
		uint16_t m_componentId;
		uint16_t m_packetType;
		uint16_t m_direction;
		uint64_t m_pktGroupId;
	};

	class PktMonDevice {
	public:
		struct ReceiveResult {
			enum class Status {
				completed,
				timeout,
				failed,
			};

			Status status;
			std::string error;
			uint32_t errorCode = 0;
		};

		struct Filter {
			uint16_t etherType = 0;
			uint8_t transportProtocol = 0;
			uint16_t sourcePort = 0;
			uint16_t destinationPort = 0;
		};

		using RawPacketVector = pcpp::PointerVector<PktMonRawPacket>;

		using ReceiveCallback = std::function<void(const RawPacketVector &packets)>;

		PktMonDevice();
		~PktMonDevice();

		PktMonDevice(const PktMonDevice &) = delete;
		PktMonDevice &operator=(const PktMonDevice &) = delete;

		[[nodiscard]] bool open();
		void close();
		[[nodiscard]] bool isOpened() const;

		[[nodiscard]] bool addFilter(const Filter &filter);
		[[nodiscard]] bool addUdpPortFilter(uint16_t port);
		[[nodiscard]] bool addTcpPortFilter(uint16_t port);

		[[nodiscard]] ReceiveResult receivePackets(const ReceiveCallback &callback, uint32_t timeout = 5000, uint8_t batchSize = 64);
		void stopReceive();

		[[nodiscard]] std::string_view getBackendName() const;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}// namespace pcap

#endif
