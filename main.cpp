#include "request_handler.h"
#include "transport_catalogue.h"
#include <iostream>
#include <locale.h>
#include <iostream>

int main() {
	catalogue::TransportCatalogue catalogue;
	catalogue_usage::Controller controller(catalogue);

	controller.LoadJSON(std::cin);
	controller.ExecuteJSON(std::cout);

	return 0;
}
