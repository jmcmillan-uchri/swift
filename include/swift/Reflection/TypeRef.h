//===--- TypeRef.h - Swift Type References for Reflection -------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// Implements the structures of type references for property and enum
// case reflection.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_REFLECTION_TYPEREF_H
#define SWIFT_REFLECTION_TYPEREF_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Casting.h"

#include <iostream>

namespace swift {
namespace reflection {

template <typename Allocator>
class ReflectionContext;

struct ReflectionInfo;

using llvm::cast;
using llvm::dyn_cast;

enum class TypeRefKind {
#define TYPEREF(Id, Parent) Id,
#include "swift/Reflection/TypeRefs.def"
#undef TYPEREF
};

class TypeRef;
using DepthAndIndex = std::pair<unsigned, unsigned>;
using GenericArgumentMap = llvm::DenseMap<DepthAndIndex, const TypeRef *>;

class alignas(void *) TypeRef {
  TypeRefKind Kind;

public:
  TypeRef(TypeRefKind Kind) : Kind(Kind) {}

  TypeRefKind getKind() const {
    return Kind;
  }

  void dump() const;
  void dump(std::ostream &OS, unsigned Indent = 0) const;

  bool isConcrete() const;

  template <typename Runtime>
  const TypeRef *
  subst(ReflectionContext<Runtime> &RC, GenericArgumentMap Subs) const;

  GenericArgumentMap getSubstMap() const;

  virtual ~TypeRef() = default;
};

class BuiltinTypeRef final : public TypeRef {
  std::string MangledName;

public:
  BuiltinTypeRef(const std::string &MangledName)
    : TypeRef(TypeRefKind::Builtin), MangledName(MangledName) {}

  template <typename Allocator>
  static const BuiltinTypeRef *create(Allocator &A, std::string MangledName) {
    return A.template make_typeref<BuiltinTypeRef>(MangledName);
  }

  const std::string &getMangledName() const {
    return MangledName;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Builtin;
  }
};

class NominalTypeTrait {
  std::string MangledName;
  const TypeRef *Parent;

protected:
  NominalTypeTrait(const std::string &MangledName, const TypeRef *Parent)
      : MangledName(MangledName), Parent(Parent) {}

public:
  const std::string &getMangledName() const {
    return MangledName;
  }

  bool isStruct() const;
  bool isEnum() const;
  bool isClass() const;

  const TypeRef *getParent() const {
    return Parent;
  }

  unsigned getDepth() const;
};

class NominalTypeRef final : public TypeRef, public NominalTypeTrait {
public:
  NominalTypeRef(const std::string &MangledName,
                 const TypeRef *Parent = nullptr)
    : TypeRef(TypeRefKind::Nominal), NominalTypeTrait(MangledName, Parent) {}

  template <typename Allocator>
  static const NominalTypeRef *create(Allocator &A,
                                      const std::string &MangledName,
                                      const TypeRef *Parent = nullptr) {
    return A.template make_typeref<NominalTypeRef>(MangledName, Parent);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Nominal;
  }
};

class BoundGenericTypeRef final : public TypeRef, public NominalTypeTrait {
  std::vector<const TypeRef *> GenericParams;

public:
  BoundGenericTypeRef(const std::string &MangledName,
                      std::vector<const TypeRef *> GenericParams,
                      const TypeRef *Parent = nullptr)
    : TypeRef(TypeRefKind::BoundGeneric),
      NominalTypeTrait(MangledName, Parent),
      GenericParams(GenericParams) {}

  template <typename Allocator>
  static const BoundGenericTypeRef *
  create(Allocator &A, const std::string &MangledName,
         std::vector<const TypeRef *> GenericParams,
         const TypeRef *Parent = nullptr) {
    return A.template make_typeref<BoundGenericTypeRef>(MangledName,
                                                        GenericParams,
                                                        Parent);
  }

  const std::vector<const TypeRef *> &getGenericParams() const {
    return GenericParams;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::BoundGeneric;
  }
};

class TupleTypeRef final : public TypeRef {
  std::vector<const TypeRef *> Elements;
  bool Variadic;

public:
  TupleTypeRef(std::vector<const TypeRef *> Elements, bool Variadic=false)
    : TypeRef(TypeRefKind::Tuple), Elements(Elements), Variadic(Variadic) {}

  template <typename Allocator>
  static TupleTypeRef *create(Allocator &A,
                              std::vector<const TypeRef *> Elements,
                              bool Variadic = false) {
    return A.template make_typeref<TupleTypeRef>(Elements, Variadic);
  }

  const std::vector<const TypeRef *> &getElements() const {
    return Elements;
  };

