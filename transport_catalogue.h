#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <set>
#include <iostream>
#include <set>
#include <iomanip>
#include <algorithm>

#include "domain.h"
#include "geo.h"


namespace catalogue {
	using namespace domain;

	class TransportCatalogue
	{
	public:
		TransportCatalogue();
		~TransportCatalogue();

		void AddStop(const Stop &stop_to_add);
		void AddBus(const Bus &bus_to_add);

		const StopData GetStopData(Stop* stop);
		const BusData GetBusData(Bus* bus);
		Stop* GetStopPointer(const std::string& str);
		Bus* GetBusPointer(const std::string& str);

		void SetDistanceBetween(Stop* from, Stop* to, int distance, bool if_equal);
		int GetDistanceBetween(Stop* from, Stop* to);

		const std::deque<Stop> GetStops() const;
		const std::deque<Bus> GetBuses() const;
	private:
		std::deque<Stop> stops_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
		std::unordered_map<std::string_view, Bus*> busname_to_bus_;
		std::unordered_map<std::string, std::set<std::string>> stopname_to_buses_;
		std::unordered_map<std::pair<Stop*, Stop*>, double, PairStopToDistanceHash> pairstop_to_distance;

		BusPath CountDistanceOfBus(Bus* bus);
	};
}