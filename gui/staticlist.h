#ifndef STATICLIST_H
#define STATICLIST_H

/* 
 * A container
 * 1) with a fixed number of elements
 * 2) with elements that are modifiable
 * 3) that is movable
 * 4) that is not copyable
 * 5) where each member contains it's own index
 *    into then list.
 */   

#include <vector>  // for vector

template<typename T>
class StaticList : private std::vector<T>
{
public:
  StaticList() : std::vector<T>() {};
  StaticList(const StaticList&) = delete;
  StaticList& operator=(const StaticList&) = delete;
  StaticList(StaticList&&) = default;
  StaticList& operator=(StaticList&&) = default;

  StaticList(const std::vector<T>&& qlist) : std::vector<T>(qlist) // converting ctor
  {
    int idx = 0;
    for (auto& element : *this) {
      element.idx_ = idx++;
    }
  }

  // public types
  using typename std::vector<T>::const_iterator;
  using typename std::vector<T>::const_reverse_iterator;
  using typename std::vector<T>::iterator;
  using typename std::vector<T>::reverse_iterator;
  // public functions
  using std::vector<T>::at;
  using std::vector<T>::back;
  using std::vector<T>::begin;
  using std::vector<T>::cbegin;
  using std::vector<T>::cend;
  using std::vector<T>::end;
  using std::vector<T>::front;
  //using std::vector<T>::isEmpty;
  using std::vector<T>::size;
  using std::vector<T>::operator[];
};
#endif // STATICLIST_H
