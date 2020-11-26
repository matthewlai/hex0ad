// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: actor.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_actor_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_actor_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3013000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3013000 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_actor_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_actor_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[3]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_actor_2eproto;
namespace hex0ad {
class Actor;
class ActorDefaultTypeInternal;
extern ActorDefaultTypeInternal _Actor_default_instance_;
class Group;
class GroupDefaultTypeInternal;
extern GroupDefaultTypeInternal _Group_default_instance_;
class Variant;
class VariantDefaultTypeInternal;
extern VariantDefaultTypeInternal _Variant_default_instance_;
}  // namespace hex0ad
PROTOBUF_NAMESPACE_OPEN
template<> ::hex0ad::Actor* Arena::CreateMaybeMessage<::hex0ad::Actor>(Arena*);
template<> ::hex0ad::Group* Arena::CreateMaybeMessage<::hex0ad::Group>(Arena*);
template<> ::hex0ad::Variant* Arena::CreateMaybeMessage<::hex0ad::Variant>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace hex0ad {

// ===================================================================

class Variant PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:hex0ad.Variant) */ {
 public:
  inline Variant() : Variant(nullptr) {}
  virtual ~Variant();

  Variant(const Variant& from);
  Variant(Variant&& from) noexcept
    : Variant() {
    *this = ::std::move(from);
  }

  inline Variant& operator=(const Variant& from) {
    CopyFrom(from);
    return *this;
  }
  inline Variant& operator=(Variant&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const Variant& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Variant* internal_default_instance() {
    return reinterpret_cast<const Variant*>(
               &_Variant_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(Variant& a, Variant& b) {
    a.Swap(&b);
  }
  inline void Swap(Variant* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Variant* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Variant* New() const final {
    return CreateMaybeMessage<Variant>(nullptr);
  }

  Variant* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Variant>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const Variant& from);
  void MergeFrom(const Variant& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Variant* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "hex0ad.Variant";
  }
  protected:
  explicit Variant(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_actor_2eproto);
    return ::descriptor_table_actor_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNameFieldNumber = 1,
    kMeshPathFieldNumber = 3,
    kFrequencyFieldNumber = 2,
  };
  // optional string name = 1;
  bool has_name() const;
  private:
  bool _internal_has_name() const;
  public:
  void clear_name();
  const std::string& name() const;
  void set_name(const std::string& value);
  void set_name(std::string&& value);
  void set_name(const char* value);
  void set_name(const char* value, size_t size);
  std::string* mutable_name();
  std::string* release_name();
  void set_allocated_name(std::string* name);
  private:
  const std::string& _internal_name() const;
  void _internal_set_name(const std::string& value);
  std::string* _internal_mutable_name();
  public:

  // optional string mesh_path = 3;
  bool has_mesh_path() const;
  private:
  bool _internal_has_mesh_path() const;
  public:
  void clear_mesh_path();
  const std::string& mesh_path() const;
  void set_mesh_path(const std::string& value);
  void set_mesh_path(std::string&& value);
  void set_mesh_path(const char* value);
  void set_mesh_path(const char* value, size_t size);
  std::string* mutable_mesh_path();
  std::string* release_mesh_path();
  void set_allocated_mesh_path(std::string* mesh_path);
  private:
  const std::string& _internal_mesh_path() const;
  void _internal_set_mesh_path(const std::string& value);
  std::string* _internal_mutable_mesh_path();
  public:

  // optional float frequency = 2;
  bool has_frequency() const;
  private:
  bool _internal_has_frequency() const;
  public:
  void clear_frequency();
  float frequency() const;
  void set_frequency(float value);
  private:
  float _internal_frequency() const;
  void _internal_set_frequency(float value);
  public:

  // @@protoc_insertion_point(class_scope:hex0ad.Variant)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr mesh_path_;
  float frequency_;
  friend struct ::TableStruct_actor_2eproto;
};
// -------------------------------------------------------------------

class Group PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:hex0ad.Group) */ {
 public:
  inline Group() : Group(nullptr) {}
  virtual ~Group();

