#include "core/app.hpp"
#include "src/dispatch/dispatch.hpp"
#include "ui/home.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char **argv) {
	using namespace squi;

#ifdef _WIN32
	// Attach console to the parent process for debugging
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		FILE *stream = nullptr;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
	}
#endif

	if (argc > 1 && std::string_view{argv[1]} == "--refresh-seed") {
		try {
			auto seeds = dispatch::deriveInitialSeeds();
			for (const auto &[title, seed]: seeds) {
				std::println("New seed for {}: {:016X}", title, seed);
			}
		} catch (const std::exception &e) {
			std::println(stderr, "dispatch error: {}", e.what());
			return 1;
		}
		return 0;
	}

	auto systemTheme = Theme::getSystemAccentColor();

	core::App app{
		.windowOptions{
			.name = "ZZZ Packet Capture",
		},
		.child = ui::Home{},
		.theme = Theme{
			.accent = systemTheme.value_or(Theme{}.accent),
		},
	};
	app.initialize();
	App::runAllWindows();
}