  bool isVariadic() const {
    return Variadic;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Tuple;
  }
};

class FunctionTypeRef final : public TypeRef {
  std::vector<const TypeRef *> Arguments;
  const TypeRef *Result;

public:
  FunctionTypeRef(std::vector<const TypeRef *> Arguments, const TypeRef *Result)
    : TypeRef(TypeRefKind::Function), Arguments(Arguments), Result(Result) {}

  template <typename Allocator>
  static FunctionTypeRef *create(Allocator &A,
                                 std::vector<const TypeRef *> Arguments,
                                 const TypeRef *Result) {
    return A.template make_typeref<FunctionTypeRef>(Arguments, Result);
  }

  const std::vector<const TypeRef *> &getArguments() const {
    return Arguments;
  };

  const TypeRef *getResult() const {
    return Result;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Function;
  }
};

class ProtocolTypeRef final : public TypeRef {
  std::string ModuleName;
  std::string Name;

public:
  ProtocolTypeRef(const std::string &ModuleName,
                  const std::string &Name)
    : TypeRef(TypeRefKind::Protocol), ModuleName(ModuleName), Name(Name) {}

  template <typename Allocator>
  static const ProtocolTypeRef *
  create(Allocator &A, const std::string &ModuleName,
         const std::string &Name) {
    return A.template make_typeref<ProtocolTypeRef>(ModuleName, Name);
  }

  const std::string &getName() const {
    return Name;
  }

  const std::string &getModuleName() const {
    return ModuleName;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Protocol;
  }

  bool operator==(const ProtocolTypeRef &Other) const {
    return ModuleName.compare(Other.ModuleName) == 0 &&
           Name.compare(Other.Name) == 0;
  }
  bool operator!=(const ProtocolTypeRef &Other) const {
    return !(*this == Other);
  }
};

class ProtocolCompositionTypeRef final : public TypeRef {
  std::vector<const TypeRef *> Protocols;

public:
  ProtocolCompositionTypeRef(std::vector<const TypeRef *> Protocols)
    : TypeRef(TypeRefKind::ProtocolComposition), Protocols(Protocols) {}

  template <typename Allocator>
  static const ProtocolCompositionTypeRef *
  create(Allocator &A, std::vector<const TypeRef *> Protocols) {
    return A.template make_typeref<ProtocolCompositionTypeRef>(Protocols);
  }

  const std::vector<const TypeRef *> &getProtocols() const {
    return Protocols;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ProtocolComposition;
  }
};

class MetatypeTypeRef final : public TypeRef {
  const TypeRef *InstanceType;

public:
  MetatypeTypeRef(const TypeRef *InstanceType)
    : TypeRef(TypeRefKind::Metatype), InstanceType(InstanceType) {}

  template <typename Allocator>
  static const MetatypeTypeRef *create(Allocator &A,
                                 const TypeRef *InstanceType) {
    return A.template make_typeref<MetatypeTypeRef>(InstanceType);
  }

  const TypeRef *getInstanceType() const {
    return InstanceType;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Metatype;
  }
};

class ExistentialMetatypeTypeRef final : public TypeRef {
  const TypeRef *InstanceType;

public:
  ExistentialMetatypeTypeRef(const TypeRef *InstanceType)
    : TypeRef(TypeRefKind::ExistentialMetatype), InstanceType(InstanceType) {}

  template <typename Allocator>
  static const ExistentialMetatypeTypeRef *
  create(Allocator &A, const TypeRef *InstanceType) {
    return A.template make_typeref<ExistentialMetatypeTypeRef>(InstanceType);
  }

  const TypeRef *getInstanceType() const {
    return InstanceType;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ExistentialMetatype;
  }
};

class GenericTypeParameterTypeRef final : public TypeRef {
  const uint32_t Depth;
  const uint32_t Index;

public:
  GenericTypeParameterTypeRef(uint32_t Depth, uint32_t Index)
    : TypeRef(TypeRefKind::GenericTypeParameter), Depth(Depth), Index(Index) {}

  template <typename Allocator>
  static const GenericTypeParameterTypeRef *
  create(Allocator &A, uint32_t Depth, uint32_t Index) {
    return A.template make_typeref<GenericTypeParameterTypeRef>(Depth, Index);
  }

  uint32_t getDepth() const {
    return Depth;
  }

  uint32_t getIndex() const {
    return Index;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::GenericTypeParameter;
  }
};

class DependentMemberTypeRef final : public TypeRef {
  std::string Member;
  const TypeRef *Base;
  const TypeRef *Protocol;

public:
  DependentMemberTypeRef(const std::string &Member, const TypeRef *Base,
                         const TypeRef *Protocol)
    : TypeRef(TypeRefKind::DependentMember), Member(Member), Base(Base),
      Protocol(Protocol) {}

