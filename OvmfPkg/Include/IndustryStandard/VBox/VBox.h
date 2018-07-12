/** @file
  Copyright (c) 2018, 9elements Cyber Security GmbH

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.   The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __VBOX_VBOX_H__
#define __VBOX_VBOX_H__

#include <Library/PciLib.h>
#include <stdint.h>

//
// VGA Device Vendor ID (VID) and Device ID (DID) value for VBox
//
#define VBOX_VENDOR_ID          0x80ee
#define VBOX_VGA_DEVICE_ID      0xbeef
//
// B/D/F/Type: 0/2/0/PCI
//
#define VGA_REGISTER_VBOX(Offset) PCI_LIB_ADDRESS(0, 2, 0, (Offset))


//
// INTEL 82371AB ACPI (DID) value for VBox
//
#define INTEL_82371AB_ACPI_DEVICE_ID 0x7113

//
// B/D/F/Type: 0/7/0/PCI
//
#define POWER_MGMT_REGISTER_VBOX(Offset) PCI_LIB_ADDRESS(0, 7, 0, (Offset))

//
// INTEL 82801IR (ICH9R)  (DID) value for VBox
//
#define INTEL_82801IR_ACPI_DEVICE_ID 0x27B9

#define VBOX_PMBA 0x40
#define VBOX_PMBA_MASK                                                         \
  (BIT15 | BIT14 | BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6)

#define VBOX_ACPI_CNTL 0x80
#define VBOX_ACPI_CNTL_ACPI_EN BIT0

#endif
