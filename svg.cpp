#include "svg.h"

namespace svg {

    using namespace std::literals;

    // Operator<< overloading for enum
    std::ostream& operator<<(std::ostream& os, const StrokeLineCap& data) {
        switch (data) {
        case StrokeLineCap::BUTT:
            os << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            os << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            os << "square"sv;
            break;
        default:
            break;
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& data) {
        switch (data) {
        case StrokeLineJoin::ARCS:
            os << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            os << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            os << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            os << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            os << "round"sv;
            break;
        default:
            break;
        }
        return os;
    }
    // end overloadnig

    // Operator<< overloading for RGB and RGBA

    std::ostream& operator<<(std::ostream& os, const Rgb& data) {
        os << "rgb(" <<
            static_cast<int>(data.red) << "," <<
            static_cast<int>(data.green) << "," <<
            static_cast<int>(data.blue) << ")";

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Rgba& data) {
        os << "rgba(" <<
            static_cast<int>(data.red) << "," <<
            static_cast<int>(data.green) << "," <<
            static_cast<int>(data.blue) << "," << data.opacity << ")";

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Color& data) {
        std::visit(ColorGetter{ os }, data);
        return os;
    }

    // end overloading

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline ----------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    size_t Polyline::NumOfPoints() {
        return points_.size();
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        bool flag = true;
        out << "<polyline points=\""sv;
        for (auto& point : points_) {
            if (!flag) {
                out << " "sv;
            }
            out << point.x << ","sv << point.y;
            flag = false;
        }
        out << "\"";
        if (!IfAttrsSet()) {
            out << " ";
        }

        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Text --------------------

    Text& Text::SetPosition(Point pos) {
        pos_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data) {
        for (size_t i = 0; i < data.size(); i++) {
            if (data[i] == '>') {
                data.erase(i, 1);
                data.insert(i, "&gt;"s);
                i += 4;
            }
            else if (data[i] == '<') {
                data.erase(i, 1);
                data.insert(i, "&lt;"s);
                i += 4;
            }
            else if (data[i] == '\'') {
                data.erase(i, 1);
                data.insert(i, "&apos;"s);
                i += 4;
            }
            else if (data[i] == '\"') {
                data.erase(i, 1);
                data.insert(i, "&quot;"s);
                i += 4;
            }
            else if (data[i] == '&') {
                data.erase(i, 1);
                data.insert(i, "&amp;"s);
                i += 4;
            }
        }
        data_ = data;
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<text";
        RenderAttrs(out);
        out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv
            << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv
            << "font-size=\""sv << size_ << "\""sv;

        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }

        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\"";
        }
        out << ">"sv << data_ << "</text>"sv;
    }

    const Point& Text::GetPoint() {
        return pos_;
    }

    // ---------- Document ----------------

    // Добавляет в svg-документ объект-наследник svg::Object
    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        to_render_.push_back(std::move(obj));
    }

    // Выводит в ostream svg-представление документа
    void Document::Render(std::ostream& out) const { 
        RenderContext context_(out); 
        size_t n = to_render_.size(); 
        size_t i = 0; 
 
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"; 
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n "; 
        for (auto& obj : to_render_) { 
            if (i != 0 || i == n - 1) { 
                out << "\n "; 
            } 
            out << " "; 
            obj->Render(context_); 
            ++i; 
        } 
        out << "\n</svg>"; 
    }
}  // namespace svg

namespace shapes {
    using namespace svg;
    // ---------- Triangle ----------------

    Triangle::Triangle(Point p1, Point p2, Point p3)
        : p1_(p1)
        , p2_(p2)
        , p3_(p3) {

    }

    void Triangle::Draw(ObjectContainer& container) const {
        container.Add(Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
    }

    // ---------- Star --------------------

    Star::Star(Point center, double outer_rad, double inner_rad, int num_rays) {
        for (int i = 0; i <= num_rays; ++i) {
            double angle = 2 * M_PI * (i % num_rays) / num_rays;
            polyline_.AddPoint({ center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle) })
                .SetFillColor("red")
                .SetStrokeColor("black");
            if (i == num_rays) {
                break;
            }
            angle += M_PI / num_rays;
            polyline_.AddPoint({ center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle) })
                .SetFillColor("red")
                .SetStrokeColor("black");
        }
    }

    void Star::Draw(ObjectContainer& container) const {

        container.Add(polyline_);
    }

    // ---------- Snowman -----------------

    Snowman::Snowman(Point center, double radius) {
        Point third_center{ center.x, center.y + radius * 5 };
        circles_.push_back(Circle()
            .SetCenter(third_center)
            .SetRadius(radius * 2)
            .SetFillColor(Rgb{ 240,240,240 })
            .SetStrokeColor("black"));

        Point secont_center{ center.x, center.y + radius * 2 };
        circles_.push_back(Circle()
            .SetCenter(secont_center)
            .SetRadius(radius * 1.5)
            .SetFillColor(Rgb{ 240,240,240 })
            .SetStrokeColor("black"));

        circles_.push_back(Circle()
            .SetCenter(center)
            .SetRadius(radius)
            .SetFillColor(Rgb{ 240,240,240 })
            .SetStrokeColor("black"));
    }

    void Snowman::Draw(ObjectContainer& container) const {
        for (auto& circle : circles_) {
            container.Add(circle);
        }
    }
} // shapes
