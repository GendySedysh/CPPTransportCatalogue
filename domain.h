#pragma once
#include "geo.h"
#include <unordered_map>
#include <math.h>
#include <string>
#include <vector>

namespace domain {

	struct Stop
	{
		std::string name;
		geo::Coordinates location;
		std::unordered_map<std::string, int> stopname_to_dist;

		bool operator==(Stop a) const {
			if (a.name == name && a.location == location) {
				return true;
			}
			return false;
		}
	};

	struct Bus
	{
		std::string name;
		std::vector<Stop*> stops;
		bool is_rounded;
	};

	struct AddBus
	{
		std::string name;
		std::vector<std::string> stops;
		bool is_rounded;
	};

	struct StopData
	{
		bool is_null;
		size_t num_of_buses;
		std::vector<std::string> buses;
	};

	struct BusPath
	{
		int real;
		double geographical;
		double curvature;
	};

	struct BusData
	{
		bool is_null;
		size_t stops_on_route;
		size_t unique_stops;
		BusPath path_data;
	};

	struct PairStopToDistanceHash
	{
		std::size_t operator()(std::pair<Stop*, Stop*> stops) const noexcept
		{
			std::size_t h1 = std::hash<std::string>{}(stops.first->name) +
				static_cast<size_t>(stops.first->location.lat * 11) +
				static_cast<size_t>(stops.first->location.lng * std::pow(11, 2));

			std::size_t h2 = std::hash<std::string>{}(stops.second->name) +
				static_cast<size_t>(stops.second->location.lat * 11) +
				static_cast<size_t>(stops.second->location.lng * std::pow(11, 2));

			return h1 + h2;
		}
	};

	struct hash_pair {
		template <class T1, class T2>
		size_t operator()(const std::pair<T1, T2>& p) const
		{
			auto hash1 = std::hash<T1>{}(p.first);
			auto hash2 = std::hash<T2>{}(p.second);

			if (hash1 != hash2) {
				return hash1 ^ hash2;
			}

			// If hash1 == hash2, their XOR is zero.
			return hash1;
		}
	};
} //domain