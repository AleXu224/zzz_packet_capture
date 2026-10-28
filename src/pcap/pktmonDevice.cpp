#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "pktmonDevice.hpp"
#include "../util/debug.hpp"

#include <evntcons.h>
#include <evntrace.h>
#include <tdh.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

namespace pcap {
	namespace {
		constexpr uint32_t pktMonApiVersion = 0x00010000;
		constexpr uint64_t pktMonPayloadKeyword = 0x10;
		constexpr UCHAR pktMonTraceLevelInformation = 4;
		constexpr size_t pktMonMaxQueuedPackets = 65536;
		constexpr uint16_t pktMonTruncationSize = 9000;
		constexpr uint16_t pktMonBufferSizeMultiplier = 10;
		constexpr size_t pktMonStatusSize = 0x103C;
		constexpr DWORD pktMonStatusCode = 0;

		constexpr wchar_t pktMonSessionName[] = L"PcapPlusPlus-PktMon";

		const GUID pktMonProviderGuid = {
			0x4d4f80d9,
			0xc8bd,
			0x4d73,
			{0xbb, 0x5b, 0x19, 0xc9, 0x04, 0x02, 0xc5, 0xac}
		};

		const GUID pktMonSessionGuid = {
			0x5b2b901c,
			0x294b,
			0x4eab,
			{0x8c, 0xd4, 0x80, 0x39, 0xa6, 0x08, 0x6f, 0x35}
		};

		DECLARE_HANDLE(PACKETMONITOR_HANDLE);
		DECLARE_HANDLE(PACKETMONITOR_SESSION);
		DECLARE_HANDLE(PACKETMONITOR_REALTIME_STREAM);

		enum class PktMonStreamEventKind {
			started = 0,
			stopped = 1,
			fatalError = 2,
			processInfo = 3
		};

		struct PktMonStreamStartInfo {
			uint32_t packetBufferSizeInBytes;
			uint16_t truncationSize;
		};

		struct PktMonStreamStopInfo {
			BOOLEAN isFatalError;
			uint32_t reason;
		};

		struct PktMonStreamProcessInfo {
			BOOLEAN isWarning;
			uint32_t reason;
			uint64_t packetLength;
		};

		union PktMonStreamEventInfo {
			PktMonStreamStartInfo streamStartInfo;
			PktMonStreamStopInfo streamStopInfo;
			PktMonStreamProcessInfo streamProcessInfo;
		};

		struct PktMonStreamMetadata {
			uint64_t pktGroupId;
			uint16_t pktCount;
			uint16_t appearanceCount;
			uint16_t directionName;
			uint16_t packetType;
			uint16_t componentId;
			uint16_t edgeId;
			uint16_t reserved;
			uint32_t dropReason;
			uint32_t dropLocation;
			uint16_t processor;
			LARGE_INTEGER timeStamp;
		};

		struct PktMonStreamDataDescriptor {
			const void *data;
			uint32_t dataSize;
			uint32_t metadataOffset;
			uint32_t packetOffset;
			uint32_t packetLength;
			uint32_t missedPacketWriteCount;
			uint32_t missedPacketReadCount;
		};

		using PktMonStreamEventCallback = void(CALLBACK *)(void *context, const PktMonStreamEventInfo *eventInfo, PktMonStreamEventKind eventKind);
		using PktMonStreamDataCallback = void(CALLBACK *)(void *context, const PktMonStreamDataDescriptor *data);

		struct PktMonRealtimeStreamConfiguration {
			void *userContext;
			PktMonStreamEventCallback eventCallback;
			PktMonStreamDataCallback dataCallback;
			uint16_t bufferSizeMultiplier;
			uint16_t truncationSize;
		};

		union PktMonIpAddress {
			ULONG ipv4;
			std::array<UCHAR, 4> ipv4Bytes;
			std::array<ULONGLONG, 2> ipv6;
			std::array<UCHAR, 16> ipv6Bytes;
		};

		struct PktMonProtocolConstraint {
			std::array<WCHAR, 64> name;
			union {
				struct
				{
					UINT mac1 : 1;
					UINT mac2 : 1;
					UINT vlanId : 1;
					UINT etherType : 1;
					UINT dscp : 1;
					UINT transportProtocol : 1;
					UINT ip1 : 1;
					UINT ip2 : 1;
					UINT ipv6 : 1;
					UINT prefixLength1 : 1;
					UINT prefixLength2 : 1;
					UINT port1 : 1;
					UINT port2 : 1;
					UINT tcpFlags : 1;
					UINT encapType : 1;
					UINT vxLanPort : 1;
					UINT clusterHeartbeat : 1;
				} isPresent;
				UINT isPresentValue;
			};
			std::array<UCHAR, 6> mac1;
			std::array<UCHAR, 6> mac2;
			USHORT vlanId;
			USHORT etherType;
			USHORT dscp;
			UCHAR transportProtocol;
			PktMonIpAddress ip1;
			PktMonIpAddress ip2;
			UCHAR prefixLength1;
			UCHAR prefixLength2;
			USHORT port1;
			USHORT port2;
			UCHAR tcpFlags;
			ULONG encapType;
			USHORT vxLanPort;
			uint64_t packets;
			uint64_t bytes;
		};

