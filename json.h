#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <initializer_list>

namespace json {

    class Node;
    // Сохраните объявления Dict и Array без изменения
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };


    class Node final
        : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
    public:
        using variant::variant;
        using Value = variant;

        Node(Value val);
        const Value& GetValue() const;

        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        int AsInt() const;
        bool AsBool() const;
        double AsDouble() const;
        const std::string& AsString() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;
        Array& AsArray();
        Dict& AsMap();

        bool operator==(const Node& rhs) const;
    };

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

        bool operator==(const Document& r) const;
    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

    // Вывод данных из Node
    void PrintNode(const Node& node, std::ostream& out);

    inline void PrintIntend(size_t intend, std::ostream& out) {
        for (size_t i = 0; i < intend; ++i) {
            out << ' ';
        }
    }

    struct PrintValueSt
    {
        void operator()(std::nullptr_t) const {
            out << "null";
        }

        void operator()(int data) const {
            out << data;
        }

        void operator()(double data) const {
            out << data;
        }

        void operator()(std::string str) const {
            for (size_t i = 0; i < str.length(); ++i) {
                if (str[i] == '\\' || str[i] == '\"') {
                    str.insert(i, "\\");
                    i += 1;
                }
                else if (str[i] == '\r') {
                    str.erase(i, 1);
                    str.insert(i, "\\r");
                }
                else if (str[i] == '\n') {
                    str.erase(i, 1);
                    str.insert(i, "\\n");
                }
            }
            out << "\"" << str << "\"";
        }

        void operator()(bool data) const {
            if (data) {
                out << "true";
            }
            else {
                out << "false";
            }
        }

        void operator()(Array data) const {
            bool flag = true;

            out << "[\n";
            for (auto& part : data) {
                if (flag) {
                    flag = false;
                }
                else {
                    out << ",\n";
                }
                std::visit(PrintValueSt{ out, intend }, part.GetValue());
            }
            out << "\n]";
        }

        void operator()(Dict data) const {
            bool flag = true;

            out << "{\n";
            for (auto& [key, value] : data) {

                if (flag) {
                    flag = false;
                }
                else {
                    out << ",\n";
                }

                out << "\"" << key << "\": ";
                std::visit(PrintValueSt{ out, intend }, value.GetValue());
            }
            out << "\n}";
        }


        std::ostream& out;
        size_t intend;
    };

}  // namespace json