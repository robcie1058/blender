#pragma once

#include "FN_multi_function.h"
#include "BLI_resource_collector.h"
#include "BLI_string_map.h"
#include "BLI_string_multi_map.h"

namespace FN {
namespace Expr {

using BLI::ResourceCollector;
using BLI::StringMap;
using BLI::StringMultiMap;

class FunctionTable {
 private:
  StringMultiMap<const MultiFunction *> m_table;

 public:
  void add(StringRef name, const MultiFunction &fn)
  {
    m_table.add(name, &fn);
  }

  ArrayRef<const MultiFunction *> lookup(StringRef name) const
  {
    return m_table.lookup_default(name);
  }
};

struct SingleConstant {
  const CPPType *type;
  void *buffer;
};

class ConstantsTable {
 private:
  LinearAllocator<> m_allocator;
  StringMap<SingleConstant> m_table;

 public:
  ConstantsTable() = default;
  ~ConstantsTable()
  {
    m_table.foreach_value(
        [&](SingleConstant &constant) { constant.type->destruct(constant.buffer); });
  }

  void add_single(StringRef name, const CPPType &type, const void *buffer)
  {
    void *own_buffer = m_allocator.allocate(type.size(), type.alignment());
    type.copy_to_uninitialized(buffer, own_buffer);
    m_table.add_new(name, {&type, own_buffer});
  }

  template<typename T> void add_single(StringRef name, const T &value)
  {
    this->add_single(name, CPP_TYPE<T>(), (const void *)&value);
  }

  Optional<SingleConstant> try_lookup(StringRef name) const
  {
    return m_table.try_lookup(name);
  }
};

const MultiFunction &expression_to_multi_function(StringRef str,
                                                  ResourceCollector &resources,
                                                  ArrayRef<StringRef> variable_names,
                                                  ArrayRef<MFDataType> variable_types,
                                                  const ConstantsTable &constants_table);

}  // namespace Expr
}  // namespace FN
