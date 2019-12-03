// Copyright 2019 Rover Robotics via Dan Rose
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "TypeSupport2.hpp"

#include <unordered_map>
#include <utility>

namespace rmw_cyclonedds_cpp
{
static std::unordered_map<const rosidl_message_type_support_t *,
  std::unique_ptr<const StructValueType>>
s_struct_cache;

class ROSIDLC_StructValueType : public StructValueType
{
  const rosidl_typesupport_introspection_c__MessageMembers impl;
  std::vector<Member> m_members;
  std::vector<std::unique_ptr<AnyValueType>> m_inner_value_types;
  template<typename ConstructedType, typename ... Args>
  ConstructedType * make_value_type(Args && ... args)
  {
    auto unique_ptr = std::make_unique<ConstructedType>(std::forward<Args>(args)...);
    auto ptr = unique_ptr.get();
    m_inner_value_types.push_back(std::move(unique_ptr));
    return ptr;
  }

public:
  static constexpr TypeGenerator gen = TypeGenerator::ROSIDL_C;
  explicit ROSIDLC_StructValueType(decltype(impl) impl);
  size_t sizeof_struct() const override {return impl.size_of_;}
  size_t n_members() const override {return impl.member_count_;}
  const Member * get_member(size_t index) const override {return &m_members.at(index);}
};

class ROSIDLCPP_StructValueType : public StructValueType
{
  const rosidl_typesupport_introspection_cpp::MessageMembers impl;
  std::vector<Member> m_members;
  std::vector<std::unique_ptr<AnyValueType>> m_inner_value_types;
  template<typename ConstructedType, typename ... Args>
  ConstructedType * make_value_type(Args && ... args)
  {
    auto unique_ptr = std::make_unique<ConstructedType>(std::forward<Args>(args)...);
    auto ptr = unique_ptr.get();
    m_inner_value_types.push_back(std::move(unique_ptr));
    return ptr;
  }

public:
  static constexpr TypeGenerator gen = TypeGenerator::ROSIDL_Cpp;
  explicit ROSIDLCPP_StructValueType(decltype(impl) impl);
  size_t sizeof_struct() const override {return impl.size_of_;}
  size_t n_members() const override {return impl.member_count_;}
  const Member * get_member(size_t index) const final {return &m_members.at(index);}
};

const StructValueType * struct_type_from_rosidl(const rosidl_message_type_support_t * mts)
{
  auto iter = s_struct_cache.find(mts);
  if (iter != s_struct_cache.end()) {
    return iter->second.get();
  }
  std::unique_ptr<const StructValueType> result;
  {
    if (auto ts = mts->func(mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::identifier)) {
      auto members = static_cast<const MetaMessage<TypeGenerator::ROSIDL_C> *>(ts->data);
      result = std::make_unique<ROSIDLC_StructValueType>(*members);
    } else if (auto ts = mts->func(mts, TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::identifier)) {
      auto members = static_cast<const MetaMessage<TypeGenerator::ROSIDL_Cpp> *>(ts->data);
      result = std::make_unique<ROSIDLCPP_StructValueType>(*members);
    } else {
      throw std::runtime_error(
              "could not identify message typesupport " + std::string(mts->typesupport_identifier));
    }
    auto ret = result.get();
    s_struct_cache[mts] = std::move(result);
    return ret;
  }
}

std::pair<rosidl_message_type_support_t, rosidl_message_type_support_t>
get_svc_request_response_typesupports(const rosidl_service_type_support_t * svc_ts)
{
  rosidl_message_type_support_t request;
  rosidl_message_type_support_t response;

  request.typesupport_identifier = response.typesupport_identifier = svc_ts->typesupport_identifier;
  request.func = response.func = get_message_typesupport_handle_function;

  if (auto ts = svc_ts->func(svc_ts, TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::identifier)) {
    auto typed =
      static_cast<const TypeGeneratorInfo<TypeGenerator::ROSIDL_C>::MetaService *>(ts->data);
    request.data = typed->request_members_;
    response.data = typed->response_members_;
  } else if (auto ts = svc_ts->func(svc_ts,
    TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::identifier))
  {
    auto typed =
      static_cast<const TypeGeneratorInfo<TypeGenerator::ROSIDL_Cpp>::MetaService *>(ts->data);
    request.data = typed->request_members_;
    response.data = typed->response_members_;
  } else {
    throw std::runtime_error("Unidentified service type support: " +
            std::string(svc_ts->typesupport_identifier));
  }

  return {request, response};
}


ROSIDLC_StructValueType::ROSIDLC_StructValueType(decltype(impl) impl)
: impl(impl)
{
  for (size_t index = 0; index < impl.member_count_; index++) {
    size_t next_member_offset;
    if (index + 1 == impl.member_count_) {
      next_member_offset = impl.size_of_;
    } else {
      next_member_offset = impl.members_[index + 1].offset_;
    }
    auto member_impl = impl.members_[index];

    const AnyValueType * element_value_type;
    switch (ROSIDL_TypeKind(member_impl.type_id_)) {
      case ROSIDL_TypeKind::MESSAGE:
        element_value_type = struct_type_from_rosidl(member_impl.members_);
        break;
      case ROSIDL_TypeKind::STRING: {
          element_value_type = make_value_type<ROSIDLC_StringValueType>();
        } break;
      case ROSIDL_TypeKind::WSTRING: {
          element_value_type = make_value_type<ROSIDLC_WStringValueType>();
        } break;
      default: {
          element_value_type =
            make_value_type<PrimitiveValueType>(ROSIDL_TypeKind(member_impl.type_id_));
        } break;
    }

    const AnyValueType * member_value_type;
    if (!member_impl.is_array_) {
      member_value_type = element_value_type;
    } else if (member_impl.array_size_ != 0 && !member_impl.is_upper_bound_) {
      member_value_type = make_value_type<ArrayValueType>(element_value_type,
          member_impl.array_size_);
    } else if (member_impl.size_function) {
      member_value_type = make_value_type<CallbackSpanSequenceValueType>(element_value_type,
          member_impl.size_function,
          member_impl.get_const_function);
    } else {
      member_value_type = make_value_type<ROSIDLC_SpanSequenceValueType>(element_value_type);
    }
    auto a_member = Member{
      member_impl.name_,
      member_value_type,
      member_impl.offset_,
      next_member_offset,
    };
    m_members.push_back(a_member);
  }
}

ROSIDLCPP_StructValueType::ROSIDLCPP_StructValueType(decltype(impl) impl)
: impl(impl)
{
  for (size_t index = 0; index < impl.member_count_; index++) {
    Member a_member;

    size_t next_member_offset;

    if (index + 1 == impl.member_count_) {
      next_member_offset = impl.size_of_;
    } else {
      next_member_offset = impl.members_[index + 1].offset_;
    }
    a_member.next_member_offset = next_member_offset;

    auto member_impl = impl.members_[index];
    a_member.member_offset = member_impl.offset_;
    a_member.name = member_impl.name_;

    const AnyValueType * element_value_type;
    switch (ROSIDL_TypeKind(member_impl.type_id_)) {
      case ROSIDL_TypeKind::MESSAGE:
        element_value_type = struct_type_from_rosidl(member_impl.members_);
        break;
      case ROSIDL_TypeKind::STRING:
        element_value_type = make_value_type<ROSIDLCPP_StringValueType>();
        break;
      case ROSIDL_TypeKind::WSTRING:
        element_value_type = make_value_type<ROSIDLCPP_U16StringValueType>();
        break;
      default:
        element_value_type =
          make_value_type<PrimitiveValueType>(ROSIDL_TypeKind(member_impl.type_id_));
        break;
    }

    if (!member_impl.is_array_) {
      a_member.value_type = element_value_type;
    } else if (member_impl.array_size_ != 0 && !member_impl.is_upper_bound_) {
      a_member.value_type = make_value_type<ArrayValueType>(element_value_type,
          member_impl.array_size_);
    } else if (ROSIDL_TypeKind(member_impl.type_id_) == ROSIDL_TypeKind::BOOLEAN) {
      a_member.value_type = make_value_type<BoolVectorValueType>();
    } else {
      a_member.value_type = make_value_type<CallbackSpanSequenceValueType>(
        element_value_type, member_impl.size_function, member_impl.get_const_function);
    }
    m_members.push_back(a_member);
  }
}
}  // namespace rmw_cyclonedds_cpp
