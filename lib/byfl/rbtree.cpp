#include <queue>
#include "rbtree.h"

using namespace std;

bool RBtree::is_leaf(RBnode* node) const {
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

  if(node->parent->left == node){
    node->parent->left = child;
  } else {
    node->parent->right = child;
  }

  if(node->color == Color::BLACK){
    if(child->color == Color::RED){
      child->color = Color::BLACK;
    } else {
      delete_fixup(node);
    }
  }
  return;
}

void RBtree::delete_fixup(RBnode* node){
  //case 1
  if(node->parent == nullptr){
    return;
  }
  RBnode* sibling;
  if(node == node->parent->left){
    sibling = node->parent->right;
  } else {
    sibling = node->parent->left;
  }

  // case 2
  if(sibling->color == Color::RED){
    node->parent->color = Color::RED;
    sibling->color = Color::BLACK;
    if(node == node->parent->left){
      rotate_left(node->parent);
    } else {
      rotate_right(node->parent);
    }
  }
  
  // case 3
  if(node->parent->color == Color::BLACK &&
     sibling->color == Color::BLACK &&
     sibling->left->color == Color::BLACK &&
     sibling->right->color == Color::BLACK){
    sibling->color = Color::RED;
    delete_fixup(node->parent);
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
    if(node == node->parent->left &&
       sibling->right->color == Color::BLACK &&
       sibling->left->color == Color::RED){
      sibling->color = Color::RED;
      sibling->left->color = Color::BLACK;
      rotate_right(sibling);
    } else if(node == node->parent->right &&
              sibling->left->color == Color::BLACK &&
              sibling->right->color == Color::RED){
      sibling->color = Color::RED;
      sibling->right->color = Color::BLACK;
      rotate_left(sibling);
    }
    return;
  }

  // case 6
  sibling->color = node->parent->color;
  node->parent->color = Color::BLACK;
  if(node == node->parent->left){
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
  if(hole < node->left_key - 1){
    if(!is_leaf(node->left)){
      return node->sum + get_distance(node->left, hole);
    } else {
      node->left = new RBnode(sentinel_, node, hole);
      add_rebalance(node->left);
      return node->sum;
    }
  } 
  
  if(hole > node->right_key + 1){
    if(!is_leaf(node->right)){
      ++node->sum;
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
        remove(pred);
        return node->right_key - hole + node->sum;
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
        remove(successor);
        return node->sum;
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

ostream& operator <<(ostream& os, const RBtree& tree){
  typedef pair<const RBtree::RBnode*, int> node_level_t;
  queue<node_level_t> nodes;
  nodes.push(node_level_t(tree.root_, 0));
  int level = -1;
  while(!nodes.empty()){
    auto cur = nodes.front();
    nodes.pop();
    if(cur.second != level){
      os << endl << "Level " << cur.second << endl;
      level = cur.second;
    }
    os << "(" << cur.first->left_key << "," << cur.first->right_key << ")(" 
       << (cur.first->color == RBtree::Color::BLACK ? "B)\t" : "R)\t");
    if(!tree.is_leaf(cur.first->left)){
      nodes.push(node_level_t(cur.first->left, level + 1));
    }
    if(!tree.is_leaf(cur.first->right)){
      nodes.push(node_level_t(cur.first->right, level + 1));
    }
  }
  return os;
}