		struct PktMonLegacyFilter {
			uint16_t size;
			std::array<WCHAR, 64> name;
			std::array<UCHAR, 6> macSource;
			std::array<UCHAR, 6> macDestination;
			uint16_t vlan;
			uint16_t protocol;
			UCHAR transportProtocol;
			UCHAR ipv6;
			std::array<UCHAR, 4> padding1;
			PktMonIpAddress ipSource;
			PktMonIpAddress ipDestination;
			UCHAR ipSourcePrefix;
			UCHAR ipDestinationPrefix;
			uint16_t portSource;
			uint16_t portDestination;
			UCHAR tcpFlags;
			UCHAR encapsulation;
			uint16_t vxlanPort;
			UCHAR heartbeat;
			UCHAR padding2;
			uint16_t dscp;
			std::array<UCHAR, 2> padding3;
		};

		struct PktMonStartParams {
			uint32_t captureType;
			uint32_t unknown1;
			uint32_t mode;
			uint16_t componentCount;
			uint16_t unknown2;
			const void *componentList;
			uint16_t unknown3;
		};

		struct PktMonEtwSessionProperties {
			EVENT_TRACE_PROPERTIES properties;
			std::array<WCHAR, 128> loggerName;
		};

		static_assert(sizeof(PktMonLegacyFilter) == 200, "Unexpected PktMon legacy filter layout");
		static_assert(offsetof(PktMonLegacyFilter, portSource) == 0xBA, "Unexpected PktMon legacy filter layout");
		static_assert(sizeof(PktMonProtocolConstraint) == 216, "Unexpected PktMon protocol constraint layout");
		static_assert(offsetof(PktMonProtocolConstraint, port1) == 0xBA, "Unexpected PktMon protocol constraint layout");
		static_assert(sizeof(L"PcapPlusPlus") / sizeof(wchar_t) <= 64);
		static_assert(sizeof(pktMonSessionName) / sizeof(wchar_t) <= 128);

		using PacketMonitorInitializeFn = HRESULT(WINAPI *)(UINT32 apiVersion, void *reserved, PACKETMONITOR_HANDLE *handle);
		using PacketMonitorUninitializeFn = void(WINAPI *)(PACKETMONITOR_HANDLE handle);
		using PacketMonitorCreateLiveSessionFn = HRESULT(WINAPI *)(PACKETMONITOR_HANDLE handle, PCWSTR name, PACKETMONITOR_SESSION *session);
		using PacketMonitorSetSessionActiveFn = HRESULT(WINAPI *)(PACKETMONITOR_SESSION session, BOOL active);
		using PacketMonitorCreateRealtimeStreamFn = HRESULT(WINAPI *)(PACKETMONITOR_HANDLE handle, const PktMonRealtimeStreamConfiguration *configuration, PACKETMONITOR_REALTIME_STREAM *realtimeStream);
		using PacketMonitorAttachOutputToSessionFn = HRESULT(WINAPI *)(PACKETMONITOR_SESSION session, PACKETMONITOR_REALTIME_STREAM outputHandle);
		using PacketMonitorCloseSessionHandleFn = void(WINAPI *)(PACKETMONITOR_SESSION session);
		using PacketMonitorCloseRealtimeStreamFn = void(WINAPI *)(PACKETMONITOR_REALTIME_STREAM realtimeStream);
		using PacketMonitorAddCaptureConstraintFn = HRESULT(WINAPI *)(PACKETMONITOR_SESSION session, const PktMonProtocolConstraint *captureConstraint);

		using PktmonAddFilterFn = uint32_t(WINAPI *)(const PktMonLegacyFilter *filter);
		using PktmonRemoveAllFiltersFn = uint32_t(WINAPI *)();
		using PktmonStartFn = uint32_t(WINAPI *)(const PktMonStartParams *params, void *status);
		using PktmonStopFn = uint32_t(WINAPI *)(void *status);
		using PktmonGetStatusFn = uint32_t(WINAPI *)(void *status);

		timespec currentTimespec() {
			auto now = std::chrono::system_clock::now().time_since_epoch();
			auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
			return {static_cast<time_t>(nanoseconds / 1'000'000'000LL), static_cast<long>(nanoseconds % 1'000'000'000LL)};
		}

