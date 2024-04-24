#include "json_reader.h"

namespace json {

	JsonHandler::JsonHandler(catalogue::TransportCatalogue* catalogue)
		: catalogue_(catalogue) {}

	JsonHandler::~JsonHandler() {
		delete router_;
	}

	void JsonHandler::ExecuteJSON(std::ostream& out) {
		json::Array ret;

		for (auto& node : requests_.AsArray()) {
			if (node.AsMap().at("type").AsString() == "Stop") {
				ret.push_back(ExecuteStop(node));
			}
			else if (node.AsMap().at("type").AsString() == "Bus") {
				ret.push_back(ExecuteBus(node));
			}
			else if (node.AsMap().at("type").AsString() == "Map") {
				ret.push_back(ExecuteMap(node));
			}
			else if (node.AsMap().at("type").AsString() == "Route") {
				ret.push_back(ExecuteRoute(node));
			}
		}

		json::PrintNode(ret, out);
	}

	void JsonHandler::LoadJSON(std::istream& input) {
		std::vector<Parsed> parsed_data;

		json::Document document = json::Load(input);
		auto& root_node = document.GetRoot();

		for (auto& node : root_node.AsMap().at("base_requests").AsArray()) {
			parsed_data.push_back(PareseNode(node));
		}

		for (auto& parsed : parsed_data) {
			if (std::holds_alternative<Stop>(parsed)) {
				catalogue_->AddStop(std::get<Stop>(parsed));
			}
		}

		for (auto& parsed : parsed_data) {
			if (std::holds_alternative<AddBus>(parsed)) {
				catalogue_->AddBus(ParseBus(std::get<AddBus>(parsed)));
			}
		}

		requests_ = root_node.AsMap().at("stat_requests");
		renderer_ = renderer::MapRenderer{ root_node.AsMap().at("render_settings") };

		renderer_.AddBusesToRender(catalogue_->GetBuses());

		auto &route_settings = root_node.AsMap().at("routing_settings").AsMap();
		auto & cat_ref = *(catalogue_);
		router_ = new router::TransportRouter(
			{ route_settings.at("bus_velocity").AsDouble() * METERS / MINUTES,
			 route_settings.at("bus_wait_time").AsInt() }, cat_ref
		);
	}

	JsonHandler::Parsed JsonHandler::PareseNode(const json::Node& data) {
		if (data.AsMap().at("type").AsString() == "Stop") {
			return ParseStop(data);
		}
		else if (data.AsMap().at("type").AsString() == "Bus") {
			return ParseBus(data);
		}
		else {
			return AddBus{};
		}
	}

	Stop JsonHandler::ParseStop(const json::Node& data) {
		std::unordered_map<std::string, int> stopname_to_dist;
		for (auto& [name, dist] : data.AsMap().at("road_distances").AsMap()) {
			stopname_to_dist[name] = dist.AsInt();
		}

		return Stop{
			data.AsMap().at("name").AsString(),		//name
				{data.AsMap().at("latitude").AsDouble(),  //geo lat
				 data.AsMap().at("longitude").AsDouble()}, //geo lon
			stopname_to_dist }; // distance map
	}

	Bus JsonHandler::ParseBus(AddBus& data) {
		std::vector<Stop*> stops;

		for (auto& name : data.stops) {
			stops.push_back(std::move(catalogue_->GetStopPointer(name)));
		}
		return { data.name, stops, data.is_rounded };
	}

	AddBus JsonHandler::ParseBus(const json::Node& data) {
		std::vector<std::string> stop_names;

		for (auto& stop_name : data.AsMap().at("stops").AsArray()) {
			stop_names.push_back(stop_name.AsString());
		}

		if (!data.AsMap().at("is_roundtrip").AsBool()) {
			std::vector<std::string> r_stops{ stop_names.rbegin() + 1, stop_names.rend() };
			for (auto& stop : r_stops) {
				stop_names.push_back(stop);
			}
		}

		return AddBus{ data.AsMap().at("name").AsString(), stop_names, data.AsMap().at("is_roundtrip").AsBool() };
	}

