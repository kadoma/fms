/*
 * dev_interface.c
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

#include "ds_ata_ioctl.h"
#include "dev_interface.h"
#include "ds_impl.h"

//init ata_register
bool is_set(const ata_register_t *ata_register)
{       
	return ata_register->m_is_set;
}

void ata_registers(ata_register_t *ata_register)
{       
	ata_register->m_val = 0x00;
	ata_register->m_is_set = false;
}

void set_m_val(ata_register_t *ata_register, unsigned char val)
{       
	ata_register->m_val = val;
	ata_register->m_is_set = true;
}

unsigned char get_m_val(const ata_register_t *ata_register)
{
	return ata_register->m_val;
}

void ata_register_init(ata_register_t *ata_register)
{
	ata_register->is_set = is_set;
	ata_register->get_m_val = get_m_val;
	ata_register->ata_reg = ata_registers;
	ata_register->set_m_val = set_m_val;

	ata_register->ata_reg(ata_register);
}

//init ata_in_regs
bool in_is_set(const ata_in_regs_t *ata_in_regs)
{       
	return (ata_in_regs->features.is_set(&(ata_in_regs->features))
	     || ata_in_regs->sector_count.is_set(&(ata_in_regs->sector_count))
	     || ata_in_regs->lba_low.is_set(&(ata_in_regs->lba_low))
	     || ata_in_regs->lba_mid.is_set(&(ata_in_regs->lba_mid))
	     || ata_in_regs->lba_high.is_set(&(ata_in_regs->lba_high))
             || ata_in_regs->device.is_set(&(ata_in_regs->device))
             || ata_in_regs->command.is_set(&(ata_in_regs->command)));
}

void ata_in_regs_init(ata_in_regs_t *ata_in_regs)
{
	ata_register_init(&(ata_in_regs->features));
	ata_register_init(&(ata_in_regs->sector_count));
	ata_register_init(&(ata_in_regs->lba_low));
	ata_register_init(&(ata_in_regs->lba_mid));
	ata_register_init(&(ata_in_regs->lba_high));
	ata_register_init(&(ata_in_regs->device));
	ata_register_init(&(ata_in_regs->command));
	ata_in_regs->in_is_set = in_is_set;
}

//init ata_out_regs
bool out_is_set(const ata_out_regs_t *ata_out_regs)
{       
	return (ata_out_regs->error.is_set(&(ata_out_regs->error))
	     || ata_out_regs->sector_count.is_set(&(ata_out_regs->sector_count))
	     || ata_out_regs->lba_low.is_set(&(ata_out_regs->lba_low))
	     || ata_out_regs->lba_mid.is_set(&(ata_out_regs->lba_mid))
	     || ata_out_regs->lba_high.is_set(&(ata_out_regs->lba_high))
	     || ata_out_regs->device.is_set(&(ata_out_regs->device))
	     || ata_out_regs->status.is_set(&(ata_out_regs->status)));
}

void ata_out_regs_init(ata_out_regs_t *ata_out_regs)
{
	ata_register_init(&(ata_out_regs->error));
	ata_register_init(&(ata_out_regs->sector_count));
	ata_register_init(&(ata_out_regs->lba_low));
	ata_register_init(&(ata_out_regs->lba_mid));
	ata_register_init(&(ata_out_regs->lba_high));
	ata_register_init(&(ata_out_regs->device));
	ata_register_init(&(ata_out_regs->status));
	ata_out_regs->out_is_set = out_is_set;
}

//init ata_reg_alias_16
void reg_alias_16(ata_reg_alias_16_t *ata_reg_alias_16, ata_register_t lo, ata_register_t hi)
{       
	ata_reg_alias_16->m_lo = lo;
	ata_reg_alias_16->m_hi = hi;
}

///ata_reg_alias_16 & operator=(unsigned short val)
void set_alias_16_val(ata_reg_alias_16_t *ata_reg_alias_16, unsigned short val)
{       
//	ata_reg_alias_16->m_lo.m_val = (unsigned char) val;
//	ata_reg_alias_16->m_lo.m_is_set = true;
//	ata_reg_alias_16->m_hi.m_val = (unsigned char)(val >> 8);
//	ata_reg_alias_16->m_hi.m_is_set = true;
	unsigned char i = (unsigned char) val;
	ata_reg_alias_16->m_lo.set_m_val(&(ata_reg_alias_16->m_lo), i);
	unsigned char j = (unsigned char)(val >> 8);
	ata_reg_alias_16->m_hi.set_m_val(&(ata_reg_alias_16->m_hi), j);
//	printf("in.m_lo.m_val=%02x\n", ata_reg_alias_16->m_lo.m_val);
//	printf("in.m_hi.m_val=%02x\n", ata_reg_alias_16->m_hi.m_val);
}

unsigned short a_16_val(ata_reg_alias_16_t *ata_reg_alias_16)
{
	return ata_reg_alias_16->m_lo.m_val | (ata_reg_alias_16->m_hi.m_val << 8);
}  

void ata_reg_alias_16_init(ata_reg_alias_16_t *ata_reg_alias_16)
{
	ata_register_init(&(ata_reg_alias_16->m_lo));
	ata_register_init(&(ata_reg_alias_16->m_hi));
	ata_reg_alias_16->a_16_val = a_16_val;
	ata_reg_alias_16->reg_alias_16 = reg_alias_16;
	ata_reg_alias_16->set_alias_16_val = set_alias_16_val;
}

//init ata_in_regs_48bit
bool is_48bit_cmd(const ata_in_regs_48bit_t *ata_in_regs_48bit)
{
	return ata_in_regs_48bit->prev.in_is_set(&(ata_in_regs_48bit->prev));
}

bool is_real_48bit_cmd(ata_in_regs_48bit_t *ata_in_regs_48bit)
{
	return ( ata_in_regs_48bit->prev.features.is_set(&(ata_in_regs_48bit->prev.features))
	      || ata_in_regs_48bit->prev.sector_count.is_set(&(ata_in_regs_48bit->prev.sector_count))
	      || ata_in_regs_48bit->prev.lba_low.is_set(&(ata_in_regs_48bit->prev.lba_low))
	      || ata_in_regs_48bit->prev.lba_mid.is_set(&(ata_in_regs_48bit->prev.lba_mid))
	      || ata_in_regs_48bit->prev.lba_high.is_set(&(ata_in_regs_48bit->prev.lba_high)) );
}

void in_regs_48bit(ata_in_regs_48bit_t *ata_in_regs_48bit)
{
	ata_in_regs_48bit->features_16.reg_alias_16(&(ata_in_regs_48bit->features_16), ata_in_regs_48bit->in_48bit.features, ata_in_regs_48bit->prev.sector_count);
	ata_in_regs_48bit->sector_count_16.reg_alias_16(&(ata_in_regs_48bit->sector_count_16), ata_in_regs_48bit->in_48bit.sector_count, ata_in_regs_48bit->prev.sector_count);
	ata_in_regs_48bit->lba_low_16.reg_alias_16(&(ata_in_regs_48bit->lba_low_16), ata_in_regs_48bit->in_48bit.lba_low, ata_in_regs_48bit->prev.lba_low);
	ata_in_regs_48bit->lba_mid_16.reg_alias_16(&(ata_in_regs_48bit->lba_mid_16), ata_in_regs_48bit->in_48bit.lba_mid, ata_in_regs_48bit->prev.lba_mid);
	ata_in_regs_48bit->lba_high_16.reg_alias_16(&(ata_in_regs_48bit->lba_high_16), ata_in_regs_48bit->in_48bit.lba_high, ata_in_regs_48bit->prev.lba_high);
}

void ata_in_regs_48bit_init(ata_in_regs_48bit_t *ata_in_regs_48bit)
{
	ata_in_regs_init(&(ata_in_regs_48bit->in_48bit));
	ata_in_regs_init(&(ata_in_regs_48bit->prev));
	ata_reg_alias_16_init(&(ata_in_regs_48bit->features_16));
	ata_reg_alias_16_init(&(ata_in_regs_48bit->sector_count_16));
	ata_reg_alias_16_init(&(ata_in_regs_48bit->lba_low_16));
	ata_reg_alias_16_init(&(ata_in_regs_48bit->lba_mid_16));
	ata_reg_alias_16_init(&(ata_in_regs_48bit->lba_high_16));
	ata_in_regs_48bit->is_48bit_cmd = is_48bit_cmd;
	ata_in_regs_48bit->is_real_48bit_cmd = is_real_48bit_cmd;
	ata_in_regs_48bit->in_regs_48bit = in_regs_48bit;

	ata_in_regs_48bit->in_regs_48bit(ata_in_regs_48bit);
}

//init ata_out_regs_48bit
void out_regs_48bit(ata_out_regs_48bit_t *ata_out_regs_48bit)
{
	ata_out_regs_48bit->sector_count_16.reg_alias_16(&(ata_out_regs_48bit->sector_count_16), ata_out_regs_48bit->out_48bit.sector_count, ata_out_regs_48bit->prev.sector_count);
	ata_out_regs_48bit->lba_low_16.reg_alias_16(&(ata_out_regs_48bit->lba_low_16), ata_out_regs_48bit->out_48bit.lba_low, ata_out_regs_48bit->prev.lba_low);
	ata_out_regs_48bit->lba_mid_16.reg_alias_16(&(ata_out_regs_48bit->lba_mid_16), ata_out_regs_48bit->out_48bit.lba_mid, ata_out_regs_48bit->prev.lba_mid);
	ata_out_regs_48bit->lba_high_16.reg_alias_16(&(ata_out_regs_48bit->lba_high_16), ata_out_regs_48bit->out_48bit.lba_high, ata_out_regs_48bit->prev.lba_high);
}

void ata_out_regs_48bit_init(ata_out_regs_48bit_t *ata_out_regs_48bit)
{
	ata_out_regs_init(&(ata_out_regs_48bit->out_48bit));
	ata_out_regs_init(&(ata_out_regs_48bit->prev));
	ata_reg_alias_16_init(&(ata_out_regs_48bit->sector_count_16));
	ata_reg_alias_16_init(&(ata_out_regs_48bit->lba_low_16));
	ata_reg_alias_16_init(&(ata_out_regs_48bit->lba_mid_16));
	ata_reg_alias_16_init(&(ata_out_regs_48bit->lba_high_16));
	ata_out_regs_48bit->out_regs_48bit = out_regs_48bit;

	ata_out_regs_48bit->out_regs_48bit(ata_out_regs_48bit);
}

//init ata_out_regs_flags
bool f_is_set(const ata_out_regs_flags_t *ata_out_regs_flags)
{
	return ( ata_out_regs_flags->error    || ata_out_regs_flags->sector_count
	      || ata_out_regs_flags->lba_low  || ata_out_regs_flags->lba_mid
	      || ata_out_regs_flags->lba_high || ata_out_regs_flags->device
	      || ata_out_regs_flags->status );
}

void ata_out_regs_flag(ata_out_regs_flags_t *ata_out_regs_flags)
{
	ata_out_regs_flags->error = false;
	ata_out_regs_flags->sector_count = false;
	ata_out_regs_flags->lba_low = false;
	ata_out_regs_flags->lba_mid = false;
	ata_out_regs_flags->lba_high = false;
	ata_out_regs_flags->device = false;
	ata_out_regs_flags->status = false;
}

void ata_out_regs_flags_init(ata_out_regs_flags_t *ata_out_regs_flags)
{
	ata_out_regs_flags->f_is_set = f_is_set;
	ata_out_regs_flags->out_regs_flags = ata_out_regs_flag;

	ata_out_regs_flags->out_regs_flags(ata_out_regs_flags);
}

//init ata_cmd_in
void set_data_in(ata_cmd_in_t *ata_cmd_in, void *buf, unsigned nsectors)
{
	ata_cmd_in->buffer = buf;
//      ata_cmd_in->in_regs.in_48bit.sector_count = nsectors;
	ata_cmd_in->in_regs.in_48bit.sector_count.set_m_val(&(ata_cmd_in->in_regs.in_48bit.sector_count), nsectors);
	ata_cmd_in->direction = data_in;
	ata_cmd_in->size = nsectors * 512;
}

void set_data_out(ata_cmd_in_t *ata_cmd_in, void *buf, unsigned nsectors)
{
//      ata_cmd_in->buffer = const_cast<void *>(buf);
	ata_cmd_in->buffer = buf;
//      ata_cmd_in->in_regs.in_48bit.sector_count = nsectors;
	ata_cmd_in->in_regs.in_48bit.sector_count.set_m_val(&(ata_cmd_in->in_regs.in_48bit.sector_count), nsectors);
	ata_cmd_in->direction = data_out;
	ata_cmd_in->size = nsectors * 512;
}

void set_data_in_48bit(ata_cmd_in_t *ata_cmd_in, void *buf, unsigned nsectors)
{
	ata_cmd_in->buffer = buf;
	// Note: This also sets 'in_regs.is_48bit_cmd()'
//      ata_cmd_in->in_regs.sector_count_16 = nsectors;
	unsigned short i = (unsigned short)nsectors;
//	ata_cmd_in->in_regs.sector_count_16.set_alias_16_val(&(ata_cmd_in->in_regs.sector_count_16), nsectors);
	ata_cmd_in->in_regs.sector_count_16.set_alias_16_val(&(ata_cmd_in->in_regs.sector_count_16), i);
	ata_cmd_in->direction = data_in;
	ata_cmd_in->size = nsectors * 512;
}

void cmd_in(ata_cmd_in_t *ata_cmd_in)
{
	ata_cmd_in->direction = no_data;
	ata_cmd_in->buffer = 0;
	ata_cmd_in->size = 0;
}

void ata_cmd_in_init(ata_cmd_in_t *ata_cmd_in)
{
	ata_in_regs_48bit_init(&(ata_cmd_in->in_regs));
	ata_out_regs_flags_init(&(ata_cmd_in->out_needed));
	ata_cmd_in->set_data_in = set_data_in;
	ata_cmd_in->set_data_out = set_data_out;
	ata_cmd_in->set_data_in_48bit = set_data_in_48bit;
	ata_cmd_in->cmd_in = cmd_in;

	ata_cmd_in->cmd_in(ata_cmd_in);
}

//init ata_cmd_out
void ata_cmd_out_init(ata_cmd_out_t *ata_cmd_out)
{
	ata_out_regs_48bit_init(&(ata_cmd_out->out_regs));
}

//bool ata_device::ata_pass_through(const ata_cmd_in & in)
bool ata_pass_through_in(int fd, const ata_cmd_in_t *in)
{
	ata_cmd_out_t dummy;
	ata_cmd_out_init(&dummy);

	return ata_pass_through(fd, in, dummy);
}

//bool ata_device::ata_cmd_is_ok(const ata_cmd_in & in,
//  bool data_out_support /*= false*/,
//  bool multi_sector_support /*= false*/,
//  bool ata_48bit_support /*= false*/)
bool ata_cmd_is_ok(ata_cmd_in_t *in, bool data_out_support, bool multi_sector_support,
		    bool ata_48bit_support)
{
	// Check DATA IN/OUT
  	switch(in->direction) {
    		case no_data:
			break;
    		case data_in:
			break;
    		case data_out:
			break;
    		default:
      			return set_err("Invalid data direction %d", (int)in->direction);
  	}

  	// Check buffer size
  	if (in->direction == no_data) {
    		if (in->size)
      			return set_err("Buffer size %u > 0 for NO DATA command", in->size);
  	} else {
    		if (!in->buffer)
      			return set_err("Buffer not set for DATA IN/OUT command");
    		unsigned count = (in->in_regs.prev.sector_count.get_m_val(&(in->in_regs.prev.sector_count))<<16)|in->in_regs.in_48bit.sector_count.get_m_val(&(in->in_regs.in_48bit.sector_count));
    		// TODO: Add check for sector count == 0
    		if (count * 512 != in->size)
      			return set_err("Sector count %u does not match buffer size %u\n", count, in->size);
  	}

  	// Check features
  	if (in->direction == data_out && !data_out_support)
    		return set_err("DATA OUT ATA commands not supported");
  	if (!(in->size == 0 || in->size == 512) && !multi_sector_support)
    		return set_err("Multi-sector ATA commands not supported");
  	if (in->in_regs.is_48bit_cmd(&(in->in_regs)) && !ata_48bit_support)
    		return set_err("48-bit ATA commands not supported");

	return true;
}

bool ata_identify_is_cached(void)
{
	return false;
}

