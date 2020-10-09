#ifndef MASSTREE_VALUE_H
#define MASSTREE_VALUE_H

namespace masstree{

class Value{
public:

  Value(int body_)
    : body(body_){}

  bool operator==(const Value &rhs)const{
    return body == rhs.body;
  }

  bool operator!=(const Value &rhs)const{
    return !(*this == rhs);
  }

  inline int getBody() const{
    return body;
  }

private:
  int body;
};

}

#endif //MASSTREE_VALUE_H
