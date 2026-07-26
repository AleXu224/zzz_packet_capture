#pragma once

#include "../pcap/pcap.hpp"
#include "../serialization/proto.hpp"
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
			data::ExportSettings exportSettings;
			serialization::Proto proto;
			Pcap pcap;
			VoidObserver onEventUpdate{};

			void initState() override;

			std::optional<std::string> getSelectedRegion();

			Child build(const Element &element) override;
		};
	};
}// namespace ui