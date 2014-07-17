#include <queue>
#include <iostream>
#include "rbtree.h"

using namespace std;

bool RBtree::is_leaf(const RBnode* node) const {
  return node == RBtree::sentinel_;
}

RBtree::RBtree() : root_{nullptr} {
  sentinel_ = new RBnode{nullptr, nullptr, 0};
  sentinel_->color = Color::BLACK;
}

RBtree::RBnode::RBnode(RBnode* sentinel, RBnode* parent, uint64_t hole) : left_key{hole}, right_key{hole},
  left{sentinel}, right{sentinel}, sum{0}, parent{parent},
  color{Color::RED} { }

void RBtree::rotate_left(RBnode* node) {
  RBnode* new_top = node->right;
  node->right = new_top->left;
  new_top->left->parent = node;
  new_top->parent = node->parent;
  if(new_top->parent == nullptr){
    root_ = new_top;
  } else if(node == node->parent->left){
    node->parent->left = new_top;
  } else {
    node->parent->right = new_top;
  }
  new_top->left = node;
  node->parent = new_top;
  node->sum -= new_top->sum + 
               new_top->right_key - new_top->left_key + 1;
}

void RBtree::rotate_right(RBnode* node) {
  RBnode* new_top = node->left;
  node->left = new_top->right;
  new_top->right->parent = node;
  new_top->parent = node->parent;
  if(new_top->parent == nullptr){
    root_ = new_top;
  } else if(node == node->parent->right){
    node->parent->right = new_top;
  } else {
    node->parent->left = new_top;
  }
  new_top->right = node;
  node->parent = new_top;
  new_top->sum += node->sum + 
                  node->right_key - node->left_key + 1;
}

void RBtree::add_rebalance(RBnode* node){
  // case 1
  if(node->parent == nullptr){
    node->color = Color::BLACK;
    return;
  }

  // case 2
  if(node->parent->color == Color::BLACK){
    return;
  }

  // case 3
  RBnode *uncle = nullptr, *grandparent = nullptr;
  if(node->parent != nullptr){
    grandparent = node->parent->parent;
  }
  if(grandparent != nullptr){
    if(node->parent == grandparent->left){
      uncle = grandparent->right;
    } else {
      uncle = grandparent->left;
    }
  }

  if(uncle != nullptr &&
     uncle->color == Color::RED){
    node->parent->color = Color::BLACK;
    uncle->color = Color::BLACK;
    grandparent->color = Color::RED;
    add_rebalance(grandparent);
    return;
  }

  // case 4
  RBnode* active = node;
  if(node == node->parent->right &&
     node->parent == grandparent->left){
    rotate_left(node->parent);
    active = node->left;
  } else if(node == node->parent->left &&
            node->parent == grandparent->right){
    rotate_right(node->parent);
    active = node->right;
  }

  // case 5
  RBnode* active_grandparent = nullptr;
  if(active->parent != nullptr){
    active_grandparent = active->parent->parent;
  }
  active->parent->color = Color::BLACK;
  active_grandparent->color = Color::RED;
  if(active == active->parent->left){
    rotate_right(active_grandparent);
  } else {
    rotate_left(active_grandparent);
  }
}

void RBtree::remove(RBnode* node){
  RBnode* child = nullptr;
  if(!is_leaf(node->left)){
    child = node->left;
  } else {
    child = node->right;
  }

  if(!is_leaf(child)){
    child->parent = node->parent;
  }

  bool is_left = is_left_child(node);
  if(is_left){
    node->parent->left = child;
  } else {
    node->parent->right = child;
    //node->parent->sum -= node->right_key - node->left_key + 1;
  }

  if(node->color == Color::BLACK){
    if(child->color == Color::RED){
      child->color = Color::BLACK;
    } else {
      delete_fixup(node, is_left);
    }
  }
  return;
}

bool RBtree::is_left_child(RBnode* node){
  bool res = false;
  if(node->parent &&
     node == node->parent->left){
    res = true;
  }
  return res;
}

