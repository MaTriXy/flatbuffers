/*
 * Copyright 2015 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLATBUFFERS_REFLECTION_H_
#define FLATBUFFERS_REFLECTION_H_

#include "flatbuffers/util.h"

// This is somewhat of a circular dependency because flatc (and thus this
// file) is needed to generate this header in the first place.
// Should normally not be a problem since it can be generated by the
// previous version of flatc whenever this code needs to change.
// See reflection/generate_code.sh
#include "flatbuffers/reflection_generated.h"

// Helper functionality for reflection.

namespace flatbuffers {

inline size_t GetTypeSize(reflection::BaseType base_type) {
  // This needs to correspond to the BaseType enum.
  static size_t sizes[] = { 0, 1, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 4, 4, 4, 4 };
  return sizes[base_type];
}

// Get the root, regardless of what type it is.
inline Table *GetAnyRoot(uint8_t *flatbuf) {
  return GetMutableRoot<Table>(flatbuf);
}
inline const Table *GetAnyRoot(const uint8_t *flatbuf) {
  return GetRoot<Table>(flatbuf);
}

// Get a field, if you know it's an integer, and its exact type.
template<typename T> T GetFieldI(const Table *table,
                                 const reflection::Field *field) {
  assert(sizeof(T) == GetTypeSize(field->type()->base_type()));
  return table->GetField<T>(field->offset(),
                            static_cast<T>(field->default_integer()));
}

// Get a field, if you know it's floating point and its exact type.
template<typename T> T GetFieldF(const Table *table,
                                 const reflection::Field *field) {
  assert(sizeof(T) == GetTypeSize(field->type()->base_type()));
  return table->GetField<T>(field->offset(),
                            static_cast<T>(field->default_real()));
}

// Get a field, if you know it's a string.
inline const String *GetFieldS(const Table *table,
                               const reflection::Field *field) {
  assert(field->type()->base_type() == reflection::String);
  return table->GetPointer<const String *>(field->offset());
}

// Get a field, if you know it's a vector.
template<typename T> const Vector<T> *GetFieldV(const Table *table,
                                               const reflection::Field *field) {
  assert(field->type()->base_type() == reflection::Vector &&
         sizeof(T) == GetTypeSize(field->type()->element()));
  return table->GetPointer<const Vector<T> *>(field->offset());
}

// Get any field as a 64bit int, regardless of what it is (bool/int/float/str).
inline int64_t GetAnyFieldI(const Table *table,
                            const reflection::Field *field) {
# define FLATBUFFERS_GET(C, T) \
    static_cast<int64_t>(GetField##C<T>(table, field))
  switch (field->type()->base_type()) {
    case reflection::UType:
    case reflection::Bool:
    case reflection::UByte:  return FLATBUFFERS_GET(I, uint8_t);
    case reflection::Byte:   return FLATBUFFERS_GET(I, int8_t);
    case reflection::Short:  return FLATBUFFERS_GET(I, int16_t);
    case reflection::UShort: return FLATBUFFERS_GET(I, uint16_t);
    case reflection::Int:    return FLATBUFFERS_GET(I, int32_t);
    case reflection::UInt:   return FLATBUFFERS_GET(I, uint32_t);
    case reflection::Long:   return FLATBUFFERS_GET(I, int64_t);
    case reflection::ULong:  return FLATBUFFERS_GET(I, uint64_t);
    case reflection::Float:  return FLATBUFFERS_GET(F, float);
    case reflection::Double: return FLATBUFFERS_GET(F, double);
    case reflection::String: return StringToInt(
                                      GetFieldS(table, field)->c_str());
    default: return 0;
  }
# undef FLATBUFFERS_GET
}

// Get any field as a double, regardless of what it is (bool/int/float/str).
inline double GetAnyFieldF(const Table *table,
                           const reflection::Field *field) {
  switch (field->type()->base_type()) {
    case reflection::Float:  return GetFieldF<float>(table, field);
    case reflection::Double: return GetFieldF<double>(table, field);
    case reflection::String: return strtod(GetFieldS(table, field)->c_str(),
                                           nullptr);
    default: return static_cast<double>(GetAnyFieldI(table, field));
  }
}

// Get any field as a string, regardless of what it is (bool/int/float/str).
inline std::string GetAnyFieldS(const Table *table,
                                const reflection::Field *field) {
  switch (field->type()->base_type()) {
    case reflection::Float:
    case reflection::Double: return NumToString(GetAnyFieldF(table, field));
    case reflection::String: return GetFieldS(table, field)->c_str();
    // TODO: could return vector/table etc as JSON string.
    default: return NumToString(GetAnyFieldI(table, field));
  }
}

// Set any scalar field, if you know its exact type.
template<typename T> bool SetField(Table *table, const reflection::Field *field,
                                   T val) {
  assert(sizeof(T) == GetTypeSize(field->type()->base_type()));
  return table->SetField(field->offset(), val);
}

// Set any field as a 64bit int, regardless of what it is (bool/int/float/str).
inline void SetAnyFieldI(Table *table, const reflection::Field *field,
                         int64_t val) {
# define FLATBUFFERS_SET(T) SetField<T>(table, field, static_cast<T>(val))
  switch (field->type()->base_type()) {
    case reflection::UType:
    case reflection::Bool:
    case reflection::UByte:  FLATBUFFERS_SET(uint8_t  ); break;
    case reflection::Byte:   FLATBUFFERS_SET(int8_t   ); break;
    case reflection::Short:  FLATBUFFERS_SET(int16_t  ); break;
    case reflection::UShort: FLATBUFFERS_SET(uint16_t ); break;
    case reflection::Int:    FLATBUFFERS_SET(int32_t  ); break;
    case reflection::UInt:   FLATBUFFERS_SET(uint32_t ); break;
    case reflection::Long:   FLATBUFFERS_SET(int64_t  ); break;
    case reflection::ULong:  FLATBUFFERS_SET(uint64_t ); break;
    case reflection::Float:  FLATBUFFERS_SET(float    ); break;
    case reflection::Double: FLATBUFFERS_SET(double   ); break;
    // TODO: support strings
    default: break;
  }
# undef FLATBUFFERS_SET
}

// Set any field as a double, regardless of what it is (bool/int/float/str).
inline void SetAnyFieldF(Table *table, const reflection::Field *field,
                         double val) {
  switch (field->type()->base_type()) {
    case reflection::Float:  SetField<float> (table, field,
                                              static_cast<float>(val)); break;
    case reflection::Double: SetField<double>(table, field, val); break;
    // TODO: support strings.
    default: SetAnyFieldI(table, field, static_cast<int64_t>(val)); break;
  }
}

// Set any field as a string, regardless of what it is (bool/int/float/str).
inline void SetAnyFieldS(Table *table, const reflection::Field *field,
                         const char *val) {
  switch (field->type()->base_type()) {
    case reflection::Float:
    case reflection::Double: SetAnyFieldF(table, field, strtod(val, nullptr));
    // TODO: support strings.
    default: SetAnyFieldI(table, field, StringToInt(val)); break;
  }
}

// "smart" pointer for use with resizing vectors: turns a pointer inside
// a vector into a relative offset, such that it is not affected by resizes.
template<typename T, typename U> class pointer_inside_vector {
 public:
  pointer_inside_vector(const T *ptr, const std::vector<U> &vec)
    : offset_(reinterpret_cast<const uint8_t *>(ptr) -
              reinterpret_cast<const uint8_t *>(vec.data())),
      vec_(vec) {}

  const T *operator*() const {
    return reinterpret_cast<const T *>(
             reinterpret_cast<const uint8_t *>(vec_.data()) + offset_);
  }
  const T *operator->() const {
    return operator*();
  }
  void operator=(const pointer_inside_vector &piv);
 private:
  size_t offset_;
  const std::vector<U> &vec_;
};

// Helper to create the above easily without specifying template args.
template<typename T, typename U> pointer_inside_vector<T, U> piv(
    const T *ptr, const std::vector<U> &vec) {
  return pointer_inside_vector<T, U>(ptr, vec);
}

// Resize a FlatBuffer in-place by iterating through all offsets in the buffer
// and adjusting them by "delta" if they straddle the start offset.
// Once that is done, bytes can now be inserted/deleted safely.
// "delta" may be negative (shrinking).
// Unless "delta" is a multiple of the largest alignment, you'll create a small
// amount of garbage space in the buffer.
class ResizeContext {
 public:
  ResizeContext(const reflection::Schema &schema, uoffset_t start, int delta,
                std::vector<uint8_t> *flatbuf)
     : schema_(schema), startptr_(flatbuf->data() + start),
       delta_(delta), buf_(*flatbuf),
       dag_check_(flatbuf->size() / sizeof(uoffset_t), false) {
    auto mask = sizeof(largest_scalar_t) - 1;
    delta_ = (delta_ + mask) & ~mask;
    if (!delta_) return;  // We can't shrink by less than largest_scalar_t.
    // Now change all the offsets by delta_.
    auto root = GetAnyRoot(buf_.data());
    Straddle<uoffset_t, 1>(buf_.data(), root, buf_.data());
    ResizeTable(schema.root_table(), root);
    // We can now add or remove bytes at start.
    if (delta_ > 0) buf_.insert(buf_.begin() + start, delta_, 0);
    else buf_.erase(buf_.begin() + start, buf_.begin() + start - delta_);
  }

  // Check if the range between first (lower address) and second straddles
  // the insertion point. If it does, change the offset at offsetloc (of
  // type T, with direction D).
  template<typename T, int D> void Straddle(void *first, void *second,
                                            void *offsetloc) {
    if (first <= startptr_ && second >= startptr_) {
      WriteScalar<T>(offsetloc, ReadScalar<T>(offsetloc) + delta_ * D);
      DagCheck(offsetloc) = true;
    }
  }

  // This returns a boolean that records if the corresponding offset location
  // has been modified already. If so, we can't even read the corresponding
  // offset, since it is pointing to a location that is illegal until the
  // resize actually happens.
  // This must be checked for every offset, since we can't know which offsets
  // will straddle and which won't.
  uint8_t &DagCheck(void *offsetloc) {
    auto dag_idx = reinterpret_cast<uoffset_t *>(offsetloc) -
                   reinterpret_cast<uoffset_t *>(buf_.data());
    return dag_check_[dag_idx];
  }

  void ResizeTable(const reflection::Object *objectdef, Table *table) {
    if (DagCheck(table))
      return;  // Table already visited.
    auto vtable = table->GetVTable();
    // Check if the vtable offset points beyond the insertion point.
    Straddle<soffset_t, -1>(table, vtable, table);
    // This direction shouldn't happen because vtables that sit before tables
    // are always directly adjacent, but check just in case we ever change the
    // way flatbuffers are built.
    Straddle<soffset_t, -1>(vtable, table, table);
    // Early out: since all fields inside the table must point forwards in
    // memory, if the insertion point is before the table we can stop here.
    auto tableloc = reinterpret_cast<uint8_t *>(table);
    if (startptr_ <= tableloc) return;
    // Check each field.
    auto fielddefs = objectdef->fields();
    for (auto it = fielddefs->begin(); it != fielddefs->end(); ++it) {
      auto fielddef = *it;
      auto base_type = fielddef->type()->base_type();
      // Ignore scalars.
      if (base_type <= reflection::Double) continue;
      // Ignore fields that are not stored.
      auto offset = table->GetOptionalFieldOffset(fielddef->offset());
      if (!offset) continue;
      // Ignore structs.
      auto subobjectdef = base_type == reflection::Obj ?
        schema_.objects()->Get(fielddef->type()->index()) : nullptr;
      if (subobjectdef && subobjectdef->is_struct()) continue;
      // Get this fields' offset, and read it if safe.
      auto offsetloc = tableloc + offset;
      if (DagCheck(offsetloc))
        continue;  // This offset already visited.
      auto ref = offsetloc + ReadScalar<uoffset_t>(offsetloc);
      Straddle<uoffset_t, 1>(offsetloc, ref, offsetloc);
      // Recurse.
      switch (base_type) {
        case reflection::Obj: {
          ResizeTable(subobjectdef, reinterpret_cast<Table *>(ref));
          break;
        }
        case reflection::Vector: {
          if (fielddef->type()->element() != reflection::Obj) break;
          auto vec = reinterpret_cast<Vector<uoffset_t> *>(ref);
          auto elemobjectdef =
            schema_.objects()->Get(fielddef->type()->index());
          if (elemobjectdef->is_struct()) break;
          for (uoffset_t i = 0; i < vec->size(); i++) {
            auto loc = vec->Data() + i * sizeof(uoffset_t);
            if (DagCheck(loc))
              continue;  // This offset already visited.
            auto dest = loc + vec->Get(i);
            Straddle<uoffset_t, 1>(loc, dest ,loc);
            ResizeTable(elemobjectdef, reinterpret_cast<Table *>(dest));
          }
          break;
        }
        case reflection::Union: {
          auto enumdef = schema_.enums()->Get(fielddef->type()->index());
          // TODO: this is clumsy and slow, but no other way to find it?
          auto type_field = fielddefs->LookupByKey(
                    (fielddef->name()->c_str() + std::string("_type")).c_str());
          assert(type_field);
          auto union_type = GetFieldI<uint8_t>(table, type_field);
          auto enumval = enumdef->values()->LookupByKey(union_type);
          ResizeTable(enumval->object(), reinterpret_cast<Table *>(ref));
          break;
        }
        case reflection::String:
          break;
        default:
          assert(false);
      }
    }
  }

  void operator=(const ResizeContext &rc);

 private:
  const reflection::Schema &schema_;
  uint8_t *startptr_;
  int delta_;
  std::vector<uint8_t> &buf_;
  std::vector<uint8_t> dag_check_;
};

// Changes the contents of a string inside a FlatBuffer. FlatBuffer must
// live inside a std::vector so we can resize the buffer if needed.
// "str" must live inside "flatbuf" and may be invalidated after this call.
inline void SetString(const reflection::Schema &schema, const std::string &val,
                      const String *str, std::vector<uint8_t> *flatbuf) {
  auto delta = static_cast<int>(val.size()) - static_cast<int>(str->Length());
  auto start = static_cast<uoffset_t>(reinterpret_cast<const uint8_t *>(str) -
                                      flatbuf->data() +
                                      sizeof(uoffset_t));
  if (delta) {
    // Different size, we must expand (or contract).
    ResizeContext(schema, start, delta, flatbuf);
    if (delta < 0) {
      // Clear the old string, since we don't want parts of it remaining.
      memset(flatbuf->data() + start, 0, str->Length());
    }
  }
  // Copy new data. Safe because we created the right amount of space.
  memcpy(flatbuf->data() + start, val.c_str(), val.size() + 1);
}

// Resizes a flatbuffers::Vector inside a FlatBuffer. FlatBuffer must
// live inside a std::vector so we can resize the buffer if needed.
// "vec" must live inside "flatbuf" and may be invalidated after this call.
template<typename T> void ResizeVector(const reflection::Schema &schema,
                                       uoffset_t newsize, T val,
                                       const Vector<T> *vec,
                                       std::vector<uint8_t> *flatbuf) {
  auto delta_elem = static_cast<int>(newsize) - static_cast<int>(vec->size());
  auto delta_bytes = delta_elem * static_cast<int>(sizeof(T));
  auto vec_start = reinterpret_cast<const uint8_t *>(vec) - flatbuf->data();
  auto start = static_cast<uoffset_t>(vec_start + sizeof(uoffset_t) +
                                      sizeof(T) * vec->size());
  if (delta_bytes) {
    ResizeContext(schema, start, delta_bytes, flatbuf);
    WriteScalar(flatbuf->data() + vec_start, newsize);  // Length field.
    // Set new elements to "val".
    for (int i = 0; i < delta_elem; i++) {
      auto loc = flatbuf->data() + start + i * sizeof(T);
      auto is_scalar = std::is_scalar<T>::value;
      if (is_scalar) {
        WriteScalar(loc, val);
      } else {  // struct
        *reinterpret_cast<T *>(loc) = val;
      }
    }
  }
}


}  // namespace flatbuffers

#endif  // FLATBUFFERS_REFLECTION_H_
