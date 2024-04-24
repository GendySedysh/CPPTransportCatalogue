#include "json_builder.h"

namespace json {

	// ------------- Builder -------------------
	Node Builder::Build() {
		if (nodes_stack_.size() != 1) {
			throw std::logic_error("");
		}

		root_ = *nodes_stack_.back();
		return root_;
	}

	Builder& Builder::Value(Node::Value val) {
		if (nodes_stack_.size() != 0) {
			if (nodes_stack_.back()->IsArray()) {
				nodes_stack_.back()->AsArray().emplace_back(Node{ val });
			}
			else if (nodes_stack_.back()->IsMap()) {
				nodes_stack_.back()->AsMap().emplace(keys_.back(), Node{ val });
				keys_.erase(keys_.end() - 1);
			}
			else {
				nodes_stack_.emplace_back(new Node{ val });
			}
		}
		else {
			nodes_stack_.emplace_back(new Node{ val });
		}

		return *this;
	}

	Builder::StartArrayContext Builder::StartArray() {
		nodes_stack_.emplace_back(new Node{ Array{} });
		return StartArrayContext(*this);
	}

	Builder& Builder::EndArray() {
		if (!nodes_stack_.back()->IsArray()) {
			throw std::logic_error("");
		}

		if (nodes_stack_.size() > 1) {
			auto tmp = (nodes_stack_.end() - 2);
			auto node = *tmp;

			if (node->IsArray()) {
				node->AsArray().push_back(std::move(*nodes_stack_.back()));
			}
			else if (node->IsMap()) {
				node->AsMap().insert({ keys_.back(), std::move(*nodes_stack_.back()) });
				keys_.erase(keys_.end() - 1);
			}
			nodes_stack_.erase(nodes_stack_.end() - 1);
		}

		return *this;
	}

	Builder::StartDictContext Builder::StartDict() {
		nodes_stack_.emplace_back(new Node{ Dict{} });
		return StartDictContext(*this);
	}

	Builder& Builder::Key(std::string key) {
		keys_.push_back(key);

		return *this;
	}

	Builder& Builder::EndDict() {

		if (!nodes_stack_.back()->IsMap()) {
			throw std::logic_error("");
		}

		if (nodes_stack_.size() > 1) {
			auto tmp = (nodes_stack_.end() - 2);
			auto node = *tmp;

			if (node->IsArray()) {
				node->AsArray().push_back(std::move(*nodes_stack_.back()));
			}
			else if (node->IsMap()) {
				node->AsMap().insert({ keys_.back(), std::move(*nodes_stack_.back()) });
				keys_.erase(keys_.end() - 1);
			}
			nodes_stack_.erase(nodes_stack_.end() - 1);
		}
		return *this;
	}

}