  template <typename Allocator>
  static const DependentMemberTypeRef *
  create(Allocator &A, const std::string &Member,
         const TypeRef *Base, const TypeRef *Protocol) {
    return A.template make_typeref<DependentMemberTypeRef>(Member, Base,
                                                           Protocol);
  }

  const std::string &getMember() const {
    return Member;
  }

  const TypeRef *getBase() const {
    return Base;
  }

  const ProtocolTypeRef *getProtocol() const {
    return cast<ProtocolTypeRef>(Protocol);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::DependentMember;
  }
};

class ForeignClassTypeRef final : public TypeRef {
  std::string Name;
  static const ForeignClassTypeRef *UnnamedSingleton;
public:
  ForeignClassTypeRef(const std::string &Name)
    : TypeRef(TypeRefKind::ForeignClass), Name(Name) {}

  static const ForeignClassTypeRef *getUnnamed();


  template <typename Allocator>
  static ForeignClassTypeRef *create(Allocator &A,
                                     const std::string &Name) {
    return A.template make_typeref<ForeignClassTypeRef>(Name);
  }

  const std::string &getName() const {
    return Name;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ForeignClass;
  }
};

class ObjCClassTypeRef final : public TypeRef {
  std::string Name;
  static const ObjCClassTypeRef *UnnamedSingleton;
public:
  ObjCClassTypeRef(const std::string &Name)
    : TypeRef(TypeRefKind::ObjCClass), Name(Name) {}

  static const ObjCClassTypeRef *getUnnamed();

  template <typename Allocator>
  static const ObjCClassTypeRef *create(Allocator &A, const std::string &Name) {
    return A.template make_typeref<ObjCClassTypeRef>(Name);
  }

  const std::string &getName() const {
    return Name;
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::ObjCClass;
  }
};

class OpaqueTypeRef final : public TypeRef {
  static const OpaqueTypeRef *Singleton;
public:
  OpaqueTypeRef() : TypeRef(TypeRefKind::Opaque) {}

  static const OpaqueTypeRef *get();

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::Opaque;
  }
};

class ReferenceStorageTypeRef : public TypeRef {
  const TypeRef *Type;

protected:
  ReferenceStorageTypeRef(TypeRefKind Kind, const TypeRef *Type)
    : TypeRef(Kind), Type(Type) {}

public:
  const TypeRef *getType() const {
    return Type;
  }
};

class UnownedStorageTypeRef final : public ReferenceStorageTypeRef {
public:
  UnownedStorageTypeRef(const TypeRef *Type)
    : ReferenceStorageTypeRef(TypeRefKind::UnownedStorage, Type) {}

  template <typename Allocator>
  static const UnownedStorageTypeRef *create(Allocator &A,
                                       const TypeRef *Type) {
    return A.template make_typeref<UnownedStorageTypeRef>(Type);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::UnownedStorage;
  }
};

class WeakStorageTypeRef final : public ReferenceStorageTypeRef {
public:
  WeakStorageTypeRef(const TypeRef *Type)
    : ReferenceStorageTypeRef(TypeRefKind::WeakStorage, Type) {}

  template <typename Allocator>
  static const WeakStorageTypeRef *create(Allocator &A,
                                    const TypeRef *Type) {
    return A.template make_typeref<WeakStorageTypeRef>(Type);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::WeakStorage;
  }
};

class UnmanagedStorageTypeRef final : public ReferenceStorageTypeRef {
public:
  UnmanagedStorageTypeRef(const TypeRef *Type)
    : ReferenceStorageTypeRef(TypeRefKind::UnmanagedStorage, Type) {}

  template <typename Allocator>
  static const UnmanagedStorageTypeRef *create(Allocator &A,
                                               const TypeRef *Type) {
    return A.template make_typeref<UnmanagedStorageTypeRef>(Type);
  }

  static bool classof(const TypeRef *TR) {
    return TR->getKind() == TypeRefKind::UnmanagedStorage;
  }
};

template <typename ImplClass, typename RetTy = void, typename... Args>
class TypeRefVisitor {
public:

  RetTy visit(const TypeRef *typeRef, Args... args) {
    switch (typeRef->getKind()) {
#define TYPEREF(Id, Parent) \
    case TypeRefKind::Id: \
      return static_cast<ImplClass*>(this) \
        ->visit##Id##TypeRef(cast<Id##TypeRef>(typeRef), \
                           ::std::forward<Args>(args)...);
#include "swift/Reflection/TypeRefs.def"
    }
  }
};