	json::Node JsonHandler::ExecuteStop(const json::Node& node) {
		using namespace std::literals::string_literals;

		domain::StopData data = catalogue_->GetStopData(
			catalogue_->GetStopPointer(node.AsMap().at("name").AsString()));

		if (data.is_null) {
			json::Node to_add{
				json::Builder{}.StartDict()
					.Key("request_id"s).Value(node.AsMap().at("id").AsInt())
					.Key("error_message"s).Value("not found"s)
				.EndDict().Build()
			};
			return to_add;
		}
		else {
			json::Array buses;
			for (auto& bus : data.buses) {
				buses.push_back(json::Node{ bus });
			}

			json::Node to_add{
				json::Builder{}.StartDict()
					.Key("request_id"s).Value(node.AsMap().at("id").AsInt())
					.Key("buses"s).Value(buses)
				.EndDict().Build()
			};
			return to_add;
		}
	}

	json::Node JsonHandler::ExecuteBus(const json::Node& node) {
		using namespace std::literals::string_literals;

		domain::BusData data = catalogue_->GetBusData(
			catalogue_->GetBusPointer(node.AsMap().at("name").AsString()));

		if (data.is_null) {
			json::Node to_add{
				json::Builder{}.StartDict()
					.Key("request_id"s).Value(node.AsMap().at("id").AsInt())
					.Key("error_message"s).Value("not found"s)
				.EndDict().Build()
			};

			return to_add;
		}
		else {

			json::Node to_add{
				json::Builder{}.StartDict()
					.Key("request_id"s).Value(node.AsMap().at("id").AsInt())
					.Key("stop_count"s).Value(static_cast<int>(data.stops_on_route))
					.Key("unique_stop_count"s).Value(static_cast<int>(data.unique_stops))
					.Key("route_length"s).Value(static_cast<int>(data.path_data.real))
					.Key("curvature"s).Value(data.path_data.curvature)
				.EndDict().Build()
			};

			return to_add;
		}
	}

	json::Node JsonHandler::ExecuteMap(const json::Node& node) {
		using namespace std::literals::string_literals;

		std::ostringstream stream;
		renderer_.RenderBusLines(stream);
		std::string svg_text = stream.str();

		json::Node to_add{
			json::Builder{}.StartDict()
					.Key("request_id"s).Value(node.AsMap().at("id").AsInt())
					.Key("map"s).Value(svg_text)
				.EndDict().Build()
		};
		return to_add;
	}

	json::Node JsonHandler::TransformRouteResponse(router::RouteResponce data) {
		using namespace std::literals::string_literals;

		if (data.total_time == -1) {
			json::Node to_add{
				json::Builder{}.StartDict()
					.Key("request_id"s).Value(data.request_id)
					.Key("error_message"s).Value("not found"s)
				.EndDict().Build()
			};
			return to_add;
		}

		json::Node items{ json::Builder{}.StartArray().EndArray().Build() };
		for (auto item : data.items) {
			json::Node add_item;
			if (item.is_transplant) {
				add_item = std::move(json::Node{
					json::Builder{}.StartDict()
						.Key("stop_name"s).Value(item.name)
						.Key("time"s).Value(item.time)
						.Key("type").Value("Wait"s)
					.EndDict().Build()
					});
			}
			else {
				add_item = std::move(json::Node{
					json::Builder{}.StartDict()
						.Key("bus").Value(item.name)
						.Key("span_count"s).Value(item.span_count)
						.Key("time"s).Value(item.time)
						.Key("type").Value("Bus"s)
					.EndDict().Build()
					});
			}
			items.AsArray().push_back(add_item);
		}

		json::Node to_add{
				json::Builder{}.StartDict()
					.Key("request_id"s).Value(data.request_id)
					.Key("total_time"s).Value(data.total_time)
					.Key("items").Value(items.GetValue())
				.EndDict().Build()
		};
		return to_add;
	}

	json::Node JsonHandler::ExecuteRoute(const json::Node& node) {
		using namespace std::literals::string_literals;

		std::string stop_from = node.AsMap().at("from").AsString();
		std::string stop_to = node.AsMap().at("to").AsString();
		router::RouteResponce responce = router_->GetRouteData(catalogue_->GetStopPointer(stop_from),
																catalogue_->GetStopPointer(stop_to), 
														node.AsMap().at("id").AsInt());

		return TransformRouteResponse(responce);
	}
}