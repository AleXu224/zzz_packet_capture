#include "exportSettings.hpp"
#include "widgets/column.hpp"
#include "widgets/dropdownButton.hpp"
#include "widgets/expander.hpp"
#include "widgets/slider.hpp"

namespace ui {
	squi::core::Child ExportSettings::State::build(const Element &element) {
		return Expander{
			.title = "Export settings",
			.alwaysExpanded = true,
			.content = Column{
				.spacer = ExpanderItemSpacer{},
				.children{
					ExpanderItem{
						.title = "Minimum disc rarity",
						.action = DropdownButton{
							.text = std::format("{}", widget->exportSettings.minDiscRarity),
							.items = [&]() {
								std::vector<ContextMenu::Item> items;
								for (uint8_t i = 3; i <= 5; ++i) {
									items.push_back(ContextMenu::Button{
										.text = std::format("{}", i),
										.callback = [this, i]() {
											widget->exportSettings.minDiscRarity = i;
											setState();
										},
									});
								}
								return items;
							}(),
						},
					},
					ExpanderItem{
						.title = "Minimum disc level",
						.action = Slider{
							.widget{
								.sizeConstraints = BoxConstraints{
									.maxWidth = 200.f,
								},
							},
							.minValue = 0.f,
							.maxValue = 15.f,
							.value = static_cast<float>(widget->exportSettings.minDiscLevel),
							.ticks = Slider::TickInterval{3},
							.onChange = [this](float newVal) {
								setState([&]() {
									widget->exportSettings.minDiscLevel = static_cast<uint8_t>(newVal);
								});
							},
						},
					},
					ExpanderItem{
						.title = "Minimum engine rarity",
						.action = DropdownButton{
							.text = std::format("{}", widget->exportSettings.minEngineRarity),
							.items = [&]() {
								std::vector<ContextMenu::Item> items;
								for (uint8_t i = 3; i <= 5; ++i) {
									items.push_back(ContextMenu::Button{
										.text = std::format("{}", i),
										.callback = [this, i]() {
											widget->exportSettings.minEngineRarity = i;
											setState();
										},
									});
								}
								return items;
							}(),
						},
					},
					ExpanderItem{
						.title = "Minimum engine level",
						.action = Slider{
							.widget{
								.sizeConstraints = BoxConstraints{
									.maxWidth = 200.f,
								},
							},
							.minValue = 0.f,
							.maxValue = 60.f,
							.value = static_cast<float>(widget->exportSettings.minEngineLevel),
							.ticks = Slider::TickInterval{10},
							.onChange = [this](float newVal) {
								setState([&]() {
									widget->exportSettings.minEngineLevel = static_cast<uint8_t>(newVal);
								});
							},
						},
					},
					ExpanderItem{
						.title = "Minimum agent rarity",
						.action = DropdownButton{
							.text = std::format("{}", widget->exportSettings.minAgentRarity),
							.items = [&]() {
								std::vector<ContextMenu::Item> items;
								for (uint8_t i = 4; i <= 5; ++i) {
									items.push_back(ContextMenu::Button{
										.text = std::format("{}", i),
										.callback = [this, i]() {
											widget->exportSettings.minAgentRarity = i;
											setState();
										},
									});
								}
								return items;
							}(),
						},
					},
					ExpanderItem{
						.title = "Minimum agent level",
						.action = Slider{
							.widget{
								.sizeConstraints = BoxConstraints{
									.maxWidth = 200.f,
								},
							},
							.minValue = 0.f,
							.maxValue = 60.f,
							.value = static_cast<float>(widget->exportSettings.minAgentLevel),
							.ticks = Slider::TickInterval{10},
							.onChange = [this](float newVal) {
								setState([&]() {
									widget->exportSettings.minAgentLevel = static_cast<uint8_t>(newVal);
								});
							},
						},
					},
				},
			},
		};
	}
}// namespace ui