/** @file
  Internal definitions for ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.   The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _VBOX_LIB_INTERNAL_H_
#define _VBOX_LIB_INTERNAL_H_

/**
 * Information requests.
 */
typedef enum
{
    EFI_INFO_INDEX_INVALID = 0,
    EFI_INFO_INDEX_VOLUME_BASE,
    EFI_INFO_INDEX_VOLUME_SIZE,
    EFI_INFO_INDEX_TEMPMEM_BASE,
    EFI_INFO_INDEX_TEMPMEM_SIZE,
    EFI_INFO_INDEX_STACK_BASE,
    EFI_INFO_INDEX_STACK_SIZE,
    EFI_INFO_INDEX_BOOT_ARGS,
    EFI_INFO_INDEX_DEVICE_PROPS,
    EFI_INFO_INDEX_FSB_FREQUENCY,
    EFI_INFO_INDEX_CPU_FREQUENCY,
    EFI_INFO_INDEX_TSC_FREQUENCY,
    EFI_INFO_INDEX_GRAPHICS_MODE,
    EFI_INFO_INDEX_HORIZONTAL_RESOLUTION,
    EFI_INFO_INDEX_VERTICAL_RESOLUTION,
    EFI_INFO_INDEX_MCFG_BASE,
    EFI_INFO_INDEX_MCFG_SIZE,
    EFI_INFO_INDEX_END
} EfiInfoIndex;

/**
  This function detects if OVMF is running on VBox PIIX3.

**/
BOOLEAN
VBoxDetectedPiiX3 (
  VOID
  );

/**
  This function detects if OVMF is running on VBox.

**/
BOOLEAN
VBoxDetectedICH9 (
  VOID
  );

/**
  This function detects if OVMF is running on VBox.

**/
BOOLEAN
VBoxDetected (
  VOID
  );

/**
  This function returns VBOX VM variables.

**/
UINT32
VBoxGetVmVariable(
  EfiInfoIndex Variable,
  CHAR8 *pbBuf,
  UINT32 cbBuf);

/**
  Fix unaligned IO access
  @param  Port  : Port to write to
  @param  Value : value to write
**/
VOID
VBoxIoWrite16 (
  UINTN                           Port,
  UINT16                          Value
  );

/**
  Fix unaligned IO access
  @param  Port  : Port to write to
  @param  Value : value to write
**/
VOID
VBoxIoWrite32 (
  UINTN                           Port,
  UINT32                          Value
  );

/**
  Fix unaligned IO access
  @param  Port  : Port to read from
**/
UINT16
VBoxIoRead16(
  UINTN                           Port
  );

/**
  Fix unaligned IO access
  @param  Port  : Port to read from
**/
UINT32
VBoxIoRead32(
  UINTN                           Port
  );
#endif // _VBOX_LIB_INTERNAL_H_
