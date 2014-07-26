/*
 * dev_interface.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * You should have received a copy of the GNU General Public License
 * (for example COPYING); If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DEV_INTERFACE_H
#define _DEV_INTERFACE_H

#include <stdarg.h>

// ATA specific interface

/// ATA register value and info whether is has been ever set
// (Automatically set by first assignment)
typedef struct ata_register ata_register_t;

struct ata_register {
	unsigned char m_val; ///< Register value
	bool m_is_set; ///< true if set

	bool (*is_set)(const ata_register_t *ata_register);
	unsigned char (*get_m_val)(const ata_register_t *ata_register);
	void (*ata_reg)(ata_register_t *ata_register);
	void (*set_m_val)(ata_register_t *ata_register, unsigned char val);
};

#if 0
class ata_register
{
public:
  ata_register()
    : m_val(0x00), m_is_set(false) { }

  ata_register & operator=(unsigned char val)
    { m_val = val; m_is_set = true; return * this; }

  unsigned char val() const
    { return m_val; }
  operator unsigned char() const
    { return m_val; }

  bool is_set() const
    { return m_is_set; }

private:
  unsigned char m_val; ///< Register value
  bool m_is_set; ///< true if set
};
#endif

/// ATA Input registers (for 28-bit commands)
typedef struct ata_in_regs ata_in_regs_t;

struct ata_in_regs {
	// ATA-6/7 register names    // ATA-3/4/5        // ATA-8
	ata_register_t features;     // features         // features
	ata_register_t sector_count; // sector count     // count
	ata_register_t lba_low;      // sector number    // ]
	ata_register_t lba_mid;      // cylinder low     // ] lba
	ata_register_t lba_high;     // cylinder high    // ]
	ata_register_t device;       // device/head      // device
	ata_register_t command;      // command          // command

	bool (*in_is_set)(const ata_in_regs_t *ata_in_regs);

//	// Return true if any register is set
//	bool is_set() const
//	{ return (features.is_set() || sector_count.is_set()
//      		|| lba_low.is_set() || lba_mid.is_set() || lba_high.is_set()
//      		|| device.is_set()  || command.is_set()); }
};

/// ATA Output registers (for 28-bit commands)
typedef struct ata_out_regs ata_out_regs_t;

struct ata_out_regs {
	ata_register_t error;
	ata_register_t sector_count;
  	ata_register_t lba_low;
  	ata_register_t lba_mid;
  	ata_register_t lba_high;
  	ata_register_t device;
  	ata_register_t status;

	bool (*out_is_set)(const ata_out_regs_t *ata_out_regs);

//  	/// Return true if any register is set
//  	bool is_set() const
//    	{ return (error.is_set() || sector_count.is_set()
//      		|| lba_low.is_set() || lba_mid.is_set() || lba_high.is_set()
//      		|| device.is_set()  || status.is_set()); }
};

/// 16-bit alias to a 8-bit ATA register pair.
typedef struct ata_reg_alias_16 ata_reg_alias_16_t;

struct ata_reg_alias_16 {
	ata_register_t m_lo;
	ata_register_t m_hi;

	unsigned short (*a_16_val)(ata_reg_alias_16_t *ata_reg_alias_16);
	void (*reg_alias_16)(ata_reg_alias_16_t *ata_reg_alias_16, ata_register_t lo, ata_register_t hi);
	void (*set_alias_16_val)(ata_reg_alias_16_t *ata_reg_alias_16, unsigned short val);
};

#if 0
class ata_reg_alias_16
{
public:
  ata_reg_alias_16(ata_register & lo, ata_register & hi)
    : m_lo(lo), m_hi(hi) { }

  ata_reg_alias_16 & operator=(unsigned short val)
    { m_lo = (unsigned char) val;
      m_hi = (unsigned char)(val >> 8);
      return * this;                   }

  unsigned short val() const
    { return m_lo | (m_hi << 8); }
  operator unsigned short() const
    { return m_lo | (m_hi << 8); }

private:
  ata_register & m_lo, & m_hi;

  // References must not be copied.
  ata_reg_alias_16(const ata_reg_alias_16 &);
  void operator=(const ata_reg_alias_16 &);
};
#endif

/// ATA Input registers for 48-bit commands
// See section 4.14 of T13/1532D Volume 1 Revision 4b
//
// Uses ATA-6/7 method to specify 16-bit registers as
// recent (low byte) and previous (high byte) content of
// 8-bit registers.
//
// (ATA-8 ACS does not longer follow this scheme, it uses
// abstract registers with sufficient size and leaves the
// actual mapping to the transport layer.)
//
typedef struct ata_in_regs_48bit ata_in_regs_48bit_t;

struct ata_in_regs_48bit {
	ata_in_regs_t in_48bit;	// public elem
	ata_in_regs_t prev;	///< "previous content"

	// 16-bit aliases for above pair.
	ata_reg_alias_16_t features_16;
	ata_reg_alias_16_t sector_count_16;
	ata_reg_alias_16_t lba_low_16;
	ata_reg_alias_16_t lba_mid_16;
	ata_reg_alias_16_t lba_high_16;

	/// Return true if 48-bit command
	bool (*is_48bit_cmd)(const ata_in_regs_48bit_t *ata_in_regs_48bit);

	/// Return true if 48-bit command with any nonzero high byte
	bool (*is_real_48bit_cmd)(ata_in_regs_48bit_t *ata_in_regs_48bit);

	/// Init struct
	void (*in_regs_48bit)(ata_in_regs_48bit_t *ata_in_regs_48bit);
};

#if 0
struct ata_in_regs_48bit
: public ata_in_regs   // "most recently written" registers
{
  ata_in_regs prev;  ///< "previous content"

  // 16-bit aliases for above pair.
  ata_reg_alias_16 features_16;
  ata_reg_alias_16 sector_count_16;
  ata_reg_alias_16 lba_low_16;
  ata_reg_alias_16 lba_mid_16;
  ata_reg_alias_16 lba_high_16;

  /// Return true if 48-bit command
  bool is_48bit_cmd() const
    { return prev.is_set(); }

  /// Return true if 48-bit command with any nonzero high byte
  bool is_real_48bit_cmd() const
    { return (   prev.features || prev.sector_count
              || prev.lba_low || prev.lba_mid || prev.lba_high); }

  ata_in_regs_48bit();
};
#endif

/// ATA Output registers for 48-bit commands
typedef struct ata_out_regs_48bit ata_out_regs_48bit_t;

struct ata_out_regs_48bit {
	ata_out_regs_t out_48bit;	//public elem
	ata_out_regs_t prev;     	///< read with HOB=1

	// 16-bit aliases for above pair.
	ata_reg_alias_16_t sector_count_16;
	ata_reg_alias_16_t lba_low_16;
	ata_reg_alias_16_t lba_mid_16;
	ata_reg_alias_16_t lba_high_16;

	/// Init struct
	void (*out_regs_48bit)(ata_out_regs_48bit_t *ata_out_regs_48bit);
};

#if 0
struct ata_out_regs_48bit
: public ata_out_regs   // read with HOB=0
{
  ata_out_regs prev;  ///< read with HOB=1

  // 16-bit aliases for above pair.
  ata_reg_alias_16 sector_count_16;
  ata_reg_alias_16 lba_low_16;
  ata_reg_alias_16 lba_mid_16;
  ata_reg_alias_16 lba_high_16;

  ata_out_regs_48bit();
};
#endif

/// Flags for each ATA output register
typedef struct ata_out_regs_flags ata_out_regs_flags_t;

struct ata_out_regs_flags {
	bool error;
	bool sector_count;
	bool lba_low;
	bool lba_mid;
	bool lba_high;
	bool device;
	bool status;

	/// Return true if any flag is set.
	bool (*f_is_set)(const ata_out_regs_flags_t *ata_out_regs_flags);

	/// Default constructor clears all flags.
	void (*out_regs_flags)(ata_out_regs_flags_t *ata_out_regs_flags);
};

#if 0
struct ata_out_regs_flags
{
  bool error, sector_count, lba_low, lba_mid, lba_high, device, status;

  /// Return true if any flag is set.
  bool is_set() const
    { return (   error || sector_count || lba_low
              || lba_mid || lba_high || device || status); }

  /// Default constructor clears all flags.
  ata_out_regs_flags()
    : error(false), sector_count(false), lba_low(false), lba_mid(false),
      lba_high(false), device(false), status(false) { }
};
#endif

enum {
	no_data = 0,
	data_in,
	data_out
};

/// ATA pass through input parameters
typedef struct ata_cmd_in ata_cmd_in_t;

struct ata_cmd_in {
	ata_in_regs_48bit_t	in_regs;	///< Input registers
	ata_out_regs_flags_t 	out_needed; 	///< True if output register value needed
/*
	enum {
		no_data = 0,
		data_in,
		data_out
	} direction; 				///< I/O direction
*/
	int direction;                          ///< I/O direction
	void *buffer; 				///< Pointer to data buffer
	unsigned size; 				///< Size of buffer

	/// Prepare for 28-bit DATA IN command
	void (*set_data_in)(ata_cmd_in_t *ata_cmd_in, void *buf, unsigned nsectors);

	/// Prepare for 28-bit DATA OUT command
	void (*set_data_out)(ata_cmd_in_t *ata_cmd_in, void *buf, unsigned nsectors);

	/// Prepare for 48-bit DATA IN command
	void (*set_data_in_48bit)(ata_cmd_in_t *ata_cmd_in, void *buf, unsigned nsectors);

	/// Init struct
	void (*cmd_in)(ata_cmd_in_t *ata_cmd_in);
};