  Group(const Group& from);
  Group(Group&& from) noexcept
    : Group() {
    *this = ::std::move(from);
  }

  inline Group& operator=(const Group& from) {
    CopyFrom(from);
    return *this;
  }
  inline Group& operator=(Group&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const Group& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Group* internal_default_instance() {
    return reinterpret_cast<const Group*>(
               &_Group_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(Group& a, Group& b) {
    a.Swap(&b);
  }
  inline void Swap(Group* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Group* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Group* New() const final {
    return CreateMaybeMessage<Group>(nullptr);
  }

  Group* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Group>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const Group& from);
  void MergeFrom(const Group& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Group* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "hex0ad.Group";
  }
  protected:
  explicit Group(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_actor_2eproto);
    return ::descriptor_table_actor_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kVariantsFieldNumber = 1,
  };
  // repeated .hex0ad.Variant variants = 1;
  int variants_size() const;
  private:
  int _internal_variants_size() const;
  public:
  void clear_variants();
  ::hex0ad::Variant* mutable_variants(int index);
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Variant >*
      mutable_variants();
  private:
  const ::hex0ad::Variant& _internal_variants(int index) const;
  ::hex0ad::Variant* _internal_add_variants();
  public:
  const ::hex0ad::Variant& variants(int index) const;
  ::hex0ad::Variant* add_variants();
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Variant >&
      variants() const;

  // @@protoc_insertion_point(class_scope:hex0ad.Group)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Variant > variants_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_actor_2eproto;
};
// -------------------------------------------------------------------

class Actor PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:hex0ad.Actor) */ {
 public:
  inline Actor() : Actor(nullptr) {}
  virtual ~Actor();

  Actor(const Actor& from);
  Actor(Actor&& from) noexcept
    : Actor() {
    *this = ::std::move(from);
  }

