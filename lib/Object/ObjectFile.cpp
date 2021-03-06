//===- ObjectFile.cpp - File format independent object file -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a file format independent ObjectFile class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/COFF.h"
#include "llvm/Object/MachO.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include <system_error>

using namespace llvm;
using namespace object;

void ObjectFile::anchor() { }

ObjectFile::ObjectFile(unsigned int Type, MemoryBufferRef Source)
    : SymbolicFile(Type, Source) {}

bool SectionRef::containsSymbol(SymbolRef S) const {
  ErrorOr<section_iterator> SymSec = S.getSection();
  if (!SymSec)
    return false;
  return *this == **SymSec;
}

uint64_t ObjectFile::getSymbolValue(DataRefImpl Ref) const {
  uint32_t Flags = getSymbolFlags(Ref);
  if (Flags & SymbolRef::SF_Undefined)
    return 0;
  if (Flags & SymbolRef::SF_Common)
    return getCommonSymbolSize(Ref);
  return getSymbolValueImpl(Ref);
}

std::error_code ObjectFile::printSymbolName(raw_ostream &OS,
                                            DataRefImpl Symb) const {
  ErrorOr<StringRef> Name = getSymbolName(Symb);
  if (std::error_code EC = Name.getError())
    return EC;
  OS << *Name;
  return std::error_code();
}

uint32_t ObjectFile::getSymbolAlignment(DataRefImpl DRI) const { return 0; }

bool ObjectFile::isSectionBitcode(DataRefImpl Sec) const {
  StringRef SectName;
  if (!getSectionName(Sec, SectName))
    return SectName == ".llvmbc";
  return false;
}

section_iterator ObjectFile::getRelocatedSection(DataRefImpl Sec) const {
  return section_iterator(SectionRef(Sec, this));
}

Expected<std::unique_ptr<ObjectFile>>
ObjectFile::createObjectFile(MemoryBufferRef Object, sys::fs::file_magic Type) {
  StringRef Data = Object.getBuffer();
  if (Type == sys::fs::file_magic::unknown)
    Type = sys::fs::identify_magic(Data);

  switch (Type) {
  case sys::fs::file_magic::unknown:
  case sys::fs::file_magic::bitcode:
  case sys::fs::file_magic::archive:
  case sys::fs::file_magic::macho_universal_binary:
  case sys::fs::file_magic::windows_resource:
    return errorCodeToError(object_error::invalid_file_type);
  case sys::fs::file_magic::elf:
  case sys::fs::file_magic::elf_relocatable:
  case sys::fs::file_magic::elf_executable:
  case sys::fs::file_magic::elf_shared_object:
  case sys::fs::file_magic::elf_core:
    return errorOrToExpected(createELFObjectFile(Object));
  case sys::fs::file_magic::macho_object:
  case sys::fs::file_magic::macho_executable:
  case sys::fs::file_magic::macho_fixed_virtual_memory_shared_lib:
  case sys::fs::file_magic::macho_core:
  case sys::fs::file_magic::macho_preload_executable:
  case sys::fs::file_magic::macho_dynamically_linked_shared_lib:
  case sys::fs::file_magic::macho_dynamic_linker:
  case sys::fs::file_magic::macho_bundle:
  case sys::fs::file_magic::macho_dynamically_linked_shared_lib_stub:
  case sys::fs::file_magic::macho_dsym_companion:
  case sys::fs::file_magic::macho_kext_bundle:
    return createMachOObjectFile(Object);
  case sys::fs::file_magic::coff_object:
  case sys::fs::file_magic::coff_import_library:
  case sys::fs::file_magic::pecoff_executable:
    return errorOrToExpected(createCOFFObjectFile(Object));
  }
  llvm_unreachable("Unexpected Object File Type");
}

Expected<OwningBinary<ObjectFile>>
ObjectFile::createObjectFile(StringRef ObjectPath) {
  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
      MemoryBuffer::getFile(ObjectPath);
  if (std::error_code EC = FileOrErr.getError())
    return errorCodeToError(EC);
  std::unique_ptr<MemoryBuffer> Buffer = std::move(FileOrErr.get());

  Expected<std::unique_ptr<ObjectFile>> ObjOrErr =
      createObjectFile(Buffer->getMemBufferRef());
  if (!ObjOrErr)
    ObjOrErr.takeError();
  std::unique_ptr<ObjectFile> Obj = std::move(ObjOrErr.get());

  return OwningBinary<ObjectFile>(std::move(Obj), std::move(Buffer));
}
