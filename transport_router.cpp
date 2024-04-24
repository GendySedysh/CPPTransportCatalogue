#include "transport_router.h"

namespace router {

	static int EdgeId_ = 0;

	TransportRouter::TransportRouter(RouteSettings new_settings, catalogue::TransportCatalogue& cat_ref):
		settings_(new_settings), catalogue_ref_(cat_ref){
		SetUpRouter();
	}

	TransportRouter::~TransportRouter() {
	}

	void TransportRouter::SetUpRouter() {
		SetStopEdges();
		SetBusEdges();

		local_graph_ = std::move(std::make_unique<graph::DirectedWeightedGraph<double>>(catalogue_ref_.GetStops().size() * 2));
		for (auto& [id, edgeInfo] : id_to_edge) {
			local_graph_->AddEdge({ edgeInfo.point_from, edgeInfo.point_to, edgeInfo.weight });
		}
	}

	void TransportRouter::AddBusDistances(std::vector<Stop*>& stops_in_bus, std::string name) {
		for (int i = 0; i < stops_in_bus.size(); ++i) {
			int span_num = 0;
			int dist = 0;
			for (int j = i + 1; j < stops_in_bus.size(); ++j) {
				// Обрабатываем пары остановок
				++span_num;
				auto start_ptr = stops_in_bus[i];
				auto end_ptr = stops_in_bus[j];
				int dist_to_add = catalogue_ref_.GetDistanceBetween(stops_in_bus[j - 1], end_ptr);
				if (dist_to_add == 0) {
					dist_to_add = catalogue_ref_.GetDistanceBetween(end_ptr, stops_in_bus[j - 1]);
				}
				dist += dist_to_add;
				distance_storage[{stop_to_routestop[start_ptr].point_leaving, stop_to_routestop[end_ptr].point_coming}] = dist / settings_.bus_velocity;
				id_to_edge[EdgeId_++] = EdgeInfo(stop_to_routestop[start_ptr].point_leaving,
														 stop_to_routestop[end_ptr].point_coming,
														 false, name, dist / settings_.bus_velocity, span_num);
			}
		}
	}

	void TransportRouter::SetStopEdges() {
		auto stop_list = catalogue_ref_.GetStops();
		size_t i = 0;
		for (auto& stop : stop_list) {
			auto tmp_ptr = catalogue_ref_.GetStopPointer(stop.name);
			stop_to_routestop[tmp_ptr] = { stop.name, i, i + 1 };
			routepoint_to_stop[i] = tmp_ptr;
			routepoint_to_stop[i + 1] = tmp_ptr;
			distance_storage[{ i, i + 1 }] = settings_.bus_wait_time;
			id_to_edge[EdgeId_++] = EdgeInfo(i, i + 1, true, "", settings_.bus_wait_time, 0);
			i += 2;
		}
	}

	void TransportRouter::SetBusEdges() {
		for (auto& buses : catalogue_ref_.GetBuses()) {
			if (buses.is_rounded) {
				auto stops_in_bus = buses.stops;
				AddBusDistances(stops_in_bus, buses.name);
			}
			else {
				size_t reverse_point = (buses.stops.size() / 2) + 1;
				auto stops_in_bus = std::vector<Stop*>{ buses.stops.begin(), buses.stops.begin() + reverse_point };
				AddBusDistances(stops_in_bus, buses.name);
				stops_in_bus = { stops_in_bus.rbegin(), stops_in_bus.rend() };
				AddBusDistances(stops_in_bus, buses.name);
			}
		}
	}

	RouteItem TransportRouter::GetItemFromEdge(size_t edge_id) {
		using namespace std::literals::string_literals;
		router::EdgeInfo &data = id_to_edge.at(edge_id);

		RouteItem item;
		item.is_transplant = data.is_transplant;
		if (data.is_transplant) {
			item.name = routepoint_to_stop[data.point_from]->name;
			item.time = settings_.bus_wait_time;
			return item;
		}

		item.name = data.bus;
		item.span_count = data.span_count;
		item.time = data.weight;
		return item;
	}

	RouteResponce TransportRouter::Response(size_t request_id, size_t from_id,
										 std::optional<graph::Router<double>::RouteInfo> data) {
		RouteResponce response;

		if (data.has_value()) {
			response.request_id = request_id;
			response.total_time = data->weight + settings_.bus_wait_time;
			// Отправная точка
			response.items.push_back(RouteItem{ settings_.bus_wait_time * 1.0 , 0, routepoint_to_stop[from_id]->name, true });
			// Остальные рёбра
			for (auto& edge : data->edges) {
				response.items.push_back(GetItemFromEdge(edge));
			}
		}
		else {
			response.request_id = request_id;
			response.total_time = -1.0;
		}
		return response;
	}

	RouteResponce TransportRouter::GetRouteData(Stop* from, Stop* to, size_t request_id) {
		using namespace std::literals::string_literals;
		
		if (router_ptr_.get() == nullptr) {
			router_ptr_ = std::move(std::make_unique<graph::Router<double>>(*local_graph_.get()));
		}

		if (from == to) {
			RouteResponce blank;

			blank.request_id = request_id;
			blank.total_time = 0;
			return blank;
		}

		size_t from_id = stop_to_routestop[from].point_leaving;
		size_t to_id = stop_to_routestop[to].point_coming;

		auto data = router_ptr_->BuildRoute(from_id, to_id);
		return Response(request_id, from_id, data);
	}
}
