/** @file
  Detect VBox hvmloader SMBIOS data for usage by OVMF.

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmbiosPlatformDxe.h"
#include <Library/HobLib.h>

#define VBOX_SMBIOS_PHYSICAL_ADDRESS2      0x000E0000
#define VBOX_SMBIOS_PHYSICAL_ADDRESS       0x000F0000
#define VBOX_SMBIOS_PHYSICAL_END           0x000FFFFF

/**
  Validates the SMBIOS entry point structure

  @param  EntryPointStructure  SMBIOS entry point structure

  @retval TRUE   The entry point structure is valid
  @retval FALSE  The entry point structure is not valid

**/
STATIC
BOOLEAN
IsEntryPointStructureValid (
  IN SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure
  )
{
  UINTN                     Index;
  UINT8                     Length;
  UINT8                     Checksum;
  UINT8                     *BytePtr;

  BytePtr = (UINT8*) EntryPointStructure;
  Length = EntryPointStructure->EntryPointLength;
  Checksum = 0;

  for (Index = 0; Index < Length; Index++) {
    Checksum = Checksum + (UINT8) BytePtr[Index];
  }

  if (Checksum != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Locates the VBox SMBIOS data if it exists

  @return SMBIOS_TABLE_ENTRY_POINT   Address of VBox SMBIOS data

**/
SMBIOS_TABLE_ENTRY_POINT *
GetVBoxSmbiosTables (
  VOID
  )
{
  UINT8                     *VBoxSmbiosPtr;
  SMBIOS_TABLE_ENTRY_POINT  *VBoxSmbiosEntryPointStructure;
  UINT8                     *VBoxRanges[2] = {
                  (UINT8*)(UINTN) VBOX_SMBIOS_PHYSICAL_ADDRESS,
                  /* Reading memory at 0xE0000 is very slow ... */
                  (UINT8*)(UINTN) VBOX_SMBIOS_PHYSICAL_ADDRESS2 };

  for (UINT8 i = 0; i < ARRAY_SIZE(VBoxRanges); i++) {
    for (VBoxSmbiosPtr = VBoxRanges[i];
         VBoxSmbiosPtr < (UINT8*)(UINTN) VBOX_SMBIOS_PHYSICAL_END;
         VBoxSmbiosPtr += 0x10) {

      VBoxSmbiosEntryPointStructure = (SMBIOS_TABLE_ENTRY_POINT *) VBoxSmbiosPtr;

      if (!AsciiStrnCmp ((CHAR8 *) VBoxSmbiosEntryPointStructure->AnchorString, "_SM_", 4) &&
          !AsciiStrnCmp ((CHAR8 *) VBoxSmbiosEntryPointStructure->IntermediateAnchorString, "_DMI_", 5) &&
          IsEntryPointStructureValid (VBoxSmbiosEntryPointStructure)) {

        return VBoxSmbiosEntryPointStructure;

      }
    }
  }

  return NULL;
}