		timespec fileTimeToTimespec(uint64_t fileTime) {
			constexpr uint64_t fileTimeUnixEpoch = 116'444'736'000'000'000ULL;
			constexpr uint64_t fileTimeMin = fileTimeUnixEpoch + 1'577'836'800ULL * 10'000'000ULL;
			constexpr uint64_t fileTimeMax = fileTimeUnixEpoch + 4'102'444'800ULL * 10'000'000ULL;
			if (fileTime < fileTimeMin || fileTime > fileTimeMax) {
				return currentTimespec();
			}

			uint64_t unixTime = fileTime - fileTimeUnixEpoch;
			timespec result{
				static_cast<time_t>(unixTime / 10'000'000ULL),
				static_cast<long>((unixTime % 10'000'000ULL) * 100ULL),
			};
			auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			int64_t difference = static_cast<int64_t>(result.tv_sec) - nowSeconds;
			if (difference < -3600 || difference > 3600) {
				return currentTimespec();
			}
			return result;
		}

		pcpp::LinkLayerType getLinkLayerType(uint16_t packetType, std::span<const uint8_t> data) {
			switch (packetType) {
				case 1:
					return pcpp::LINKTYPE_ETHERNET;
				case 2:
					return pcpp::LINKTYPE_IEEE802_11;
				case 3:
					if (!data.empty()) {
						switch (data[0] >> 4) {
							case 4:
								return pcpp::LINKTYPE_IPV4;
							case 6:
								return pcpp::LINKTYPE_IPV6;
							default:
								return pcpp::LINKTYPE_ETHERNET;
						}
					}
					return pcpp::LINKTYPE_INVALID;
				default:
					return pcpp::LINKTYPE_INVALID;
			}
		}

		template<typename T>
		bool resolveFunction(HMODULE module, const char *name, T &function) {
			function = reinterpret_cast<T>(GetProcAddress(module, name));
			return function != nullptr;
		}
	}

	struct PktMonDevice::Impl {
		enum class Backend {
			none,
			realtime,
			legacy
		};

		struct CapturedPacket {
			std::vector<uint8_t> data;
			timespec timestamp = {};
			pcpp::LinkLayerType linkType = pcpp::LINKTYPE_INVALID;
			uint16_t componentId = 0;
			uint16_t packetType = 0;
			uint16_t direction = 0;
			uint64_t pktGroupId = 0;
		};

		Backend backend = Backend::none;
		HMODULE module = nullptr;
		bool opened = false;
		bool captureStarted = false;
		std::atomic<bool> stopRequested{false};
		std::atomic<bool> receiving{false};

		std::mutex queueMutex;
		std::condition_variable queueCv;
		std::deque<CapturedPacket> queue;

		bool hasRealtimeApi = false;
		bool hasLegacyApi = false;

		PACKETMONITOR_HANDLE monitorHandle = nullptr;
		PACKETMONITOR_SESSION session = nullptr;
		PACKETMONITOR_REALTIME_STREAM stream = nullptr;
		PktMonRealtimeStreamConfiguration streamConfig = {};
		PacketMonitorInitializeFn initialize = nullptr;
		PacketMonitorUninitializeFn uninitialize = nullptr;
		PacketMonitorCreateLiveSessionFn createLiveSession = nullptr;
		PacketMonitorSetSessionActiveFn setSessionActive = nullptr;
		PacketMonitorCreateRealtimeStreamFn createRealtimeStream = nullptr;
		PacketMonitorAttachOutputToSessionFn attachOutputToSession = nullptr;
		PacketMonitorCloseSessionHandleFn closeSessionHandle = nullptr;
		PacketMonitorCloseRealtimeStreamFn closeRealtimeStream = nullptr;
		PacketMonitorAddCaptureConstraintFn addCaptureConstraint = nullptr;

		PktmonAddFilterFn addLegacyFilter = nullptr;
		PktmonRemoveAllFiltersFn removeAllLegacyFilters = nullptr;
		PktmonStartFn startLegacyCapture = nullptr;
		PktmonStopFn stopLegacyCapture = nullptr;
		PktmonGetStatusFn getLegacyStatus = nullptr;

		TRACEHANDLE controlHandle = 0;
		TRACEHANDLE traceHandle = 0;
		PktMonEtwSessionProperties sessionProperties = {};
		std::thread traceThread;
		bool providerEnabled = false;
		bool driverStarted = false;

		~Impl() {
			close();
		}

		[[nodiscard]] std::string_view backendName() const {
			switch (backend) {
				case Backend::realtime:
					return "realtime";
				case Backend::legacy:
					return "legacy";
				default:
					return "none";
			}
		}