template <typename Runtime>
class TypeRefSubstitution
  : public TypeRefVisitor<TypeRefSubstitution<Runtime>, const TypeRef *> {
  using StoredPointer = typename Runtime::StoredPointer;
  ReflectionContext<Runtime> &RC;
  GenericArgumentMap Substitutions;
public:
  using TypeRefVisitor<TypeRefSubstitution<Runtime>, const TypeRef *>::visit;
  TypeRefSubstitution(ReflectionContext<Runtime> &RC,
                      GenericArgumentMap Substitutions)
  : RC(RC),
    Substitutions(Substitutions) {}

  const TypeRef *visitBuiltinTypeRef(const BuiltinTypeRef *B) {
    return B;
  }

  const TypeRef *visitNominalTypeRef(const NominalTypeRef *N) {
    return N;
  }

  const TypeRef *visitBoundGenericTypeRef(const BoundGenericTypeRef *BG) {
    std::vector<const TypeRef *> GenericParams;
    for (auto Param : BG->getGenericParams())
      GenericParams.push_back(visit(Param));
    return BoundGenericTypeRef::create(RC.Builder, BG->getMangledName(),
                                       GenericParams);
  }

  const TypeRef *visitTupleTypeRef(const TupleTypeRef *T) {
    std::vector<const TypeRef *> Elements;
    for (auto Element : T->getElements()) {
      Elements.push_back(visit(Element));
    }
    return TupleTypeRef::create(RC.Builder, Elements);
  }

  const TypeRef *visitFunctionTypeRef(const FunctionTypeRef *F) {
    std::vector<const TypeRef *> SubstitutedArguments;
    for (auto Argument : F->getArguments())
      SubstitutedArguments.push_back(visit(Argument));

    auto SubstitutedResult = visit(F->getResult());

    return FunctionTypeRef::create(RC.Builder, SubstitutedArguments,
                                   SubstitutedResult);
  }

  const TypeRef *visitProtocolTypeRef(const ProtocolTypeRef *P) {
    return P;
  }

  const TypeRef *
  visitProtocolCompositionTypeRef(const ProtocolCompositionTypeRef *PC) {
    return PC;
  }

  const TypeRef *visitMetatypeTypeRef(const MetatypeTypeRef *M) {
    return MetatypeTypeRef::create(RC.Builder, visit(M->getInstanceType()));
  }

  const TypeRef *
  visitExistentialMetatypeTypeRef(const ExistentialMetatypeTypeRef *EM) {
    assert(EM->getInstanceType()->isConcrete());
    return EM;
  }

  const TypeRef *
  visitGenericTypeParameterTypeRef(const GenericTypeParameterTypeRef *GTP) {
    auto found = Substitutions.find({GTP->getDepth(), GTP->getIndex()});
    assert(found != Substitutions.end());
    assert(found->second->isConcrete());
    return found->second;
  }

  const TypeRef *visitDependentMemberTypeRef(const DependentMemberTypeRef *DM) {
    auto SubstBase = visit(DM->getBase());

    const TypeRef *TypeWitness;

    switch (SubstBase->getKind()) {
    case TypeRefKind::Nominal: {
      auto Nominal = cast<NominalTypeRef>(SubstBase);
      TypeWitness = RC.getDependentMemberTypeRef(Nominal->getMangledName(), DM);
      break;
    }
    case TypeRefKind::BoundGeneric: {
      auto BG = cast<BoundGenericTypeRef>(SubstBase);
      TypeWitness = RC.getDependentMemberTypeRef(BG->getMangledName(), DM);
      break;
    }
    default:
      assert(false && "Unknown base type");
    }

    assert(TypeWitness);
    return TypeWitness->subst(RC, SubstBase->getSubstMap());
  }

  const TypeRef *visitForeignClassTypeRef(const ForeignClassTypeRef *F) {
    return F;
  }

  const TypeRef *visitObjCClassTypeRef(const ObjCClassTypeRef *OC) {
    return OC;
  }

  const TypeRef *visitUnownedStorageTypeRef(const UnownedStorageTypeRef *US) {
    return UnownedStorageTypeRef::create(RC.Builder, visit(US->getType()));
  }

  const TypeRef *visitWeakStorageTypeRef(const WeakStorageTypeRef *WS) {
    return WeakStorageTypeRef::create(RC.Builder, visit(WS->getType()));
  }

  const TypeRef *
  visitUnmanagedStorageTypeRef(const UnmanagedStorageTypeRef *US) {
    return UnmanagedStorageTypeRef::create(RC.Builder, visit(US->getType()));
  }

  const TypeRef *visitOpaqueTypeRef(const OpaqueTypeRef *Op) {
    return Op;
  }
};

template <typename Runtime>
const TypeRef *
TypeRef::subst(ReflectionContext<Runtime> &RC, GenericArgumentMap Subs) const {
  const TypeRef *Result = TypeRefSubstitution<Runtime>(RC, Subs).visit(this);
  assert(Result->isConcrete());
  return Result;
}

} // end namespace reflection
} // end namespace swift

#endif // SWIFT_REFLECTION_TYPEREF_H
