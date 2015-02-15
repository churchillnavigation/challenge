// 
// SortedVector.h
// 01/10/2015
//
// Drew Hall
// andrew.danger.hall@gmail.com
//
// A SortedVector class
// 
#include <vector>
#include <algorithm>
#include <iterator>


template <typename El, typename Pred = bool (*)(const El&, const El&)>
class SortedVector
{
  public:
    typedef El element_type;
    typedef El& reference;
    typedef const El& const_reference;
 
  private:
    typedef std::vector<element_type> vec_type;

  public:
    typedef typename vec_type::size_type size_type;
    typedef typename vec_type::iterator iterator;
    typedef typename vec_type::const_iterator const_iterator;

  public:
    SortedVector();
    SortedVector(Pred pred);

    template <class InputIterator>
    SortedVector(InputIterator b, InputIterator e);

    size_type size() const;
    bool empty() const;

    void insert(const_reference val);
    void erase(const_reference val);
    void clear();
    void resize(size_type new_size);
    
    bool contains(const_reference val) const;

    size_type merge(const SortedVector<El, Pred>& lhs, const SortedVector<El, Pred>& rhs, size_type maxSize);

    iterator begin();
    const_iterator begin() const;

    iterator end();
    const_iterator end() const;

  private:
    std::vector<element_type> mData;
    Pred mPredicate;
};



//**************************************************************************************
//* Inline implementations:
//**************************************************************************************

template <typename El, typename Pred>
SortedVector<El, Pred>::SortedVector()
{
}


template <typename El, typename Pred>
SortedVector<El, Pred>::SortedVector(Pred pred)
  : mPredicate(pred)
{
}


template <typename El, typename Pred>
  template <class InputIterator>
SortedVector<El, Pred>::SortedVector(InputIterator b, InputIterator e)
{
  std::copy(begin, end, std::back_inserter(mData));
  std::sort(begin(), end(), mPredicate);
}


template <typename El, typename Pred>
inline typename SortedVector<El, Pred>::size_type SortedVector<El, Pred>::size() const
{
  return mData.size();
}


template <typename El, typename Pred>
inline bool SortedVector<El, Pred>::empty() const
{
  return mData.empty();
}


template <typename El, typename Pred>
void SortedVector<El, Pred>::insert(const_reference val)
{
  // Insert in the proper sorted order.  This should involve
  // a single memmove() operation if El is a POD type.
  mData.insert(std::upper_bound(begin(), end(), val, mPredicate), val);
}


template <typename El, typename Pred>
void SortedVector<El, Pred>::erase(const_reference val)
{
  mData.erase(std::lower_bound(begin(), end(), val, mPredicate));
}


template <typename El, typename Pred>
void SortedVector<El, Pred>::clear()
{
  mData.clear();
}


template <typename El, typename Pred>
void SortedVector<El, Pred>::resize(typename SortedVector<El, Pred>::size_type new_size)
{
  const size_type old_size = size();
  mData.resize(new_size);

  if (new_size > old_size) {
    // Ensure that we remain sorted:
    std::sort(begin(), end(), mPredicate);
  }
}


template <typename El, typename Pred>
bool SortedVector<El, Pred>::contains(const_reference val) const
{
  return std::binary_search(begin(), end(), val, mPredicate);
}


template <typename El, typename Pred>
typename SortedVector<El, Pred>::size_type
SortedVector<El, Pred>::merge(const SortedVector<El, Pred>& lhs,
                              const SortedVector<El, Pred>& rhs,
                              typename SortedVector<El, Pred>::size_type maxSize)
{
  mData.clear();

  const_iterator lit = lhs.begin();
  const const_iterator lend = lhs.end();

  const_iterator rit = rhs.begin();
  const const_iterator rend = rhs.end();

  const size_type totalElements =
    std::distance(lit, lend) + std::distance(rit, rend);

  const size_type n = std::min(totalElements, maxSize);

  size_type i = 0;
  while (i < n) {
    if (lit == lend) {
      mData.push_back(*rit++);
    }
    else if (rit == rend) {
      mData.push_back(*lit++);
    }
    else {
      if (mPredicate(*lit, *rit)) {
        mData.push_back(*lit++);
      }
      else {
        mData.push_back(*rit++);
      }
    }
    ++i;
  }

  return i;
}



template <typename El, typename Pred>
inline typename SortedVector<El, Pred>::iterator SortedVector<El, Pred>::begin()
{
  return mData.begin();
}


template <typename El, typename Pred>
inline typename SortedVector<El, Pred>::const_iterator SortedVector<El, Pred>::begin() const
{
  return mData.begin();
}


template <typename El, typename Pred>
inline typename SortedVector<El, Pred>::iterator SortedVector<El, Pred>::end()
{
  return mData.end();
}


template <typename El, typename Pred>
inline typename SortedVector<El, Pred>::const_iterator SortedVector<El, Pred>::end() const
{
  return mData.end();
}