  inline Actor& operator=(const Actor& from) {
    CopyFrom(from);
    return *this;
  }
  inline Actor& operator=(Actor&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const Actor& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Actor* internal_default_instance() {
    return reinterpret_cast<const Actor*>(
               &_Actor_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;

  friend void swap(Actor& a, Actor& b) {
    a.Swap(&b);
  }
  inline void Swap(Actor* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Actor* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Actor* New() const final {
    return CreateMaybeMessage<Actor>(nullptr);
  }

  Actor* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Actor>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const Actor& from);
  void MergeFrom(const Actor& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Actor* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "hex0ad.Actor";
  }
  protected:
  explicit Actor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_actor_2eproto);
    return ::descriptor_table_actor_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kGroupsFieldNumber = 1,
  };
  // repeated .hex0ad.Group groups = 1;
  int groups_size() const;
  private:
  int _internal_groups_size() const;
  public:
  void clear_groups();
  ::hex0ad::Group* mutable_groups(int index);
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Group >*
      mutable_groups();
  private:
  const ::hex0ad::Group& _internal_groups(int index) const;
  ::hex0ad::Group* _internal_add_groups();
  public:
  const ::hex0ad::Group& groups(int index) const;
  ::hex0ad::Group* add_groups();
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Group >&
      groups() const;

  // @@protoc_insertion_point(class_scope:hex0ad.Actor)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Group > groups_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_actor_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// Variant

// optional string name = 1;
inline bool Variant::_internal_has_name() const {
  bool value = (_has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool Variant::has_name() const {
  return _internal_has_name();
}
inline void Variant::clear_name() {
  name_.ClearToEmpty(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
  _has_bits_[0] &= ~0x00000001u;
}
inline const std::string& Variant::name() const {
  // @@protoc_insertion_point(field_get:hex0ad.Variant.name)
  return _internal_name();
}
inline void Variant::set_name(const std::string& value) {
  _internal_set_name(value);
  // @@protoc_insertion_point(field_set:hex0ad.Variant.name)
}
inline std::string* Variant::mutable_name() {
  // @@protoc_insertion_point(field_mutable:hex0ad.Variant.name)
  return _internal_mutable_name();
}
inline const std::string& Variant::_internal_name() const {
  return name_.Get();
}
inline void Variant::_internal_set_name(const std::string& value) {
  _has_bits_[0] |= 0x00000001u;
  name_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), value, GetArena());
}
inline void Variant::set_name(std::string&& value) {
  _has_bits_[0] |= 0x00000001u;
  name_.Set(
    &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::move(value), GetArena());
  // @@protoc_insertion_point(field_set_rvalue:hex0ad.Variant.name)
}
inline void Variant::set_name(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _has_bits_[0] |= 0x00000001u;
  name_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArena());
  // @@protoc_insertion_point(field_set_char:hex0ad.Variant.name)
}
inline void Variant::set_name(const char* value,
    size_t size) {
  _has_bits_[0] |= 0x00000001u;
  name_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArena());
  // @@protoc_insertion_point(field_set_pointer:hex0ad.Variant.name)
}
inline std::string* Variant::_internal_mutable_name() {
  _has_bits_[0] |= 0x00000001u;
  return name_.Mutable(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline std::string* Variant::release_name() {
  // @@protoc_insertion_point(field_release:hex0ad.Variant.name)
  if (!_internal_has_name()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000001u;
  return name_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void Variant::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    _has_bits_[0] |= 0x00000001u;
  } else {
    _has_bits_[0] &= ~0x00000001u;
  }
  name_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), name,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:hex0ad.Variant.name)
}

// optional float frequency = 2;
inline bool Variant::_internal_has_frequency() const {
  bool value = (_has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool Variant::has_frequency() const {
  return _internal_has_frequency();
}
inline void Variant::clear_frequency() {
  frequency_ = 0;
  _has_bits_[0] &= ~0x00000004u;
}
inline float Variant::_internal_frequency() const {
  return frequency_;
}
inline float Variant::frequency() const {
  // @@protoc_insertion_point(field_get:hex0ad.Variant.frequency)
  return _internal_frequency();
}
inline void Variant::_internal_set_frequency(float value) {
  _has_bits_[0] |= 0x00000004u;
  frequency_ = value;
}
inline void Variant::set_frequency(float value) {
  _internal_set_frequency(value);
  // @@protoc_insertion_point(field_set:hex0ad.Variant.frequency)
}

// optional string mesh_path = 3;
inline bool Variant::_internal_has_mesh_path() const {
  bool value = (_has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool Variant::has_mesh_path() const {
  return _internal_has_mesh_path();
}
inline void Variant::clear_mesh_path() {
  mesh_path_.ClearToEmpty(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
  _has_bits_[0] &= ~0x00000002u;
}
inline const std::string& Variant::mesh_path() const {
  // @@protoc_insertion_point(field_get:hex0ad.Variant.mesh_path)
  return _internal_mesh_path();
}
inline void Variant::set_mesh_path(const std::string& value) {
  _internal_set_mesh_path(value);
  // @@protoc_insertion_point(field_set:hex0ad.Variant.mesh_path)
}
inline std::string* Variant::mutable_mesh_path() {
  // @@protoc_insertion_point(field_mutable:hex0ad.Variant.mesh_path)
  return _internal_mutable_mesh_path();
}
inline const std::string& Variant::_internal_mesh_path() const {
  return mesh_path_.Get();
}
inline void Variant::_internal_set_mesh_path(const std::string& value) {
  _has_bits_[0] |= 0x00000002u;
  mesh_path_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), value, GetArena());
}
inline void Variant::set_mesh_path(std::string&& value) {
  _has_bits_[0] |= 0x00000002u;
  mesh_path_.Set(
    &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::move(value), GetArena());
  // @@protoc_insertion_point(field_set_rvalue:hex0ad.Variant.mesh_path)
}
inline void Variant::set_mesh_path(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _has_bits_[0] |= 0x00000002u;
  mesh_path_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArena());
  // @@protoc_insertion_point(field_set_char:hex0ad.Variant.mesh_path)
}
inline void Variant::set_mesh_path(const char* value,
    size_t size) {
  _has_bits_[0] |= 0x00000002u;
  mesh_path_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArena());
  // @@protoc_insertion_point(field_set_pointer:hex0ad.Variant.mesh_path)
}
inline std::string* Variant::_internal_mutable_mesh_path() {
  _has_bits_[0] |= 0x00000002u;
  return mesh_path_.Mutable(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline std::string* Variant::release_mesh_path() {
  // @@protoc_insertion_point(field_release:hex0ad.Variant.mesh_path)
  if (!_internal_has_mesh_path()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000002u;
  return mesh_path_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void Variant::set_allocated_mesh_path(std::string* mesh_path) {
  if (mesh_path != nullptr) {
    _has_bits_[0] |= 0x00000002u;
  } else {
    _has_bits_[0] &= ~0x00000002u;
  }
  mesh_path_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), mesh_path,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:hex0ad.Variant.mesh_path)
}

// -------------------------------------------------------------------

// Group

// repeated .hex0ad.Variant variants = 1;
inline int Group::_internal_variants_size() const {
  return variants_.size();
}
inline int Group::variants_size() const {
  return _internal_variants_size();
}
inline void Group::clear_variants() {
  variants_.Clear();
}
inline ::hex0ad::Variant* Group::mutable_variants(int index) {
  // @@protoc_insertion_point(field_mutable:hex0ad.Group.variants)
  return variants_.Mutable(index);
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Variant >*
Group::mutable_variants() {
  // @@protoc_insertion_point(field_mutable_list:hex0ad.Group.variants)
  return &variants_;
}
inline const ::hex0ad::Variant& Group::_internal_variants(int index) const {
  return variants_.Get(index);
}
inline const ::hex0ad::Variant& Group::variants(int index) const {
  // @@protoc_insertion_point(field_get:hex0ad.Group.variants)
  return _internal_variants(index);
}
inline ::hex0ad::Variant* Group::_internal_add_variants() {
  return variants_.Add();
}
inline ::hex0ad::Variant* Group::add_variants() {
  // @@protoc_insertion_point(field_add:hex0ad.Group.variants)
  return _internal_add_variants();
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Variant >&
Group::variants() const {
  // @@protoc_insertion_point(field_list:hex0ad.Group.variants)
  return variants_;
}

// -------------------------------------------------------------------

// Actor

// repeated .hex0ad.Group groups = 1;
inline int Actor::_internal_groups_size() const {
  return groups_.size();
}
inline int Actor::groups_size() const {
  return _internal_groups_size();
}
inline void Actor::clear_groups() {
  groups_.Clear();
}
inline ::hex0ad::Group* Actor::mutable_groups(int index) {
  // @@protoc_insertion_point(field_mutable:hex0ad.Actor.groups)
  return groups_.Mutable(index);
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Group >*
Actor::mutable_groups() {
  // @@protoc_insertion_point(field_mutable_list:hex0ad.Actor.groups)
  return &groups_;
}
inline const ::hex0ad::Group& Actor::_internal_groups(int index) const {
  return groups_.Get(index);
}
inline const ::hex0ad::Group& Actor::groups(int index) const {
  // @@protoc_insertion_point(field_get:hex0ad.Actor.groups)
  return _internal_groups(index);
}
inline ::hex0ad::Group* Actor::_internal_add_groups() {
  return groups_.Add();
}
inline ::hex0ad::Group* Actor::add_groups() {
  // @@protoc_insertion_point(field_add:hex0ad.Actor.groups)
  return _internal_add_groups();
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::hex0ad::Group >&
Actor::groups() const {
  // @@protoc_insertion_point(field_list:hex0ad.Actor.groups)
  return groups_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace hex0ad

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_actor_2eproto