#pragma once
#include "graph.h"
#include "router.h"
#include "json.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "json_builder.h"
#include <memory>

namespace router {
	using namespace domain;

	struct RouteItem
	{
		double time;
		int span_count;
		std::string name;
		bool is_transplant;
	};

	struct RouteResponce
	{
		std::vector<RouteItem> items;
		int request_id = 0;
		double total_time = 0;
	};


	struct RouteSettings
	{
		double bus_velocity = 0;
		int bus_wait_time = 0;
	};

	struct RouteStop {
		std::string stop_name = "";
		size_t point_coming = 0;
		size_t point_leaving = 0;
	};

	struct EdgeInfo {
		size_t point_from = 0;
		size_t point_to = 0;

		bool is_transplant = true;
		std::string bus = "";
		double weight = 0.0;
		int span_count = 0;
	};

	class TransportRouter
	{
	public:
		TransportRouter(RouteSettings new_settings, catalogue::TransportCatalogue &cat_ref);
		RouteResponce GetRouteData(Stop* from, Stop* to, size_t request_id);
		~TransportRouter();

	private:
		RouteSettings settings_;

		std::unique_ptr<graph::Router<double>> router_ptr_;
		std::unique_ptr<graph::DirectedWeightedGraph<double>> local_graph_;

		catalogue::TransportCatalogue &catalogue_ref_;
		std::unordered_map<Stop*, RouteStop> stop_to_routestop;
		std::unordered_map<size_t, Stop*> routepoint_to_stop;
		std::unordered_map<std::pair<size_t, size_t>, double, hash_pair> distance_storage;
		std::unordered_map<size_t, EdgeInfo> id_to_edge;

		void SetUpRouter();
		void SetStopEdges();
		void SetBusEdges();

		void AddBusDistances(std::vector<Stop*>& stops_in_bus, std::string name);
		RouteResponce Response(size_t request_id, size_t from_id, std::optional<graph::Router<double>::RouteInfo> data);
		RouteItem GetItemFromEdge(size_t edge_id);
	};
}