#include "app.h"

int main()
{
	App app({ 1380, 800 });

	try {
		app.run();
	}
	catch (std::exception& e) {
		fmt::print("exception: {}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
