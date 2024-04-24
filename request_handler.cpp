#include "request_handler.h"

namespace catalogue_usage {

	Controller::Controller(catalogue::TransportCatalogue catalogue) : catalogue_(catalogue) {

	}

	Controller::~Controller() {

	}

	void Controller::LoadJSON(std::istream& input) {
		json_handle_  = json::JsonHandler{ &catalogue_ };
		json_handle_.LoadJSON(input);
	}

	void Controller::ExecuteJSON(std::ostream& out) {
		json_handle_.ExecuteJSON(out);
	}
}