		bool loadApi() {
			module = LoadLibraryW(L"Pktmonapi.dll");
			if (module == nullptr) {
				debug::log("PktMon: failed to load Pktmonapi.dll (error {})", GetLastError());
				return false;
			}

			hasRealtimeApi = resolveFunction(module, "PacketMonitorInitialize", initialize) && resolveFunction(module, "PacketMonitorUninitialize", uninitialize) && resolveFunction(module, "PacketMonitorCreateLiveSession", createLiveSession) && resolveFunction(module, "PacketMonitorSetSessionActive", setSessionActive) && resolveFunction(module, "PacketMonitorCreateRealtimeStream", createRealtimeStream) && resolveFunction(module, "PacketMonitorAttachOutputToSession", attachOutputToSession) && resolveFunction(module, "PacketMonitorCloseSessionHandle", closeSessionHandle) && resolveFunction(module, "PacketMonitorCloseRealtimeStream", closeRealtimeStream) && resolveFunction(module, "PacketMonitorAddCaptureConstraint", addCaptureConstraint);

			hasLegacyApi = resolveFunction(module, "PktmonAddFilter", addLegacyFilter) && resolveFunction(module, "PktmonRemoveAllFilters", removeAllLegacyFilters) && resolveFunction(module, "PktmonStart", startLegacyCapture) && resolveFunction(module, "PktmonStop", stopLegacyCapture) && resolveFunction(module, "PktmonGetStatus", getLegacyStatus);

			if (!hasRealtimeApi && !hasLegacyApi) {
				debug::log("PktMon: Pktmonapi.dll does not expose the expected capture APIs");
				FreeLibrary(module);
				module = nullptr;
				return false;
			}

			return true;
		}

		bool open() {
			if (opened) {
				return true;
			}

			if (!loadApi()) {
				return false;
			}

			if (hasRealtimeApi && openRealtime()) {
				backend = Backend::realtime;
				opened = true;
				return true;
			}

			cleanupRealtime();

			if (hasLegacyApi && openLegacy()) {
				backend = Backend::legacy;
				opened = true;
				return true;
			}

			cleanupLegacy();
			if (module != nullptr) {
				FreeLibrary(module);
				module = nullptr;
			}
			return false;
		}

		void close() {
			if (module == nullptr) {
				return;
			}

			stopRequested = true;
			queueCv.notify_all();
			stopCapture();

			if (backend == Backend::realtime) {
				cleanupRealtime();
			} else if (backend == Backend::legacy) {
				cleanupLegacy();
			}

			FreeLibrary(module);
			module = nullptr;
			opened = false;
			backend = Backend::none;
		}

		bool openRealtime() {
			HRESULT result = initialize(pktMonApiVersion, nullptr, &monitorHandle);
			if (FAILED(result) || monitorHandle == nullptr) {
				debug::log("PktMon: PacketMonitorInitialize failed ({:#x})", static_cast<uint32_t>(result));
				return false;
			}

			std::wstring sessionName = L"PcapPlusPlus-" + std::to_wstring(GetCurrentProcessId());
			result = createLiveSession(monitorHandle, sessionName.c_str(), &session);
			if (FAILED(result) || session == nullptr) {
				debug::log("PktMon: PacketMonitorCreateLiveSession failed ({:#x})", static_cast<uint32_t>(result));
				return false;
			}

			streamConfig.userContext = this;
			streamConfig.eventCallback = &Impl::streamEventCallback;
			streamConfig.dataCallback = &Impl::streamDataCallback;
			streamConfig.bufferSizeMultiplier = pktMonBufferSizeMultiplier;
			streamConfig.truncationSize = pktMonTruncationSize;

			result = createRealtimeStream(monitorHandle, &streamConfig, &stream);
			if (FAILED(result) || stream == nullptr) {
				debug::log("PktMon: PacketMonitorCreateRealtimeStream failed ({:#x})", static_cast<uint32_t>(result));
				return false;
			}

			result = attachOutputToSession(session, stream);
			if (FAILED(result)) {
				debug::log("PktMon: PacketMonitorAttachOutputToSession failed ({:#x})", static_cast<uint32_t>(result));
				return false;
			}

			return true;
		}

		void cleanupRealtime() {
			if (stream != nullptr && closeRealtimeStream != nullptr) {
				closeRealtimeStream(stream);
				stream = nullptr;
			}
			if (session != nullptr && closeSessionHandle != nullptr) {
				closeSessionHandle(session);
				session = nullptr;
			}
			if (monitorHandle != nullptr && uninitialize != nullptr) {
				uninitialize(monitorHandle);
				monitorHandle = nullptr;
			}
		}