void RBtree::delete_fixup(RBnode* node, bool is_left){
  //case 1
  if(node->parent == nullptr){
    return;
  }

  // case 2
  RBnode* sibling = is_left ? node->parent->right : node->parent->left;
  if(sibling->color == Color::RED){
    node->parent->color = Color::RED;
    sibling->color = Color::BLACK;
    if(is_left){
      rotate_left(node->parent);
      sibling = node->parent->right;
    } else {
      rotate_right(node->parent);
      sibling = node->parent->left;
    }
  }
  
  // case 3
  if(node->parent->color == Color::BLACK &&
     sibling->color == Color::BLACK &&
     sibling->left->color == Color::BLACK &&
     sibling->right->color == Color::BLACK){
    sibling->color = Color::RED;
    delete_fixup(node->parent, is_left_child(node->parent));
    return;
  }

  // case 4
  if(node->parent->color == Color::RED &&
     sibling->color == Color::BLACK &&
     sibling->left->color == Color::BLACK &&
     sibling->right->color == Color::BLACK){
    sibling->color = Color::RED;
    node->parent->color = Color::BLACK;
    return;
  }

  // case 5
  if(sibling->color == Color::BLACK){
    if(is_left &&
       sibling->right->color == Color::BLACK &&
       sibling->left->color == Color::RED){
      sibling->color = Color::RED;
      sibling->left->color = Color::BLACK;
      rotate_right(sibling);
      sibling = node->parent->right;
    } else if(!is_left &&
              sibling->left->color == Color::BLACK &&
              sibling->right->color == Color::RED){
      sibling->color = Color::RED;
      sibling->right->color = Color::BLACK;
      rotate_left(sibling);
      sibling = node->parent->left;
    }
  }

  // case 6
  sibling->color = node->parent->color;
  node->parent->color = Color::BLACK;
  if(is_left){
    sibling->right->color = Color::BLACK;
    rotate_left(node->parent);
  } else {
    sibling->left->color = Color::BLACK;
    rotate_right(node->parent);
  }
}

uint64_t RBtree::distance(uint64_t hole){
  if(root_ == nullptr){
    root_ = new RBnode(sentinel_, nullptr, hole);
    root_->color = Color::BLACK;
    return 0;
  }
  return get_distance(root_, hole);
}

uint64_t RBtree::get_distance(RBnode* node, uint64_t hole){
  if(hole < node->left_key - 1 &&
     node->left_key != 0){
    if(!is_leaf(node->left)){
      return (node->right_key - node->left_key + 1) + 
             node->sum + 
             get_distance(node->left, hole);
    } else {
      node->left = new RBnode(sentinel_, node, hole);
      add_rebalance(node->left);
      return (node->right_key - node->left_key + 1) + 
             node->sum;
    }
  } 
  
  if(hole > node->right_key + 1){
    ++node->sum;
    if(!is_leaf(node->right)){
      return get_distance(node->right, hole);
    } else {
      node->right = new RBnode(sentinel_, node, hole);
      add_rebalance(node->right);
      return 0;
    }
  } 
  
  if(hole == node->left_key - 1){
    if(!is_leaf(node->left)){
      RBnode* pred = right_most_child(node->left);
      if(hole == pred->right_key + 1){
        node->left_key = pred->left_key;
        //pred->parent->sum -= pred->right_key - pred->left_key + 1;
        RBnode* n = pred;
        auto pred_val = pred->right_key - pred->left_key + 1;
        while(n != node){
          //lazyness
          if(!is_left_child(n)){
            n->parent->sum -= pred_val;
          }
          n = n->parent;
        }
        auto sum = node->right_key - hole + node->sum;
        remove(pred);
        //return node->right_key - hole + node->sum;
        return sum;
      }
    }
    node->left_key = hole;
    return node->right_key - hole + node->sum;
  } 
  
  if(hole == node->right_key + 1){
    if(!is_leaf(node->right)){
      RBnode* successor = left_most_child(node->right);
      if(hole == successor->left_key - 1){
        node->right_key = successor->right_key;
        RBnode* n = successor;
        auto succ_val = successor->right_key - successor->left_key + 1;
        while(n != node){
          // lazy. if right child
          if(!is_left_child(n)){
            n->parent->sum -= succ_val;
          }
          n = n->parent;
        }
        remove(successor);
        return node->sum + node->right_key - hole;
      }
    }
    node->right_key = hole;
    return node->sum;
  }

  // shouldn't ever get here
  return 0;
}

