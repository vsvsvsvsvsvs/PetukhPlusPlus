#include "Trie.h"

#include <ranges>

Trie::Trie() { root = new Node(); }

Trie::~Trie() { FreeNode(root); }

void Trie::FreeNode(const Node* node) {
  for (const auto& val : node->next | std::views::values) {
    FreeNode(val);
  }
  delete node;
}

void Trie::Insert(const std::string& word, const int keyword_id) const {
  Node* cur = root;
  for (char c : word) {
    if (!cur->next.contains(c)) {
      cur->next[c] = new Node();
    }
    cur = cur->next[c];
  }
  cur->is_terminal = true;
  cur->keyword_id = keyword_id;
}

int Trie::Match(const std::string& word) const {
  const Node* cur = root;
  for (char c : word) {
    if (!cur->next.contains(c)) {
      return -1;
    }
    cur = cur->next.at(c);
  }
  return cur->is_terminal ? cur->keyword_id : -1;
}