		void cleanupLegacy() {
			if (driverStarted || providerEnabled || traceHandle != 0 || controlHandle != 0) {
				stopLegacy();
			}
		}

		bool openLegacy() {
			std::vector<uint8_t> status(pktMonStatusSize, 0);
			auto error = getLegacyStatus(status.data());
			if (error != pktMonStatusCode) {
				debug::log("PktMon: PktmonGetStatus failed with error {}", error);
				return false;
			}

			if (status[0] != 0) {
				debug::log("PktMon: already capturing, stopping the existing capture");
				error = stopLegacyCapture(status.data());
				if (error != pktMonStatusCode) {
					debug::log("PktMon: PktmonStop failed with error {}", error);
					return false;
				}
			}

			error = removeAllLegacyFilters();
			if (error != pktMonStatusCode) {
				debug::log("PktMon: PktmonRemoveAllFilters failed with error {}", error);
				return false;
			}

			return true;
		}

		bool addFilter(const Filter &filter) {
			if (!opened || captureStarted) {
				return false;
			}

			if (backend == Backend::realtime) {
				PktMonProtocolConstraint constraint = {};
				if (!fillConstraint(filter, constraint)) {
					return false;
				}

				HRESULT result = addCaptureConstraint(session, &constraint);
				if (FAILED(result)) {
					debug::log("PktMon: PacketMonitorAddCaptureConstraint failed ({:#x})", static_cast<uint32_t>(result));
					return false;
				}
				return true;
			}

			if (backend == Backend::legacy) {
				PktMonLegacyFilter legacyFilter = {};
				if (!fillLegacyFilter(filter, legacyFilter)) {
					return false;
				}

				auto error = addLegacyFilter(&legacyFilter);
				if (error != pktMonStatusCode) {
					debug::log("PktMon: PktmonAddFilter failed with error {}", error);
					return false;
				}
				return true;
			}

			return false;
		}

		bool fillConstraint(const Filter &filter, PktMonProtocolConstraint &constraint) {
			if (filter.etherType == 0 && filter.transportProtocol == 0 && filter.sourcePort == 0 && filter.destinationPort == 0) {
				return false;
			}

			std::ranges::copy(L"PcapPlusPlus", constraint.name.begin());
			if (filter.etherType != 0) {
				constraint.isPresent.etherType = 1;
				constraint.etherType = filter.etherType;
			}
			if (filter.transportProtocol != 0) {
				constraint.isPresent.transportProtocol = 1;
				constraint.transportProtocol = filter.transportProtocol;
			}
			if (filter.sourcePort != 0) {
				constraint.isPresent.port1 = 1;
				constraint.port1 = filter.sourcePort;
			}
			if (filter.destinationPort != 0) {
				constraint.isPresent.port2 = 1;
				constraint.port2 = filter.destinationPort;
			}
			return true;
		}

		bool fillLegacyFilter(const Filter &filter, PktMonLegacyFilter &legacyFilter) {
			if (filter.etherType == 0 && filter.transportProtocol == 0 && filter.sourcePort == 0 && filter.destinationPort == 0) {
				return false;
			}

			legacyFilter.size = sizeof(PktMonLegacyFilter);
			std::ranges::copy(L"PcapPlusPlus", legacyFilter.name.begin());
			legacyFilter.protocol = filter.etherType;
			legacyFilter.transportProtocol = filter.transportProtocol;
			legacyFilter.portSource = filter.sourcePort;
			legacyFilter.portDestination = filter.destinationPort;
			return true;
		}

		bool startCapture() {
			if (captureStarted) {
				return true;
			}

			if (backend == Backend::realtime) {
				HRESULT result = setSessionActive(session, TRUE);
				if (FAILED(result)) {
					debug::log("PktMon: PacketMonitorSetSessionActive failed ({:#x})", static_cast<uint32_t>(result));
					return false;
				}
				captureStarted = true;
				return true;
			}

			if (backend == Backend::legacy && startLegacy()) {
				captureStarted = true;
				return true;
			}

			return false;
		}

		void stopCapture() {
			if (!captureStarted) {
				return;
			}

			if (backend == Backend::realtime) {
				if (setSessionActive != nullptr && session != nullptr) {
					setSessionActive(session, FALSE);
				}
			} else if (backend == Backend::legacy) {
				stopLegacy();
			}

			captureStarted = false;
		}

		bool startLegacy() {
			if (!startEtwSession()) {
				return false;
			}

			if (!startEtwConsumer()) {
				stopEtwSession();
				return false;
			}

			if (!startDriverCapture()) {
				stopLegacy();
				return false;
			}

			return true;
		}

