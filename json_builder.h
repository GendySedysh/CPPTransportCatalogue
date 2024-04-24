#pragma once

#include "json.h"

namespace json {

	class Builder {
		class BuilderContext;
		class DictValueContext;
		class StartDictContext;
		class StartArrayContext;

	public:

		Builder() = default;
		~Builder() = default;

		Node Build();
		Builder& Value(Node::Value val);
		Builder& Key(std::string key);

		StartArrayContext StartArray();
		Builder& EndArray();

		StartDictContext StartDict();
		Builder& EndDict();

	private:
		Node root_;
		std::vector<Node*> nodes_stack_{};
		std::vector<std::string> keys_{};

		class BaseContext {
		public:
			BaseContext(Builder& builder)
				: builder_(builder) {}

			Node Build() { return builder_.Build(); }
			Builder& Value(Node::Value val) {
				return builder_.Value(val); 
			}

			DictValueContext Key(std::string key) {
				return BaseContext(builder_.Key(key));
			}

			StartArrayContext StartArray() { return builder_.StartArray(); }
			Builder& EndArray() { return builder_.EndArray(); }
			StartDictContext StartDict() { return builder_.StartDict(); }
			Builder& EndDict() { return builder_.EndDict(); }

		private:
			Builder& builder_;
		};

		// KEY
		class DictValueContext : public BaseContext {
		private:
			class KeyVal : public BaseContext
			{
			public:
				KeyVal(BaseContext base)
					: BaseContext(base) {}

				Node Build() = delete;
				Builder& Value(Node::Value val) = delete;
				StartArrayContext StartArray() = delete;
				Builder& EndArray() = delete;
				StartDictContext StartDict() = delete;
			};

		public:
			DictValueContext(BaseContext base)
				: BaseContext(base) {}

			Node Build() = delete;
			DictValueContext Key(std::string key) = delete;
			Builder& EndArray() = delete;
			Builder& EndDict() = delete;
			KeyVal Value(Node::Value val) {
				return BaseContext(BaseContext::Value(std::move(val)));
			}

		};

		// START_DICT
		class StartDictContext : public BaseContext {
		public:
			StartDictContext(BaseContext base)
				: BaseContext(base) {}

			Node Build() = delete;
			Builder& Value(Node::Value val) = delete;
			Builder& StartArray() = delete;
			Builder& EndArray() = delete;
			StartDictContext StartDict() = delete;
		};

		// START_ARRAY
		class StartArrayContext : public BaseContext {
		private:
			class KeyVal : public BaseContext
			{
			public:
				KeyVal(BaseContext base)
					: BaseContext(base) {}

				Node Build() = delete;
				Builder& Key(std::string key) = delete;
				Builder& EndDict() = delete;
				KeyVal Value(Node::Value val) {
					return BaseContext(BaseContext::Value(std::move(val)));
				}
			};
		public:
			StartArrayContext(BaseContext base)
				: BaseContext(base) {}

			Node Build() = delete;
			DictValueContext Key(std::string key) = delete;
			Builder& EndDict() = delete;
			KeyVal Value(Node::Value val) {
				return BaseContext(BaseContext::Value(std::move(val)));
			}
		};

	};
}