RBtree::RBnode* RBtree::right_most_child(RBnode* node){
  if(!is_leaf(node->right)){
    return right_most_child(node->right);
  } else {
    return node;
  }
}

RBtree::RBnode* RBtree::left_most_child(RBnode* node){
  if(!is_leaf(node->left)){
    return left_most_child(node->left);
  } else {
    return node;
  }
}

void RBtree::print_helper(const RBnode* node, ostream& os) const{
  os << "\""
     << "(" << node->left_key << "," << node->right_key << ")" 
     << "(" << (node->color == RBtree::Color::BLACK ? "B" : "R") << ")"
     << "(" << node->sum << ")"
     << "\"";
}

void RBtree::print_me(const RBnode* node, ostream& os) const{
  if(!node) return;
  if(!is_leaf(node->left)){
    os << "    ";
    print_helper(node, os);
    os << " -> ";
    print_helper(node->left, os);
    os << ";" << endl;
    print_me(node->left, os);
  }
  if(!is_leaf(node->right)){
    os << "    ";
    print_helper(node, os);
    os << " -> ";
    print_helper(node->right, os);
    os << ";" << endl;
    print_me(node->right, os);
  }
}

ostream& operator <<(ostream& os, const RBtree& tree){
  if(!tree.is_valid()){
    os << "ERROR: Invalid tree" << endl;
  }
  os << "digraph curtree {" << endl;
  tree.print_me(tree.root_, os);
  os << "}";
  return os;
}

bool RBtree::is_valid() const {
  if(!root_) return true;

  if(root_->color != Color::BLACK){
    cerr << "Error: root not black" << endl;
    return false;
  }

  int max_depth = 0;
  RBnode* cur = root_;
  while(cur){
    if(cur->color == Color::BLACK){
      ++max_depth;
    }
    cur = cur->left;
  }

  if(!check_depth(root_, 0, max_depth)){
    cerr << "Incorrect depth" << endl;
    return false;
  }
  if(!check_sum(root_)){
    cerr << "Incorrect sums" << endl;
    return false;
  }
  return true;
}

uint64_t RBtree::sum_of_holes(const RBnode* node) const {
  if(is_leaf(node)){
    return 0;
  }
  return node->sum + 
         (node->right_key - node->left_key + 1) + 
         sum_of_holes(node->left);
}

bool RBtree::check_sum(const RBnode* node) const {
  bool valid = true;
  if(!node) return valid;
  valid &= node->sum == sum_of_holes(node->right);
  if(!is_leaf(node->left)){
    valid &= check_sum(node->left);
  }
  if(!is_leaf(node->right)){
    valid &= check_sum(node->right);
  }
  return valid;
}

  /* for each node
   *  if red, make sure both children are black
   *  if black, increment current depth counter
   *  if leaf, make sure current depth == max depth
   *  otherwise, continue search along both children
   */
bool RBtree::check_depth(const RBnode* node, int cur_depth, int max_depth) const {
  if(node->color == Color::RED &&
     (node->left->color == Color::RED ||
      node->right->color == Color::RED)){
    cerr << "Error: red node with red child" << endl;
    return false;
  }
  if(node->color == Color::BLACK){
    ++cur_depth;
  }
  if(is_leaf(node)){
    if(cur_depth != max_depth){
      cerr << "Error: leaf depth " << cur_depth << " shallower than max depth " << max_depth << endl;
    }
    return cur_depth == max_depth;
  }

  return check_depth(node->left, cur_depth, max_depth) && 
         check_depth(node->right, cur_depth, max_depth);
}

