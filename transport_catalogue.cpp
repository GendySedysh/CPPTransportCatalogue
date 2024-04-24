#include "transport_catalogue.h"

namespace catalogue {

	TransportCatalogue::TransportCatalogue() {
	}

	TransportCatalogue::~TransportCatalogue() {

	}

	void TransportCatalogue::AddStop(const Stop &stop_to_add) {
		stops_.push_back(std::move(stop_to_add));
		stopname_to_stop_[stops_.back().name] = &(stops_.back());
	}

	void TransportCatalogue::AddBus(const Bus &bus_to_add) {
		buses_.push_back(std::move(bus_to_add));
		busname_to_bus_[buses_.back().name] = &(buses_.back());

		// заполняем hash-map для поиска маршрутов по остановке
		for (auto tmp : buses_.back().stops) {
			stopname_to_buses_[tmp->name].insert(buses_.back().name);
		}
	}

	Stop* TransportCatalogue::GetStopPointer(const std::string& str) {
		if (stopname_to_stop_.count(str) != 0) {
			return stopname_to_stop_.at(str);
		}
		return nullptr;
	}

	Bus* TransportCatalogue::GetBusPointer(const std::string& str) {
		if (busname_to_bus_.count(str) != 0) {
			return busname_to_bus_.at(str);
		}
		return nullptr;
	}

	const StopData TransportCatalogue::GetStopData(Stop* stop) {

		if (stop == nullptr) {
			return {true, 0, {}};
		}
		else {
			if (stopname_to_buses_.count(stop->name) == 0) {
				return {false, 0, {} };
			}
			else
			{
				std::vector<std::string> buses;
				for (auto& bus : stopname_to_buses_.at(stop->name)) {
					buses.push_back(bus);
				}

				return {false, buses.size(), buses };
			}
		}
	}

	const BusData TransportCatalogue::GetBusData(Bus* bus) {
		if (bus == nullptr) {
			return { true, 0, 0, {0, 0, 0} };
		}
		else {
			return {
				false,
				bus->stops.size(),
				std::set<Stop*>(bus->stops.begin(), bus->stops.end()).size(),
				CountDistanceOfBus(bus)
			};
		}
	}

	/*
		возвращает пару <длина маршрута, кривизна маршрута>
	*/
	BusPath TransportCatalogue::CountDistanceOfBus(Bus* bus) {
		double length = 0;
		int distance = 0;

		auto stop_from = bus->stops.begin();
		auto stop_to = std::next(stop_from);

		for (; stop_to < bus->stops.end(); ++stop_to) {
			// дорожное расстояние
			if ((*stop_from)->stopname_to_dist.count((*stop_to)->name) != 0) {
				distance += (*stop_from)->stopname_to_dist.at((*stop_to)->name);
			}
			else if ((*stop_to)->stopname_to_dist.count((*stop_from)->name) != 0) {
				distance += (*stop_to)->stopname_to_dist.at((*stop_from)->name);
			}

			// прямое расстояние
			if (pairstop_to_distance.count({ *stop_from, *stop_to }) == 0) {
				double tmp_l = geo::ComputeDistance((*stop_from)->location, (*stop_to)->location);
				pairstop_to_distance[{ *stop_from, * stop_to }] = tmp_l;
				length += tmp_l;
			}
			else {
				length += pairstop_to_distance.at({ *stop_from, *stop_to });
			}
			++stop_from;
		}

		// добавление закругления маршрута
		if ((*bus->stops.begin())->stopname_to_dist.count((*bus->stops.begin())->name) != 0) {
			distance += (*bus->stops.begin())->stopname_to_dist.at((*bus->stops.begin())->name);
		}
		return { distance, length, distance / length };
	}

	void TransportCatalogue::SetDistanceBetween(Stop* from, Stop* to, int distance, bool if_equal) {
		from->stopname_to_dist[to->name] = distance;

		if (if_equal) {
			to->stopname_to_dist[from->name] = distance;
		}
	}

	int TransportCatalogue::GetDistanceBetween(Stop* from, Stop* to) {
		if (from->stopname_to_dist.count(to->name) != 0) {
			return from->stopname_to_dist.at(to->name);
		}
		//if (to->stopname_to_dist.count(from->name) != 0) {
		//	return to->stopname_to_dist.at(from->name);
		//}
		return 0;
	}

	const std::deque<Stop> TransportCatalogue::GetStops() const {
		return stops_;
	}

	const std::deque<Bus> TransportCatalogue::GetBuses() const {
		return buses_;
	}
} // end namespace catalogue