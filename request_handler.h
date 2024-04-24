#pragma once

#include <variant>
#include <math.h>

#include "domain.h"
#include "transport_catalogue.h"
#include "json_reader.h"

namespace catalogue_usage {
	using namespace domain;

	class Controller
	{
		using Parsed = std::variant<Stop, AddBus>;

	public:
		Controller(catalogue::TransportCatalogue catalogue);
		~Controller();

		void LoadJSON(std::istream& input);
		void ExecuteJSON(std::ostream& out);

	private:
		catalogue::TransportCatalogue catalogue_;
		json::JsonHandler json_handle_;
	};
}