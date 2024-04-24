#include "json.h"

using namespace std;

namespace json {

    namespace {

        Node LoadNode(istream& input);

        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
                };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
                };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return Node{ std::stoi(parsed_num) };
                    }
                    catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return Node{ std::stod(parsed_num) };
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        std::string ReadNLetters(std::istream& input, size_t n) {
            std::string str;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();

            while (str.length() != n)
            {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                str.push_back(*it);
                ++it;
            }
            return str;
        }

        std::string LoadString(std::istream& input) {
            using namespace std::literals;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    // Поток закончился до того, как встретили закрывающую кавычку?
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    // Встретили закрывающую кавычку
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    // Встретили начало escape-последовательности
                    ++it;
                    if (it == end) {
                        // Поток завершился сразу после символа обратной косой черты
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                    switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        // Встретили неизвестную escape-последовательность
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    // Просто считываем очередной символ и помещаем его в результирующую строку
                    s.push_back(ch);
                }
                ++it;
            }
            return s;
        }

        Node LoadArray(istream& input) {
            Array result;
            bool flag = true;
            int i = 0;

            for (char c; input >> c;) {
                if (c == ']') {
                    ++i;
                    flag = false;
                    break;
                }

                if (c != ',') {
                    input.putback(c);
                }


                result.push_back(LoadNode(input));
                ++i;
            }

            if (flag || i == 0) {
                throw ParsingError("No closing '}' symbol"s);
            }
            return Node(move(result));
        }

        Node LoadDict(istream& input) {
            Dict result;
            bool flag = true;
            int i = 0;

            for (char c; input >> c;) {
                if (c == '}') {
                    flag = false;
                    ++i;
                    break;
                }

                if (c == ',') {
                    input >> c;
                }


                string key = LoadString(input);
                input >> c;
                result.insert({ move(key), LoadNode(input) });
                i++;
            }

            if (flag || i == 0) {
                throw ParsingError("No closing '}' symbol"s);
            }

            return Node(move(result));
        }

        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return Node{ std::move(LoadString(input)) };
            }
            else if (c == 'n') {
                input.putback(c);
                std::string str = ReadNLetters(input, 4);
                if (str != "null") {
                    throw ParsingError("Unnown in line"s);
                }
                return Node();
            }
            else if (c == 'f') {
                input.putback(c);
                std::string str = ReadNLetters(input, 5);
                if (str != "false") {
                    throw ParsingError("Unnown in line"s);
                }
                return Node{ false };
            }
            else if (c == 't') {
                input.putback(c);
                std::string str = ReadNLetters(input, 4);
                if (str != "true") {
                    throw ParsingError("Unnown in line"s);
                }
                return Node{ true };
            }
            else if (c == '-' || std::isdigit(c)) {
                input.putback(c);
                return LoadNumber(input);
            }
            else if (c) {
                throw ParsingError("Unnown symbol in beginning of line"s);
            }
            return Node{};
        }

    }  // namespace

    Node::Node(Value val) {
        std::swap(val, *this);
    }

    const Node::Value& Node::GetValue() const {
        return *this;
    }

    bool Node::IsInt() const {
        if (std::holds_alternative<int>(*this)) {
            return true;
        }
        return false;
    }

    bool Node::IsDouble() const {
        if (IsInt() || std::holds_alternative<double>(*this)) {
            return true;
        }
        return false;
    }

    bool Node::IsPureDouble() const {
        if (std::holds_alternative<double>(*this) && !IsInt()) {
            return true;
        }
        return false;
    }

    bool Node::IsBool() const {
        if (std::holds_alternative<bool>(*this)) {
            return true;
        }
        return false;
    }

    bool Node::IsString() const {
        if (std::holds_alternative<std::string>(*this)) {
            return true;
        }
        return false;
    }

    bool Node::IsNull() const {
        if (std::holds_alternative<std::nullptr_t>(*this)) {
            return true;
        }
        return false;
    }

    bool Node::IsArray() const {
        if (std::holds_alternative<Array>(*this)) {
            return true;
        }
        return false;
    }

    bool Node::IsMap() const {
        if (std::holds_alternative<Dict>(*this)) {
            return true;
        }
        return false;
    }

    int Node::AsInt() const {
        if (IsInt()) {
            return std::get<int>(*this);
        }

        throw logic_error("");
    }

    bool Node::AsBool() const {
        if (IsBool()) {
            return std::get<bool>(*this);
        }
        throw logic_error("");
    }

    double Node::AsDouble() const {
        if (IsPureDouble()) {
            return std::get<double>(*this);
        }
        else if (IsDouble()) {
            return static_cast<double>(AsInt());
        }
        throw logic_error("");
    }

    const std::string& Node::AsString() const {
        if (IsString()) {
            return std::get<std::string>(*this);
        }
        throw logic_error("");
    }

    const Array& Node::AsArray() const {
        if (IsArray()) {
            return std::get<Array>(*this);
        }
        throw logic_error("");
    }

    const Dict& Node::AsMap() const {
        if (IsMap()) {
            return std::get<Dict>(*this);
        }
        throw logic_error("");
    }

    Array& Node::AsArray() {
        if (IsArray()) {
            return std::get<Array>(*this);
        }
        throw logic_error("");
    }

    Dict& Node::AsMap() {
        if (IsMap()) {
            return std::get<Dict>(*this);
        }
        throw logic_error("");
    }

    bool Node::operator==(const Node& r) const {
        return GetValue() == r.GetValue();
    }

    inline bool operator!=(const Node& lhs, const Node& rhs) {
        return !(lhs == rhs);
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    bool Document::operator==(const Document& r) const { return this->GetRoot() == r.GetRoot(); }

    inline bool operator!=(const Document& lhs, const Document& rhs) {
        return !(lhs == rhs);
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        auto r_node = doc.GetRoot();
        PrintNode(r_node, output);
    }

    void PrintNode(const Node& node, std::ostream& out) {
        std::visit(PrintValueSt{ out, 4 }, node.GetValue());
    }

}  // namespace json
