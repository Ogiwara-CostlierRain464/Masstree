#ifndef MASSTREE_ALLOC_H
#define MASSTREE_ALLOC_H

#include <cstddef>
#include <iostream>

extern int inc_border_node;
extern int dec_border_node;
extern int inc_interior_node;
extern int dec_interior_node;
extern int inc_big_suffix;
extern int dec_big_suffix;

class Alloc{
private:

public:
  static void incBorder(){
    ++inc_border_node;
  }

  static void decBorder(){
    ++dec_border_node;
  }

  static void incInterior(){
    ++inc_interior_node;
  }

  static void decInterior(){
    ++dec_interior_node;
  }

  static void incSuffix(){
    ++inc_big_suffix;
  }

  static void decSuffix(){
    ++dec_big_suffix;
  }

  static void print(){
    std::cout << "BorderInc: " << inc_border_node << std::endl;
    std::cout << "BorderDec: " << dec_border_node << std::endl;
    std::cout << "Border±: " << inc_border_node - dec_border_node << std::endl;
    std::cout << "Interior±: " << inc_interior_node - dec_interior_node << std::endl;
    std::cout << "Suffix±: " << inc_big_suffix - dec_big_suffix << std::endl;
  }
};




#endif //MASSTREE_ALLOC_H