		void stopLegacy() {
			if (driverStarted && stopLegacyCapture != nullptr) {
				std::vector<uint8_t> status(pktMonStatusSize, 0);
				auto error = stopLegacyCapture(status.data());
				if (error != pktMonStatusCode) {
					debug::log("PktMon: PktmonStop failed with error {}", error);
				}
				driverStarted = false;
			}

			if (providerEnabled) {
				EnableTraceEx2(controlHandle, &pktMonProviderGuid, EVENT_CONTROL_CODE_DISABLE_PROVIDER, pktMonTraceLevelInformation, pktMonPayloadKeyword, 0, 0, nullptr);
				providerEnabled = false;
			}

			if (traceHandle != 0) {
				TRACEHANDLE handle = traceHandle;
				CloseTrace(handle);
				if (traceThread.joinable()) {
					traceThread.join();
				}
				traceHandle = 0;
			}

			stopEtwSession();

			if (removeAllLegacyFilters != nullptr) {
				removeAllLegacyFilters();
			}
		}

		bool startEtwSession() {
			PktMonEtwSessionProperties orphanProperties = {};
			orphanProperties.properties.Wnode.BufferSize = sizeof(orphanProperties);
			orphanProperties.properties.LoggerNameOffset = static_cast<ULONG>(offsetof(PktMonEtwSessionProperties, loggerName));
			std::ranges::copy(pktMonSessionName, orphanProperties.loggerName.begin());
			ControlTraceW(0, pktMonSessionName, &orphanProperties.properties, EVENT_TRACE_CONTROL_STOP);

			sessionProperties = {};
			auto &properties = sessionProperties.properties;
			properties.Wnode.BufferSize = sizeof(sessionProperties);
			properties.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
			properties.Wnode.ClientContext = 1;
			properties.Wnode.Guid = pktMonSessionGuid;
			properties.LogFileMode = EVENT_TRACE_INDEPENDENT_SESSION_MODE | EVENT_TRACE_REAL_TIME_MODE;
			properties.BufferSize = 16384;
			properties.FlushTimer = 1;
			properties.LoggerNameOffset = static_cast<ULONG>(offsetof(PktMonEtwSessionProperties, loggerName));
			std::ranges::copy(pktMonSessionName, sessionProperties.loggerName.begin());

			auto error = StartTraceW(&controlHandle, pktMonSessionName, &properties);
			if (error != ERROR_SUCCESS) {
				debug::log("PktMon: StartTrace failed with error {}", error);
				controlHandle = 0;
				return false;
			}

			auto result = EnableTraceEx2(controlHandle, &pktMonProviderGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, pktMonTraceLevelInformation, pktMonPayloadKeyword, 0, 0, nullptr);
			if (result != ERROR_SUCCESS) {
				debug::log("PktMon: EnableTraceEx2 failed with error {}", result);
				stopEtwSession();
				return false;
			}

			providerEnabled = true;
			return true;
		}

		void stopEtwSession() {
			if (controlHandle != 0) {
				ControlTraceW(controlHandle, pktMonSessionName, &sessionProperties.properties, EVENT_TRACE_CONTROL_STOP);
				controlHandle = 0;
			}
			providerEnabled = false;
		}

		bool startEtwConsumer() {
			EVENT_TRACE_LOGFILEW logFile = {};
			logFile.LoggerName = const_cast<LPWSTR>(pktMonSessionName);
			logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
			logFile.EventRecordCallback = &Impl::eventRecordCallback;
			logFile.Context = this;

			traceHandle = OpenTraceW(&logFile);
			if (traceHandle == INVALID_PROCESSTRACE_HANDLE) {
				debug::log("PktMon: OpenTrace failed with error {}", GetLastError());
				traceHandle = 0;
				return false;
			}

			traceThread = std::thread([this]() {
				auto error = ProcessTrace(&traceHandle, 1, nullptr, nullptr);
				if (error != ERROR_SUCCESS) {
					debug::log("PktMon: ProcessTrace returned error {}", error);
				}
			});
			return true;
		}

		bool startDriverCapture() {
			PktMonStartParams params = {};
			params.captureType = 1;
			params.mode = 1;

			auto error = startLegacyCapture(&params, nullptr);
			if (error != pktMonStatusCode) {
				debug::log("PktMon: PktmonStart failed with error {}", error);
				return false;
			}

			driverStarted = true;
			return true;
		}

