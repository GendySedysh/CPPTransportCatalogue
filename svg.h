#pragma once
#define _USE_MATH_DEFINES

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <ostream>
#include <optional>
#include <variant>
#include <stdint.h>

namespace svg {

    struct Rgb
    {
        Rgb() : red(0),
            green(0),
            blue(0) {}

        Rgb(uint8_t r, uint8_t g, uint8_t b)
            : red(r),
            green(g),
            blue(b) {}

        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    struct Rgba
    {
        Rgba() : red(0),
            green(0),
            blue(0),
            opacity(1.0) {}

        Rgba(uint8_t r, uint8_t g, uint8_t b, double o)
            : red(r),
            green(g),
            blue(b),
            opacity(o) {}

        uint8_t red;
        uint8_t green;
        uint8_t blue;
        double opacity;
    };

    std::ostream& operator<<(std::ostream& os, const Rgb& data);
    std::ostream& operator<<(std::ostream& os, const Rgba& data);

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
    inline const Color NoneColor{ std::monostate{} };

    struct ColorGetter
    {
        void operator()(std::monostate) const {
            os << "none";
        }

        void operator()(std::string str) const {
            os << str;
        }

        void operator()(Rgb col) const {
            os << col;
        }

        void operator()(Rgba col) const {
            os << col;
        }

        std::ostream& os;
    };

    std::ostream& operator<<(std::ostream& os, const Color& data);

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& os, const StrokeLineCap& data);
    std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& data);

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0.0;
        double y = 0.0;

    };

    inline bool operator==(const Point lhs, const Point rhs) {
        if (lhs.x == rhs.x && lhs.y == rhs.y) {
            return true;
        }
        return false;
    }

    inline bool operator!=(const Point lhs, const Point rhs) {
        return !(lhs == rhs);
    }

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    template <typename Owner>
    class PathProps {
    public:

        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeWidth(double width) {
            stroke_width_ = width;
            return AsOwner();
        }
        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_line_cap_ = line_cap;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_line_join_ = line_join;
            return AsOwner();
        }

    protected:
        virtual ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_line_cap_) {
                out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
            }
            if (stroke_line_join_) {
                out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
            }
        }

        bool IfAttrsSet() const {
            if (fill_color_ || stroke_color_ ||
                stroke_width_ || stroke_line_cap_ || stroke_line_join_) {
                return true;
            }
            return false;
        }

    private:
        Owner& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            // если класс Owner — наследник PathProps
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_ = std::nullopt;
        std::optional<Color> stroke_color_ = std::nullopt;
        std::optional<double> stroke_width_ = std::nullopt;
        std::optional<StrokeLineCap> stroke_line_cap_ = std::nullopt;
        std::optional<StrokeLineJoin> stroke_line_join_ = std::nullopt;
    };

    /*
     * Класс Circle моделирует элемент <circle> для отображения круга
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_ = { 0,0 };
        double radius_ = 1.0;
    };

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline : public Object, public PathProps<Polyline> {
    public:
        //Polyline();
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);
        size_t NumOfPoints();

        /*
         * Прочие методы и данные, необходимые для реализации элемента <polyline>
         */
    private:
        void RenderObject(const RenderContext& context) const override;
        std::vector<Point> points_;
    };

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text : public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);
        const Point& GetPoint();

        // Прочие данные и методы, необходимые для реализации элемента <text>
    private:
        void RenderObject(const RenderContext& context) const override;

        Point pos_ = { 0,0 };
        Point offset_ = { 0,0 };
        uint32_t size_ = 1;
        std::string font_family_ = "";
        std::string font_weight_ = "";
        std::string data_ = "";
    };


    class ObjectContainer {
    public:
        virtual ~ObjectContainer() = default;

        template <typename Obj>
        void Add(Obj obj) {
            to_render_.emplace_back(std::make_unique<Obj>(obj));
        }

        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    protected:
        std::vector<std::unique_ptr<Object>> to_render_;
    };

    class Document : public ObjectContainer {
    public:
        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;
    };

    class Drawable
    {
    public:
        virtual ~Drawable() = default;
        virtual void Draw(ObjectContainer& container) const = 0;

    private:

    };

}  // namespace svg

namespace shapes {
    using namespace svg;

    class Triangle : public Drawable {
    public:
        Triangle(Point p1, Point p2, Point p3);

        void Draw(ObjectContainer& container) const override;

    private:
        Point p1_, p2_, p3_;
    };

    class Star : public Drawable {
    public:
        Star(Point center, double outer_rad, double inner_rad, int num_rays);

        void Draw(ObjectContainer& container) const override;

    private:
        Polyline polyline_;
    };

    class Snowman : public Drawable {
    public:
        Snowman(Point center, double radius);

        void Draw(ObjectContainer& container) const override;

    private:
        std::vector<Circle> circles_;
    };
} // shapes