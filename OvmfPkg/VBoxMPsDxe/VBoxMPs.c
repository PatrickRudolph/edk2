/* $Id$ */
/** @file
 * VBoxMPs.c - VirtualBox system tables
 */

/*
 * Copyright (C) 2009-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/VBoxLib/VBoxLib.h>

#include <IndustryStandard/I440FxPiix4.h>
#include <IndustryStandard/LegacyBiosMpTable.h>
#include <IndustryStandard/VBox/VBox.h>

#include <Protocol/DevicePathToText.h>

#include <Guid/Mps.h>

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/

#define SYS_TABLE_PAD(ptr) (((~ptr) +1) & 0x07 )
#define EFI_SYSTEM_TABLE_MAX_ADDRESS 0xFFFFFFFF


static
EFI_STATUS
ConvertMpsTable (
  IN OUT VOID          **Table
  )
/*++

Routine Description:

  Convert MP Table if the Location of the SMBios Table is lower than Addres 0x100000
  Assumption here:
   As in legacy Bios, MP table is required to place in E/F Seg,
   So here we just check if the range is E/F seg,
   and if Not, assume the Memory type is EfiACPIMemoryNVS/EfiRuntimeServicesData
Arguments:
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  UINT32                                       Data32;
  UINT32                                       FPLength;
  EFI_LEGACY_MP_TABLE_FLOATING_POINTER         *MpsFloatingPointerOri;
  EFI_LEGACY_MP_TABLE_FLOATING_POINTER         *MpsFloatingPointerNew;
  EFI_LEGACY_MP_TABLE_HEADER                   *MpsTableOri;
  EFI_LEGACY_MP_TABLE_HEADER                   *MpsTableNew;
  VOID                                         *OemTableOri;
  VOID                                         *OemTableNew;
  EFI_STATUS                                   Status;
  EFI_PHYSICAL_ADDRESS                         BufferPtr;

  //
  // Get MP configuration Table
  //
  MpsFloatingPointerOri = (EFI_LEGACY_MP_TABLE_FLOATING_POINTER *)(UINTN)(*(UINT64*)(*Table));
  if (!(((UINTN)MpsFloatingPointerOri <= 0x100000) &&
        ((UINTN)MpsFloatingPointerOri >= 0xF0000))){
    return EFI_SUCCESS;
  }
  //
  // Get Floating pointer structure length
  //
  FPLength = MpsFloatingPointerOri->Length * 16;
  Data32   = FPLength + SYS_TABLE_PAD (FPLength);
  MpsTableOri = (EFI_LEGACY_MP_TABLE_HEADER *)(UINTN)(MpsFloatingPointerOri->PhysicalAddress);
  if (MpsTableOri != NULL) {
    Data32 += MpsTableOri->BaseTableLength;
    Data32 += MpsTableOri->ExtendedTableLength;
    if (MpsTableOri->OemTablePointer != 0x00) {
      Data32 += SYS_TABLE_PAD (Data32);
      Data32 += MpsTableOri->OemTableSize;
    }
  } else {
    return EFI_SUCCESS;
  }
  //
  // Relocate memory
  //
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(Data32),
                  &BufferPtr
                  );
  ASSERT_EFI_ERROR (Status);
  MpsFloatingPointerNew = (EFI_LEGACY_MP_TABLE_FLOATING_POINTER *)(UINTN)BufferPtr;
  CopyMem (MpsFloatingPointerNew, MpsFloatingPointerOri, FPLength);
  //
  // If Mp Table exists
  //
  if (MpsTableOri != NULL) {
    //
    // Get Mps table length, including Ext table
    //
    BufferPtr = BufferPtr + FPLength + SYS_TABLE_PAD (FPLength);
    MpsTableNew = (EFI_LEGACY_MP_TABLE_HEADER *)(UINTN)BufferPtr;
    CopyMem (
      MpsTableNew,
      MpsTableOri,
      MpsTableOri->BaseTableLength + MpsTableOri->ExtendedTableLength
    );

    if ((MpsTableOri->OemTableSize != 0x0000) &&
         (MpsTableOri->OemTablePointer != 0x0000)
       ) {
        BufferPtr += MpsTableOri->BaseTableLength;
        BufferPtr += MpsTableOri->ExtendedTableLength;
        BufferPtr += SYS_TABLE_PAD (BufferPtr);
        OemTableNew = (VOID *)(UINTN)BufferPtr;
        OemTableOri = (VOID *)(UINTN)MpsTableOri->OemTablePointer;
        CopyMem (OemTableNew, OemTableOri, MpsTableOri->OemTableSize);
        MpsTableNew->OemTablePointer = (UINT32)(UINTN)OemTableNew;
    }
    MpsTableNew->Checksum = 0;
    MpsTableNew->Checksum = CalculateCheckSum8 (
                              (UINT8*)MpsTableNew,
                              MpsTableOri->BaseTableLength
                              );
    MpsFloatingPointerNew->PhysicalAddress = (UINT32)(UINTN)MpsTableNew;
    MpsFloatingPointerNew->Checksum = 0;
    MpsFloatingPointerNew->Checksum = CalculateCheckSum8 (
                                        (UINT8*)MpsFloatingPointerNew,
                                        FPLength
                                        );
  }
  //
  // Change the pointer
  //
  *Table = MpsFloatingPointerNew;

  return EFI_SUCCESS;
}

#define EBDA_BASE (0x9FC0 << 4)

VOID *
FindMPSPtr (
  VOID
  )
{
  UINTN                           Address;
  UINTN                           Index;

  //
  // First Search 0x0e0000 - 0x0fffff for MPS Ptr
  //
  for (Address = 0xe0000; Address < 0xfffff; Address += 0x10) {
    if (!AsciiStrnCmp((CHAR8 *)(Address), "_MP_", 4)) {
      return (VOID *)Address;
    }
  }

  //
  // Search EBDA
  //
  Address = EBDA_BASE;
  for (Index = 0; Index < 0x400 ; Index += 16) {
    if (!AsciiStrnCmp((CHAR8 *)(Address + Index), "_MP_", 4)) {
      return (VOID *)(Address + Index);
    }
  }
  return NULL;
}

/**
 * VBoxMPsDxe entry point.
 *
 * @returns EFI status code.
 *
 * @param   ImageHandle     The image handle.
 * @param   SystemTable     The system table pointer.
 */
EFI_STATUS EFIAPI
DxeInitializeVBoxMPs(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS  rc = EFI_SUCCESS;
    VOID*       Ptr;

    DEBUG((DEBUG_INFO, "DxeInitializeVBoxMPs\n"));

    if (!VBoxDetected()) {
      return EFI_UNSUPPORTED;
    }

    Ptr = FindMPSPtr();
    if (Ptr) {
      rc = ConvertMpsTable(&Ptr);
      ASSERT_EFI_ERROR(rc);
      rc = gBS->InstallConfigurationTable(&gEfiMpsTableGuid, Ptr);
      ASSERT_EFI_ERROR(rc);
    }

    return rc;
}

EFI_STATUS EFIAPI
DxeUninitializeVBoxMPs(IN EFI_HANDLE         ImageHandle)
{
    return EFI_SUCCESS;
}
