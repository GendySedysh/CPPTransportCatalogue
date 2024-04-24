#include "map_renderer.h"

namespace renderer {

	bool IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

	MapRenderer::MapRenderer(const json::Node& data){
		const json::Dict &renderer_data = data.AsMap();

		width_ = renderer_data.at("width").AsDouble();
		height_ = renderer_data.at("height").AsDouble();

		padding_ = renderer_data.at("padding").AsDouble();
		line_width_ = renderer_data.at("line_width").AsDouble();
		stop_radius_ = renderer_data.at("stop_radius").AsDouble();

		bus_label_font_size_ = renderer_data.at("bus_label_font_size").AsInt();
		bus_label_offset_ = { 
			renderer_data.at("bus_label_offset").AsArray().front().AsDouble(),
			renderer_data.at("bus_label_offset").AsArray().back().AsDouble() 
		};

		stop_label_font_size_ = renderer_data.at("stop_label_font_size").AsInt();
		stop_label_offset_ = {
			renderer_data.at("stop_label_offset").AsArray().front().AsDouble(),
			renderer_data.at("stop_label_offset").AsArray().back().AsDouble()
		};

		underlayer_color_ = ParseColor(renderer_data.at("underlayer_color"));
		underlayer_width_ = renderer_data.at("underlayer_width").AsDouble();

		color_palette_.clear();
		for (auto& node : renderer_data.at("color_palette").AsArray()) {
			color_palette_.push_back(ParseColor(node));
		}
	}

	svg::Color MapRenderer::ParseColorRGB(const json::Array &color_data) {
		return svg::Color{
			svg::Rgb {
				static_cast<uint8_t>(color_data.at(0).AsInt()),
				static_cast<uint8_t>(color_data.at(1).AsInt()),
				static_cast<uint8_t>(color_data.at(2).AsInt())
			}
		};
	}

	svg::Color MapRenderer::ParseColorRGBA(const json::Array &color_data) {
		return svg::Color{
			svg::Rgba {
				static_cast<uint8_t>(color_data.at(0).AsInt()),
				static_cast<uint8_t>(color_data.at(1).AsInt()),
				static_cast<uint8_t>(color_data.at(2).AsInt()),
				color_data.at(3).AsDouble(),
			}
		};
	}

	svg::Color MapRenderer::ParseColor(const json::Node& data) {
		if (data.IsString()) {
			return svg::Color{ data.AsString() };
		}
		else if (data.IsArray()) {
			json::Array color_data = data.AsArray();
			if (color_data.size() == 3) {
				return ParseColorRGB(color_data);
			}
			else {
				return ParseColorRGBA(color_data);
			}
		}
		return svg::Color{};
	}

	void MapRenderer::AddBusesToRender(const std::deque<domain::Bus>& buses) {
		std::vector<geo::Coordinates> geo_coords;

		// Добавляем все нужные координаты
		for (auto& bus : buses) {
			for (auto& stop : bus.stops) {
				geo_coords.push_back({ stop->location.lat, stop->location.lng });
			}
		}

		// Создаём проектор сферических координат на карту
		const SphereProjector proj{
			geo_coords.begin(), geo_coords.end(), width_, height_, padding_
		};


		// Для каждого автобуса создаём по ломаной
		for (auto& bus : buses) {
			geo_coords.clear();

			for (auto& stop : bus.stops) {
				geo_coords.push_back({ stop->location.lat, stop->location.lng });
			}

			size_t count = 0; // флаг для добавления названия маршрута
			svg::Polyline line;
			// Проецируем и выводим координаты
			for (const auto geo_coord : geo_coords) {
				const svg::Point screen_coord = proj(geo_coord);
				// добавляем точку в ломаную
				line.AddPoint(screen_coord);
				// добавляем координату остановки
				if (stops_.count(bus.stops.at(count)->name) == 0) {
					stops_[bus.stops.at(count)->name] = screen_coord;
				}
				// добавляем координаты названия автобуса
				if (count == 0 || (!bus.is_rounded && bus.stops.size() / 2 == count)) {
					if (bus_names_[bus.name].size() == 0 ||
							(bus_names_[bus.name].size() != 0 &&
							bus_names_[bus.name].at(0).GetPoint() != screen_coord)) {

						bus_names_[bus.name].push_back(svg::Text().SetPosition(screen_coord));
					}
				}
				++count;
			}
			bus_lines_.insert({ bus.name, line }); // записваем линию автобуса
		}
	}

	void MapRenderer::RenderBusLines(std::ostream& out) {
		svg::Document doc;
		size_t color_index = 0;

		// рисуем линии
		for (auto &[name, line]: bus_lines_) {
			doc.Add(line
				.SetStrokeWidth(line_width_)
				.SetStrokeColor(color_palette_.at(color_index))
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
				.SetFillColor(svg::NoneColor)
			);

			if (line.NumOfPoints() != 0) {
				color_index++;
			}

			if (color_index >= color_palette_.size()) {
				color_index = 0;
			}
		}

		// рисуем названия автобусов
		color_index = 0;
		for (auto& [name, names] : bus_names_) {
			for (auto& b_name : names) {
				svg::Text underline = b_name;
				doc.Add(underline
					.SetOffset(bus_label_offset_)
					.SetFontSize(bus_label_font_size_)
					.SetFontFamily("Verdana")
					.SetFontWeight("bold")
					.SetData(name)
					.SetFillColor(underlayer_color_)
					.SetStrokeColor(underlayer_color_)
					.SetStrokeWidth(underlayer_width_)
					.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
					.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
				);

				doc.Add(b_name
					.SetOffset(bus_label_offset_)
					.SetFontSize(bus_label_font_size_)
					.SetFontFamily("Verdana")
					.SetFontWeight("bold")
					.SetData(name)
					.SetFillColor(color_palette_.at(color_index))
				);
			}

			if (names.size() != 0) {
				color_index++;
			}

			if (color_index >= color_palette_.size()) {
				color_index = 0;
			}
		}

		// рисуем остановки
		for (auto& [stopname, point] : stops_) {
			doc.Add(svg::Circle()
				.SetCenter(point)
				.SetRadius(stop_radius_)
				.SetFillColor("white")
			);
		}

		// рисуем названия остановок
		for (auto& [stopname, point] : stops_) {
			doc.Add(svg::Text()
				.SetPosition(point)
				.SetOffset(stop_label_offset_)
				.SetFontSize(stop_label_font_size_)
				.SetFontFamily("Verdana")
				.SetData(stopname)
				.SetFillColor(underlayer_color_)
				.SetStrokeColor(underlayer_color_)
				.SetStrokeWidth(underlayer_width_)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			);

			doc.Add(svg::Text()
				.SetPosition(point)
				.SetOffset(stop_label_offset_)
				.SetFontSize(stop_label_font_size_)
				.SetFontFamily("Verdana")
				.SetData(stopname)
				.SetFillColor("black")
			);
		}

		doc.Render(out);
	}

}//renderer