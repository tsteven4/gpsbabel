#ifndef STATICLIST_H
#define STATICLIST_H

/*
 * A container
 * 1) with a fixed number of elements
 * 2) with elements that are modifiable
 * 3) that is movable
 * 4) that is not copyable
 * 5) where each member contains it's own index
 *    into then list
 * 6) with iterators that are never invalidated
 *    by use of operator[]
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

  // Expose limited methods for portability.
  // public types
  using typename std::vector<T>::value_type;
  using typename std::vector<T>::allocator_type;
  using typename std::vector<T>::size_type;
  using typename std::vector<T>::difference_type;
  using typename std::vector<T>::reference;
  using typename std::vector<T>::const_reference;
  using typename std::vector<T>::pointer;
  using typename std::vector<T>::const_pointer;
  using typename std::vector<T>::iterator;
  using typename std::vector<T>::const_iterator;
  using typename std::vector<T>::reverse_iterator;
  using typename std::vector<T>::const_reverse_iterator;
  // public functions
  using std::vector<T>::back;
  using std::vector<T>::begin;
  using std::vector<T>::cbegin;
  using std::vector<T>::cend;
  using std::vector<T>::crbegin;
  using std::vector<T>::crend;
  using std::vector<T>::end;
  using std::vector<T>::empty;
  using std::vector<T>::front;
  using std::vector<T>::rbegin;
  using std::vector<T>::rend;
  using std::vector<T>::size;
  using std::vector<T>::operator[];
};
#endif // STATICLIST_H
