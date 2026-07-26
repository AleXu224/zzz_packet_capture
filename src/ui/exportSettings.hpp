#pragma once

#include "core/core.hpp"
#include "data/exportSettings.hpp"

namespace ui {
	using namespace squi;

	struct ExportSettings : StatefulWidget {
		// Args
		Key key;
		Args widget{};
		data::ExportSettings &exportSettings;

		struct State : WidgetState<ExportSettings> {
			Child build(const Element &element) override;
		};
	};
}// namespace ui