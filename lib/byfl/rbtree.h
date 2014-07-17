#ifndef RBTREE_H
#define RBTREE_H

#include <cstdint>
#include <ostream>

class RBtree {
  public:
    uint64_t distance(uint64_t hole);
    RBtree();
    friend std::ostream& operator <<(std::ostream& os, const RBtree& node);
    bool is_valid() const;

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
    void delete_fixup(RBnode* node, bool is_left);
    void add_rebalance(RBnode* node);
    bool is_leaf(const RBnode* node) const;
    uint64_t get_distance(RBnode* node, uint64_t hole);
    RBnode* right_most_child(RBnode* node);
    RBnode* left_most_child(RBnode* node);
    bool is_left_child(RBnode* node);
    bool check_depth(const RBnode* node, int cur_depth, int max_depth) const;
    bool check_sum(const RBnode* node) const;
    uint64_t sum_of_holes(const RBnode* node) const;
    void print_me(const RBnode* node, std::ostream& os) const;
    void print_helper(const RBnode* node, std::ostream& os) const;

    RBnode* root_;
    RBnode* sentinel_;
};
#endif // RBTREE_H
