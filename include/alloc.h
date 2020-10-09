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
extern int dec_value;

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

  static void decValue(){
  ++dec_value;
}

  static void print(){
    std::cout << "BorderInc: " << inc_border_node << std::endl;
    std::cout << "Interior: " << inc_interior_node << std::endl;
    std::cout << "Border±: " << inc_border_node - dec_border_node << std::endl;
    std::cout << "Interior±: " << inc_interior_node - dec_interior_node << std::endl;
    std::cout << "Suffix±: " << inc_big_suffix - dec_big_suffix << std::endl;
    std::cout << "ValueDec: " << dec_value << std::endl;


  }

  static void reset(){
    inc_border_node = 0;
    dec_border_node = 0;
    inc_interior_node = 0;
    dec_interior_node = 0;
    inc_big_suffix = 0;
    dec_big_suffix = 0;
    dec_value = 0;
  }
};




#endif //MASSTREE_ALLOC_H