		void pushPacket(std::span<const uint8_t> data, uint64_t fileTime, uint16_t packetType, uint16_t componentId, uint16_t direction, uint64_t pktGroupId) {
			if (data.empty() || stopRequested) {
				return;
			}

			pcpp::LinkLayerType linkType = getLinkLayerType(packetType, data);
			if (linkType == pcpp::LINKTYPE_INVALID) {
				return;
			}

			CapturedPacket packet;
			packet.data.assign(data.begin(), data.end());
			packet.timestamp = fileTimeToTimespec(fileTime);
			packet.linkType = linkType;
			packet.componentId = componentId;
			packet.packetType = packetType;
			packet.direction = direction;
			packet.pktGroupId = pktGroupId;

			{
				std::scoped_lock lock{queueMutex};
				if (queue.size() >= pktMonMaxQueuedPackets) {
					queue.pop_front();
				}
				queue.push_back(std::move(packet));
			}
			queueCv.notify_one();
		}

		ReceiveResult receivePackets(const ReceiveCallback &callback, uint32_t timeout, uint8_t batchSize) {
			if (!opened) {
				return {ReceiveResult::Status::failed, "Device is not open"};
			}
			if (receiving.exchange(true)) {
				return {ReceiveResult::Status::failed, "Already receiving packets, please call stopReceive() first"};
			}
			if (batchSize == 0) {
				receiving = false;
				return {ReceiveResult::Status::failed, "Batch size has to be a positive number"};
			}
			if (!callback) {
				receiving = false;
				return {ReceiveResult::Status::failed, "Callback was not provided"};
			}

			if (!startCapture()) {
				receiving = false;
				return {ReceiveResult::Status::failed, "Failed to start PktMon capture"};
			}

			std::vector<CapturedPacket> batch;
			batch.reserve(batchSize);

			while (!stopRequested) {
				{
					std::unique_lock<std::mutex> lock(queueMutex);
					auto hasData = [this]() {
						return stopRequested.load() || !queue.empty();
					};
					if (timeout == 0) {
						queueCv.wait(lock, hasData);
					} else if (!queueCv.wait_for(lock, std::chrono::milliseconds(timeout), hasData)) {
						lock.unlock();
						receiving = false;
						return {ReceiveResult::Status::timeout};
					}

					if (stopRequested && queue.empty()) {
						break;
					}

					batch.clear();
					size_t packetCount = std::min<size_t>(batchSize, queue.size());
					for (size_t index = 0; index < packetCount; ++index) {
						batch.push_back(std::move(queue.front()));
						queue.pop_front();
					}
				}

				RawPacketVector packetVec;
				for (const auto &packet: batch) {
					auto packetData = std::make_unique<uint8_t[]>(packet.data.size());
					std::ranges::copy(packet.data, packetData.get());
					packetVec.pushBack(new PktMonRawPacket(packetData.release(), static_cast<int>(packet.data.size()), packet.timestamp, true, packet.linkType, packet.componentId, packet.packetType, packet.direction, packet.pktGroupId));
				}

				callback(packetVec);
			}

			receiving = false;
			return {ReceiveResult::Status::completed};
		}

		static void CALLBACK streamDataCallback(void *context, const PktMonStreamDataDescriptor *descriptor) {
			auto *self = static_cast<Impl *>(context);
			if (self == nullptr || descriptor == nullptr || descriptor->data == nullptr) {
				return;
			}

			const auto *base = static_cast<const uint8_t *>(descriptor->data);
			if (descriptor->metadataOffset > descriptor->packetOffset || descriptor->packetOffset > descriptor->dataSize || descriptor->packetLength > descriptor->dataSize - descriptor->packetOffset) {
				return;
			}

			std::array<std::byte, sizeof(PktMonStreamMetadata)> metadataBytes{};
			size_t metadataSize = descriptor->packetOffset - descriptor->metadataOffset;
			size_t metadataCopySize = std::min(metadataSize, metadataBytes.size());
			std::ranges::copy(std::span<const std::byte>{reinterpret_cast<const std::byte *>(base + descriptor->metadataOffset), metadataCopySize}, metadataBytes.begin());
			auto metadata = std::bit_cast<PktMonStreamMetadata>(metadataBytes);
			uint64_t fileTime = metadataSize >= metadataBytes.size() ? static_cast<uint64_t>(metadata.timeStamp.QuadPart) : 0;
			self->pushPacket(std::span(base + descriptor->packetOffset, descriptor->packetLength), fileTime, metadata.packetType, metadata.componentId, metadata.directionName, metadata.pktGroupId);
		}

		static void CALLBACK streamEventCallback(void *, const PktMonStreamEventInfo *eventInfo, PktMonStreamEventKind eventKind) {
			switch (eventKind) {
				case PktMonStreamEventKind::stopped:
					if (eventInfo != nullptr && eventInfo->streamStopInfo.isFatalError) {
						debug::log("PktMon: stream stopped with fatal error (reason {})", eventInfo->streamStopInfo.reason);
					}
					break;
				case PktMonStreamEventKind::fatalError:
					debug::log("PktMon: stream reported a fatal error");
					break;
				case PktMonStreamEventKind::processInfo:
					if (eventInfo != nullptr && !eventInfo->streamProcessInfo.isWarning) {
						debug::log("PktMon: stream error (reason {}, packet {})", eventInfo->streamProcessInfo.reason, eventInfo->streamProcessInfo.packetLength);
					}
					break;
				default:
					break;
			}
		}

