#include "exportSettings.hpp"
#include "widgets/column.hpp"
#include "widgets/dropdownButton.hpp"
#include "widgets/expander.hpp"
#include "widgets/row.hpp"
#include "widgets/slider.hpp"
#include "widgets/text.hpp"
#include "widgets/toggleSwitch.hpp"

namespace ui {
	squi::core::Child ExportSettings::State::build(const Element &element) {
		return Expander{
			.title = "Export settings",
			.alwaysExpanded = true,
			.content = Column{
				.spacer = ExpanderItemSpacer{},
				.children{
					ExpanderItem{
						.icon = ToggleSwitch{
							.active = widget->exportSettings.exportDiscs,
							.statePosition = ToggleSwitch::StatePosition::None,
							.onToggle = [this](bool val) {
								setState([&]() {
									widget->exportSettings.exportDiscs = val;
								});
							},
						},
						.title = "Discs",
						.action = Row{
							.widget{
								.width = Size::Wrap,
								.height = Size::Wrap,
							},
							.crossAxisAlignment = Flex::Alignment::center,
							.spacing = 8.f,
							.children{
								Text{.text = "Min level"},
								Slider{
									.widget{
										.sizeConstraints{
											.maxWidth = 200.f,
										},
									},
									.minValue = 0.f,
									.maxValue = 15.f,
									.value = static_cast<float>(widget->exportSettings.minDiscLevel),
									.disabled = !widget->exportSettings.exportDiscs,
									.ticks = Slider::TickInterval{3},
									.onChange = [this](float newVal) {
										setState([&]() {
											widget->exportSettings.minDiscLevel = static_cast<uint8_t>(newVal);
										});
									},
								},
								DropdownButton{
									.disabled = !widget->exportSettings.exportDiscs,
									.text = std::format("Min rarity {}", widget->exportSettings.minDiscRarity),
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
						},
					},
					ExpanderItem{
						.icon = ToggleSwitch{
							.active = widget->exportSettings.exportAgents,
							.statePosition = ToggleSwitch::StatePosition::None,
							.onToggle = [this](bool val) {
								setState([&]() {
									widget->exportSettings.exportAgents = val;
								});
							},
						},
						.title = "Agents",
						.action = Row{
							.widget{
								.width = Size::Wrap,
								.height = Size::Wrap,
							},
							.crossAxisAlignment = Flex::Alignment::center,
							.spacing = 8.f,
							.children{
								Text{.text = "Min level"},
								Slider{
									.widget{
										.sizeConstraints{
											.maxWidth = 200.f,
										},
									},
									.minValue = 0.f,
									.maxValue = 60.f,
									.value = static_cast<float>(widget->exportSettings.minAgentLevel),
									.disabled = !widget->exportSettings.exportAgents,
									.ticks = Slider::TickInterval{10},
									.onChange = [this](float newVal) {
										setState([&]() {
											widget->exportSettings.minAgentLevel = static_cast<uint8_t>(newVal);
										});
									},
								},
								DropdownButton{
									.disabled = !widget->exportSettings.exportAgents,
									.text = std::format("Min rarity {}", widget->exportSettings.minAgentRarity),
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
						},
					},
					ExpanderItem{
						.icon = ToggleSwitch{
							.active = widget->exportSettings.exportEngines,
							.statePosition = ToggleSwitch::StatePosition::None,
							.onToggle = [this](bool val) {
								setState([&]() {
									widget->exportSettings.exportEngines = val;
								});
							},
						},
						.title = "Engines",
						.action = Row{
							.widget{
								.width = Size::Wrap,
								.height = Size::Wrap,
							},
							.crossAxisAlignment = Flex::Alignment::center,
							.spacing = 8.f,
							.children{
								Text{.text = "Min level"},
								Slider{
									.widget{
										.sizeConstraints{
											.maxWidth = 200.f,
										},
									},
									.minValue = 0.f,
									.maxValue = 60.f,
									.value = static_cast<float>(widget->exportSettings.minEngineLevel),
									.disabled = !widget->exportSettings.exportEngines,
									.ticks = Slider::TickInterval{10},
									.onChange = [this](float newVal) {
										setState([&]() {
											widget->exportSettings.minEngineLevel = static_cast<uint8_t>(newVal);
										});
									},
								},
								DropdownButton{
									.disabled = !widget->exportSettings.exportEngines,
									.text = std::format("Min rarity {}", widget->exportSettings.minEngineRarity),
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
						},
					},
				},
			},
		};
	}
}// namespace ui