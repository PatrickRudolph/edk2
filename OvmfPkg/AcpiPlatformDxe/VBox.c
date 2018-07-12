/** @file
  OVMF ACPI VBox support

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AcpiPlatform.h"
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>

#include <OvmfPlatforms.h>

#define VBOX_ACPI_PHYSICAL_ADDRESS         0x000E0000
#define VBOX_BIOS_PHYSICAL_END             0x000FFFFF

EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *VBoxAcpiRsdpStructurePtr = NULL;

/**
  Get the address of VBox ACPI Root System Description Pointer (RSDP)
  structure.

  @param  RsdpStructurePtr   Return pointer to RSDP structure

  @return EFI_SUCCESS        Find VBox RSDP structure successfully.
  @return EFI_NOT_FOUND      Don't find VBox RSDP structure.
  @return EFI_ABORTED        Find VBox RSDP structure, but it's not integrated.

**/
EFI_STATUS
EFIAPI
GetVBoxAcpiRsdp (
  OUT   EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER   **RsdpPtr
  )
{
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER   *RsdpStructurePtr;
  UINT8                                          *VBoxAcpiPtr;
  UINT8                                          Sum;

  //
  // Detect the RSDP structure
  //
  for (VBoxAcpiPtr = (UINT8*)(UINTN) VBOX_ACPI_PHYSICAL_ADDRESS;
       VBoxAcpiPtr < (UINT8*)(UINTN) VBOX_BIOS_PHYSICAL_END;
       VBoxAcpiPtr += 0x10) {

    RsdpStructurePtr = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)
                         (UINTN) VBoxAcpiPtr;

    if (!AsciiStrnCmp ((CHAR8 *) &RsdpStructurePtr->Signature, "RSD PTR ", 8)) {
      //
      // RSDP ACPI 1.0 checksum for 1.0/2.0/3.0 table.
      // This is only the first 20 bytes of the structure
      //
      Sum = CalculateSum8 (
              (CONST UINT8 *)RsdpStructurePtr,
              sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER)
              );
      if (Sum != 0) {
        return EFI_ABORTED;
      }

      if (RsdpStructurePtr->Revision >= 2) {
        //
        // RSDP ACPI 2.0/3.0 checksum, this is the entire table
        //
        Sum = CalculateSum8 (
                (CONST UINT8 *)RsdpStructurePtr,
                sizeof (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER)
                );
        if (Sum != 0) {
          return EFI_ABORTED;
        }
      }
      *RsdpPtr = RsdpStructurePtr;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

# define EBDA_BASE          (0x9FC0 << 4)

VOID *
FindAcpiRsdPtr(VOID)
{
  UINTN Address;
  UINTN Index;

  //
  // First Search 0x0e0000 - 0x0fffff for RSD Ptr
  //
  for (Address = VBOX_ACPI_PHYSICAL_ADDRESS;
       Address < VBOX_BIOS_PHYSICAL_END;
       Address += 0x10) {
    if (!AsciiStrnCmp ((CHAR8 *) Address, "RSD PTR ", 8)) {
      return (VOID *)Address;
    }
  }

  //
  // Search EBDA
  //
  Address = EBDA_BASE;
  for (Index = 0; Index < 0x400 ; Index += 16) {
    if (!AsciiStrnCmp ((CHAR8 *) (Address + Index), "RSD PTR ", 8)) {
      return (VOID *)Address;
    }
  }
  return NULL;
}

VOID *FindSignature(VOID* Start, UINT32 Signature)
{
  UINT8 *Ptr = (UINT8*)Start;
  UINT32 Count = 0x10000; // 16 pages

  while (Count-- > 0) {
    if (   *(UINT32*)Ptr == Signature
        && ((EFI_ACPI_DESCRIPTION_HEADER *)Ptr)->Length <= Count
        && (CalculateCheckSum8(Ptr, ((EFI_ACPI_DESCRIPTION_HEADER *)Ptr)->Length) == 0
            )) {
      return Ptr;
    }

    Ptr++;
  }
  return NULL;
}

/**
  Get VBox Acpi tables from the RSDP structure. And installs VBox ACPI tables
  into the RSDT/XSDT using InstallAcpiTable. Some signature of the installed
  ACPI tables are: FACP, APIC, HPET, WAET, SSDT, FACS, DSDT.

  @param  AcpiProtocol           Protocol instance pointer.

  @return EFI_SUCCESS            The table was successfully inserted.
  @return EFI_INVALID_PARAMETER  Either AcpiTableBuffer is NULL, TableHandle is
                                 NULL, or AcpiTableBufferSize and the size
                                 field embedded in the ACPI table pointed to
                                 by AcpiTableBuffer are not in sync.
  @return EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the
                                 request.

**/
EFI_STATUS
EFIAPI
InstallVBoxTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
  )
{
  EFI_STATUS                                       Status;
  UINTN                                            TableHandle;

  EFI_ACPI_DESCRIPTION_HEADER                      *Rsdt;
  EFI_ACPI_DESCRIPTION_HEADER                      *Xsdt;
  VOID                                             *CurrentTableEntry;
  UINTN                                            CurrentTablePointer;
  EFI_ACPI_DESCRIPTION_HEADER                      *CurrentTable;
  UINTN                                            Index;
  UINTN                                            NumberOfTableEntries;
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE        *Fadt2Table;
  EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE        *Fadt1Table;
  EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE     *Facs2Table;
  EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE     *Facs1Table;
  EFI_ACPI_DESCRIPTION_HEADER                      *DsdtTable;
  EFI_ACPI_DESCRIPTION_HEADER                      *SsdtTable;
  EFI_ACPI_DESCRIPTION_HEADER                      *HpetTable;
  EFI_ACPI_DESCRIPTION_HEADER                      *McfgTable;
  EFI_ACPI_DESCRIPTION_HEADER                      *MadtTable;

  Fadt2Table  = NULL;
  Fadt1Table  = NULL;
  Facs2Table  = NULL;
  Facs1Table  = NULL;
  DsdtTable   = NULL;
  SsdtTable = NULL;
  HpetTable = NULL;
  McfgTable = NULL;
  MadtTable = NULL;
  TableHandle = 0;
  NumberOfTableEntries = 0;

  //
  // Try to find VBox ACPI tables
  //
  Status = GetVBoxAcpiRsdp (&VBoxAcpiRsdpStructurePtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "%a: Couldn't find RSDP pointer.\n", __FUNCTION__));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Found RSDP pointer.\n", __FUNCTION__));

  //
  // If XSDT table is find, just install its tables.
  // Otherwise, try to find and install the RSDT tables.
  //
  if (VBoxAcpiRsdpStructurePtr->XsdtAddress) {
    //
    // Retrieve the addresses of XSDT and
    // calculate the number of its table entries.
    //
    Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN)
             VBoxAcpiRsdpStructurePtr->XsdtAddress;
    NumberOfTableEntries = (Xsdt->Length -
                             sizeof (EFI_ACPI_DESCRIPTION_HEADER)) /
                             sizeof (UINT64);

    /* Look for optional tables */
    SsdtTable = FindSignature(Xsdt, SIGNATURE_32('S', 'S', 'D', 'T'));
    HpetTable = FindSignature(Xsdt, SIGNATURE_32('H', 'P', 'E', 'T'));
    McfgTable = FindSignature(Xsdt, SIGNATURE_32('M', 'C', 'F', 'G'));
    MadtTable = FindSignature(Xsdt, SIGNATURE_32('M', 'A', 'D', 'T'));

    //
    // Install ACPI tables found in XSDT.
    //
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      //
      // Get the table entry from XSDT
      //
      CurrentTableEntry = (VOID *) ((UINT8 *) Xsdt +
                            sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                            Index * sizeof (UINT64));
      CurrentTablePointer = (UINTN) *(UINT64 *)CurrentTableEntry;
      CurrentTable = (EFI_ACPI_DESCRIPTION_HEADER *) CurrentTablePointer;

      //
      // Install the XSDT tables
      //
      Status = InstallAcpiTable (
                 AcpiProtocol,
                 CurrentTable,
                 CurrentTable->Length,
                 &TableHandle
                 );

      if (EFI_ERROR (Status)) {
        return Status;
      }

      /* Overwrite table as it already has been installed through XSDT */
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "SSDT", 4))
        SsdtTable = NULL;
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "HPET", 4))
        HpetTable = NULL;
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "MCFG", 4))
        McfgTable = NULL;
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "MADT", 4))
        MadtTable = NULL;

      //
      // Get the FACS and DSDT table address from the table FADT
      //
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "FACP", 4)) {
        if (FindSignature((VOID *)(CurrentTablePointer + 32),
                                   SIGNATURE_32('F', 'A', 'C', 'P')))
          // we actually have 2 FADTs, see https://xtracker.innotek.de/index.php?bug=4082
           CurrentTablePointer = (UINTN)FindSignature(
                                   (VOID *)(CurrentTablePointer + 32),
                                   SIGNATURE_32('F', 'A', 'C', 'P'));
        Fadt2Table = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)
                       (UINTN) CurrentTablePointer;
        Facs2Table = (EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)
                       (UINTN) Fadt2Table->FirmwareCtrl;
        DsdtTable  = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Fadt2Table->Dsdt;
      }
    }
  } else if (VBoxAcpiRsdpStructurePtr->RsdtAddress) {
    //
    // Retrieve the addresses of RSDT and
    // calculate the number of its table entries.
    //
    Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN)
             VBoxAcpiRsdpStructurePtr->RsdtAddress;
    NumberOfTableEntries = (Rsdt->Length -
                             sizeof (EFI_ACPI_DESCRIPTION_HEADER)) /
                             sizeof (UINT32);

    /* Look for optional tables */
    SsdtTable = FindSignature(Rsdt, SIGNATURE_32('S', 'S', 'D', 'T'));
    HpetTable = FindSignature(Rsdt, SIGNATURE_32('H', 'P', 'E', 'T'));
    McfgTable = FindSignature(Rsdt, SIGNATURE_32('M', 'C', 'F', 'G'));
    MadtTable = FindSignature(Rsdt, SIGNATURE_32('M', 'A', 'D', 'T'));

    //
    // Install ACPI tables found in XSDT.
    //
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      //
      // Get the table entry from RSDT
      //
      CurrentTableEntry = (UINT32 *) ((UINT8 *) Rsdt +
                            sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                            Index * sizeof (UINT32));
      CurrentTablePointer = *(UINT32 *)CurrentTableEntry;
      CurrentTable = (EFI_ACPI_DESCRIPTION_HEADER *) CurrentTablePointer;

      //
      // Install the RSDT tables
      //
      Status = InstallAcpiTable (
                 AcpiProtocol,
                 CurrentTable,
                 CurrentTable->Length,
                 &TableHandle
                 );

      if (EFI_ERROR (Status)) {
        return Status;
      }

      /* Overwrite table as it already has been installed through RSDT */
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "SSDT", 4))
        SsdtTable = NULL;
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "HPET", 4))
        HpetTable = NULL;
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "MCFG", 4))
        McfgTable = NULL;
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "MADT", 4))
        MadtTable = NULL;

      //
      // Get the FACS and DSDT table address from the table FADT
      //
      if (!AsciiStrnCmp ((CHAR8 *) &CurrentTable->Signature, "FACP", 4)) {
        if (FindSignature((VOID *)(CurrentTablePointer + 32), SIGNATURE_32('F', 'A', 'C', 'P')))
          // we actually have 2 FADTs, see https://xtracker.innotek.de/index.php?bug=4082
          CurrentTablePointer = (UINTN)FindSignature(
                                 (VOID *)(CurrentTablePointer + 32),
                                 SIGNATURE_32('F', 'A', 'C', 'P'));
        Fadt1Table = (EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE *)
                       (UINTN) CurrentTablePointer;
        Facs1Table = (EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)
                       (UINTN) Fadt1Table->FirmwareCtrl;
        DsdtTable  = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Fadt1Table->Dsdt;
      }
    }
  }

  //
  // Install the FACS table.
  //
  if (Fadt2Table) {
    //
    // FACS 2.0
    //
    Status = InstallAcpiTable (
               AcpiProtocol,
               Facs2Table,
               Facs2Table->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  else if (Fadt1Table) {
    //
    // FACS 1.0
    //
    Status = InstallAcpiTable (
               AcpiProtocol,
               Facs1Table,
               Facs1Table->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Install DSDT table.
  //
  Status = InstallAcpiTable (
             AcpiProtocol,
             DsdtTable,
             DsdtTable->Length,
             &TableHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install optional HPET table.
  //
  if (HpetTable) {
    DEBUG ((DEBUG_INFO, "%a: Installing HPET table.\n", __FUNCTION__));
    Status = InstallAcpiTable (
               AcpiProtocol,
               HpetTable,
               HpetTable->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Install optional MCFG table.
  //
  if (McfgTable) {
    DEBUG ((DEBUG_INFO, "%a: Installing MCFG table.\n", __FUNCTION__));
    Status = InstallAcpiTable (
               AcpiProtocol,
               McfgTable,
               McfgTable->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Install optional SSDT table.
  //
  if (SsdtTable) {
    DEBUG ((DEBUG_INFO, "%a: Installing SSDT table.\n", __FUNCTION__));
    Status = InstallAcpiTable (
               AcpiProtocol,
               SsdtTable,
               SsdtTable->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Install optional MADT table.
  //
  if (MadtTable) {
    DEBUG ((DEBUG_INFO, "%a: Installing MADT table.\n", __FUNCTION__));
    Status = InstallAcpiTable (
               AcpiProtocol,
               MadtTable,
               MadtTable->Length,
               &TableHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  return EFI_SUCCESS;
}
