/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __BLI_MEMORY_UTILS_HH__
#define __BLI_MEMORY_UTILS_HH__

/** \file
 * \ingroup bli
 */

#include <algorithm>
#include <memory>

#include "BLI_utildefines.h"

namespace BLI {

template<typename T> void copy_n(const T *src, uint n, T *dst)
{
  for (uint i = 0; i < n; i++) {
    dst[i] = src[i];
  }
}

template<typename T> void uninitialized_copy_n(const T *src, uint n, T *dst)
{
  for (uint i = 0; i < n; i++) {
    new (dst + i) T(src[i]);
  }
}

template<typename T> void uninitialized_fill_n(T *dst, uint n, const T &value)
{
  for (uint i = 0; i < n; i++) {
    new (dst + i) T(value);
  }
}

template<typename T> void destruct_n(T *ptr, uint n)
{
  for (uint i = 0; i < n; i++) {
    ptr[i].~T();
  }
}

template<typename T> void uninitialized_move_n(T *src, uint n, T *dst)
{
  std::uninitialized_copy_n(std::make_move_iterator(src), n, dst);
}

template<typename T> void move_n(T *src, uint n, T *dst)
{
  std::copy_n(std::make_move_iterator(src), n, dst);
}

template<typename T> void uninitialized_relocate_n(T *src, uint n, T *dst)
{
  uninitialized_move_n(src, n, dst);
  destruct_n(src, n);
}

template<typename T> void relocate_n(T *src, uint n, T *dst)
{
  move_n(src, n, dst);
  destruct_n(src, n);
}

template<typename T, typename... Args> std::unique_ptr<T> make_unique(Args &&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T> struct DestructValueAtAddress {
  void operator()(T *ptr)
  {
    ptr->~T();
  }
};

template<typename T> using destruct_ptr = std::unique_ptr<T, DestructValueAtAddress<T>>;

template<size_t Size, size_t Alignment> class alignas(Alignment) AlignedBuffer {
 private:
  /* Don't create an empty array. This causes problems with some compilers. */
  char m_buffer[(Size > 0) ? Size : 1];

 public:
  void *ptr()
  {
    return (void *)m_buffer;
  }

  const void *ptr() const
  {
    return (const void *)m_buffer;
  }
};

}  // namespace BLI

#endif /* __BLI_MEMORY_UTILS_HH__ */
