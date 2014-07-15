#ifndef RBTREE_H
#define RBTREE_H

#include <cstdint>
#include <ostream>

class RBtree {
  public:
    uint64_t distance(uint64_t hole);
    RBtree();
    friend std::ostream& operator <<(std::ostream& os, const RBtree& node);

  private:
    enum class Color {BLACK, RED};
    struct RBnode {
      RBnode(RBnode* sentinel, RBnode* parent, uint64_t hole);

      uint64_t left_key, right_key;
      RBnode *left, *right;
      uint64_t sum;
      RBnode* parent;
      Color color;
    };

    // rotation may change root, if root is parent
    void rotate_left(RBnode* node);
    void rotate_right(RBnode* node);
    void remove(RBnode* node);
    void delete_fixup(RBnode* node);
    void add_rebalance(RBnode* node);
    bool is_leaf(RBnode* node) const;
    uint64_t get_distance(RBnode* node, uint64_t hole);
    RBnode* right_most_child(RBnode* node);
    RBnode* left_most_child(RBnode* node);

    RBnode* root_;
    RBnode* sentinel_;
};
#endif // RBTREE_H
