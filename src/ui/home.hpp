#pragma once

#include "../pcap/pcap.hpp"
#include "core/core.hpp"
#include "data/exportSettings.hpp"

namespace ui {
	using namespace squi;

	struct Home : StatefulWidget {
		// Args
		Key key;
		Args widget{};

		struct State : WidgetState<Home> {
			bool isLoading = true;
			bool isCapturing = false;
			bool manifestUpdateAvailable = false;
			data::ExportSettings exportSettings;
			Pcap pcap;
			VoidObserver onEventUpdate{};

			void initState() override;

			void updateData();
			void initializeData();

			void dispose() override {
				pcap.stop();
			}

			std::optional<std::string> getSelectedRegion();

			Child build(const Element &element) override;
		};
	};
}// namespace ui