/** @file
  Provide basic VBox Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.   The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <OvmfPlatforms.h>
#include "VBoxLib.h"

/**
  This function detects if OVMF is running on VBox PIIX3.

**/
BOOLEAN
VBoxDetectedPiiX3 (
  VOID
  )
{
  if (PciRead16 (OVMF_HOSTBRIDGE_DID) == INTEL_82441_DEVICE_ID &&
      (PciRead16(VGA_REGISTER_VBOX(PCI_VENDOR_ID_OFFSET)) == VBOX_VENDOR_ID) &&
      (PciRead16(POWER_MGMT_REGISTER_VBOX(PCI_VENDOR_ID_OFFSET)) == 0x8086) &&
      (PciRead16(POWER_MGMT_REGISTER_VBOX(PCI_DEVICE_ID_OFFSET)) ==
       INTEL_82371AB_ACPI_DEVICE_ID)
     ) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function detects if OVMF is running on VBox.

**/
BOOLEAN
VBoxDetectedICH9 (
  VOID
  )
{
  if (PciRead16 (OVMF_HOSTBRIDGE_DID) != INTEL_82441_DEVICE_ID &&
      (PciRead16(VGA_REGISTER_VBOX(PCI_VENDOR_ID_OFFSET)) == VBOX_VENDOR_ID) &&
      (PciRead16(POWER_MGMT_REGISTER_Q35(PCI_VENDOR_ID_OFFSET)) == 0x8086) &&
      (PciRead16(POWER_MGMT_REGISTER_Q35(PCI_DEVICE_ID_OFFSET)) ==
       INTEL_82801IR_ACPI_DEVICE_ID)
     ) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function detects if OVMF is running on VBox.

**/
BOOLEAN
VBoxDetected (
  VOID
  )
{
  return VBoxDetectedPiiX3() || VBoxDetectedICH9();
}

/** The base of the I/O ports used for interaction between the EFI firmware and DevEFI. */
#define EFI_PORT_BASE           0xEF10  /**< @todo r=klaus stupid choice which
* causes trouble with PCI resource allocation in complex bridge setups,
* change to 0x0400 with appropriate saved state and reset handling */
/** The number of ports. */
#define EFI_PORT_COUNT          0x0008


/** Information querying.
 * 32-bit write sets the info index and resets the reading, see EfiInfoIndex.
 * 32-bit read returns the size of the info (in bytes).
 * 8-bit reads returns the info as a byte sequence. */
#define EFI_INFO_PORT           (EFI_PORT_BASE+0x0)

/**
  This function returns VBOX VM variables.

**/
UINT32
VBoxGetVmVariable(
  EfiInfoIndex Variable,
  CHAR8 *pbBuf,
  UINT32 cbBuf)
{
  UINT32 cbVar, offBuf;

  IoWrite32(EFI_INFO_PORT, Variable);
  cbVar = IoRead32(EFI_INFO_PORT);

  for (offBuf = 0; offBuf < cbVar && offBuf < cbBuf; offBuf++) {
    pbBuf[offBuf] = IoRead8(EFI_INFO_PORT);
  }

  return cbVar;
}

#define VBE_DISPI_IOPORT_DATA           0x01CF

/**
  Fix unaligned IO access
  @param  Port  : Port to write to
  @param  Value : value to write
**/
VOID
VBoxIoWrite16 (
  UINTN                           Port,
  UINT16                          Value
  )
{
  // The VBE port allows serialized and parallel unaligned access.
  // Can't use parallel access as unaligned IO is forbidden in EDK.
  if (Port == VBE_DISPI_IOPORT_DATA) {
   // Shift in data MSB first
    IoWrite8(Port, Value >> 8);
    IoWrite8(Port, Value & 0xff);
  } else {
    IoWrite16(Port, Value);
  }
}

/**
  Fix unaligned IO access
  @param  Port  : Port to write to
  @param  Value : value to write
**/
VOID
VBoxIoWrite32 (
  UINTN                           Port,
  UINT32                          Value
  )
{
  // The VBE port allows serialized and parallel unaligned access.
  // Can't use parallel access as unaligned IO is forbidden in EDK.
  if (Port == VBE_DISPI_IOPORT_DATA) {
   // Shift in data MSB first
    IoWrite8(Port, (Value >> 24) & 0xff);
    IoWrite8(Port, (Value >> 16) & 0xff);
    IoWrite8(Port, (Value >> 8) & 0xff);
    IoWrite8(Port, Value & 0xff);
  } else {
    IoWrite16(Port, Value);
  }
}

/**
  Fix unaligned IO access
  @param  Port  : Port to read from
**/
UINT16
VBoxIoRead16(
  UINTN                           Port
  )
{
  UINTN value;
  // The VBE port allows serialized and parallel unaligned access.
  // Can't use parallel access as unaligned IO is forbidden in EDK.
  if (Port == VBE_DISPI_IOPORT_DATA) {
    // Shift out data MSB first
    value = IoRead8(Port);
    value <<= 8;
    value |= IoRead8(Port);
    return value;
  } else {
    return IoRead16(Port);
  }
}

/**
  Fix unaligned IO access
  @param  Port  : Port to read from
**/
UINT32
VBoxIoRead32(
  UINTN                           Port
  )
{
  UINTN value;
  // The VBE port allows serialized and parallel unaligned access.
  // Can't use parallel access as unaligned IO is forbidden in EDK.
  if (Port == VBE_DISPI_IOPORT_DATA) {
    // Shift out data MSB first
    value = IoRead8(Port);
    value <<= 8;
    value |= IoRead8(Port);
    value <<= 8;
    value |= IoRead8(Port);
    value <<= 8;
    value |= IoRead8(Port);
    return value;
  } else {
    return IoRead32(Port);
  }
}
