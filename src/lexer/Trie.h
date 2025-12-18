#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <unordered_map>

class Trie {
 public:
  struct Node {
    bool is_terminal = false;
    int keyword_id = -1;
    std::unordered_map<char, Node*> next;
  };

  Trie();
  ~Trie();

  void Insert(const std::string& word, int keyword_id) const;
  [[nodiscard]] int Match(const std::string& word) const;

 private:
  Node* root;

  static void FreeNode(const Node* node);
};

#endif