		static void CALLBACK eventRecordCallback(PEVENT_RECORD eventRecord) {
			auto *self = static_cast<Impl *>(eventRecord != nullptr ? eventRecord->UserContext : nullptr);
			if (self == nullptr || eventRecord == nullptr) {
				return;
			}

			if (eventRecord->EventHeader.EventDescriptor.Id != 160) {
				return;
			}

			uint16_t packetType = 0;
			uint16_t loggedPayloadSize = 0;
			uint16_t componentId = 0;
			uint16_t direction = 0;
			uint64_t pktGroupId = 0;

			if (!self->getProperty(eventRecord, L"PacketType", packetType) || !self->getProperty(eventRecord, L"LoggedPayloadSize", loggedPayloadSize) || loggedPayloadSize == 0) {
				return;
			}

			self->getProperty(eventRecord, L"ComponentId", componentId);
			self->getProperty(eventRecord, L"DirTag", direction);
			self->getProperty(eventRecord, L"PktGroupId", pktGroupId);

			std::vector<uint8_t> payload;
			if (!self->getPropertyBuffer(eventRecord, L"Payload", payload) || payload.empty()) {
				return;
			}

			auto fileTime = static_cast<uint64_t>(eventRecord->EventHeader.TimeStamp.QuadPart);
			self->pushPacket(std::span<const uint8_t>{payload}, fileTime, packetType, componentId, direction, pktGroupId);
		}

		template<typename T>
		bool getProperty(PEVENT_RECORD eventRecord, const wchar_t *name, T &value) {
			PROPERTY_DATA_DESCRIPTOR descriptor = {};
			descriptor.PropertyName = reinterpret_cast<ULONGLONG>(name);
			auto result = TdhGetProperty(eventRecord, 0, nullptr, 1, &descriptor, sizeof(T), reinterpret_cast<PBYTE>(&value));
			return result == ERROR_SUCCESS;
		}

		bool getPropertyBuffer(PEVENT_RECORD eventRecord, const wchar_t *name, std::vector<uint8_t> &value) {
			PROPERTY_DATA_DESCRIPTOR descriptor = {};
			descriptor.PropertyName = reinterpret_cast<ULONGLONG>(name);

			ULONG size = 0;
			auto result = TdhGetPropertySize(eventRecord, 0, nullptr, 1, &descriptor, &size);
			if (result != ERROR_SUCCESS || size == 0) {
				return false;
			}

			value.resize(size);
			result = TdhGetProperty(eventRecord, 0, nullptr, 1, &descriptor, size, value.data());
			return result == ERROR_SUCCESS;
		}
	};

	PktMonDevice::PktMonDevice() : m_impl(std::make_unique<Impl>()) {}

	PktMonDevice::~PktMonDevice() {
		close();
	}

	bool PktMonDevice::open() {
		return m_impl->open();
	}

	void PktMonDevice::close() {
		m_impl->close();
	}

	bool PktMonDevice::isOpened() const {
		return m_impl->opened;
	}

	std::string_view PktMonDevice::getBackendName() const {
		return m_impl->backendName();
	}

	bool PktMonDevice::addFilter(const Filter &filter) {
		return m_impl->addFilter(filter);
	}

	bool PktMonDevice::addUdpPortFilter(uint16_t port) {
		Filter sourceFilter;
		sourceFilter.transportProtocol = 17;
		sourceFilter.sourcePort = port;

		Filter destinationFilter;
		destinationFilter.transportProtocol = 17;
		destinationFilter.destinationPort = port;

		return addFilter(sourceFilter) && addFilter(destinationFilter);
	}

	bool PktMonDevice::addTcpPortFilter(uint16_t port) {
		Filter sourceFilter;
		sourceFilter.transportProtocol = 6;
		sourceFilter.sourcePort = port;

		Filter destinationFilter;
		destinationFilter.transportProtocol = 6;
		destinationFilter.destinationPort = port;

		return addFilter(sourceFilter) && addFilter(destinationFilter);
	}

	PktMonDevice::ReceiveResult PktMonDevice::receivePackets(const ReceiveCallback &callback, uint32_t timeout, uint8_t batchSize) {
		return m_impl->receivePackets(callback, timeout, batchSize);
	}

	void PktMonDevice::stopReceive() {
		m_impl->stopRequested = true;
		m_impl->queueCv.notify_all();
		m_impl->stopCapture();
	}
}

#endif