#if 0
struct ata_cmd_in
{
  ata_in_regs_48bit in_regs;  ///< Input registers
  ata_out_regs_flags out_needed; ///< True if output register value needed
  enum { no_data = 0, data_in, data_out } direction; ///< I/O direction
  void * buffer; ///< Pointer to data buffer
  unsigned size; ///< Size of buffer

  /// Prepare for 28-bit DATA IN command
  void set_data_in(void * buf, unsigned nsectors)
    {
      buffer = buf;
      in_regs.sector_count = nsectors;
      direction = data_in;
      size = nsectors * 512;
    }

  /// Prepare for 28-bit DATA OUT command
  void set_data_out(const void * buf, unsigned nsectors)
    {
      buffer = const_cast<void *>(buf);
      in_regs.sector_count = nsectors;
      direction = data_out;
      size = nsectors * 512;
    }

  /// Prepare for 48-bit DATA IN command
  void set_data_in_48bit(void * buf, unsigned nsectors)
    {
      buffer = buf;
      // Note: This also sets 'in_regs.is_48bit_cmd()'
      in_regs.sector_count_16 = nsectors;
      direction = data_in;
      size = nsectors * 512;
    }

  ata_cmd_in();
};
#endif

/// ATA pass through output parameters
typedef struct ata_cmd_out ata_cmd_out_t;

struct ata_cmd_out {
	ata_out_regs_48bit_t out_regs; ///< Output registers
};

#if 0
struct ata_cmd_out
{
  ata_out_regs_48bit out_regs; ///< Output registers

  ata_cmd_out();
};
#endif

/////////////////////////////////////////////////////////////////////////////
void ata_register_init(ata_register_t *ata_register);
void ata_in_regs_init(ata_in_regs_t *ata_in_regs);
void ata_out_regs_init(ata_out_regs_t *ata_out_regs);
void ata_reg_alias_16_init(ata_reg_alias_16_t *ata_reg_alias_16);
void ata_in_regs_48bit_init(ata_in_regs_48bit_t *ata_in_regs_48bit);
void ata_out_regs_48bit_init(ata_out_regs_48bit_t *ata_out_regs_48bit);
void ata_out_regs_flags_init(ata_out_regs_flags_t *ata_out_regs_flags);
void ata_cmd_in_init(ata_cmd_in_t *ata_cmd_in);
void ata_cmd_out_init(ata_cmd_out_t *ata_cmd_out);

bool ata_pass_through_in(int fd, const ata_cmd_in_t *in);

#endif  /* _DEV_INTERFACE_H  */
