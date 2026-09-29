/* ARM PadOS target support.

   Copyright (C) 2026 Kurt Skauen.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "arm-tdep.h"
#include "gdb_bfd.h"
#include "objfiles.h"
#include "osabi.h"

/* qGetTLSAddr load-module IDs.  The protocol is defined in PadOS's
   Include/Kernel/KDebugger.h.  These identify the variable's image,
   independently of the selected thread's current execution mode.  */

enum class arm_pados_tls_module
{
  unknown = 0,
  kernel = 1,
  userspace = 2
};

/* Identify a PadOS image using its linker-visible metadata.  The kernel
   image can also contain a non-allocated __app_definition placeholder;
   only definitions in allocated sections identify an image's domain.  */

static arm_pados_tls_module
arm_pados_tls_module_for_bfd (bfd *abfd)
{
  arm_pados_tls_module module = arm_pados_tls_module::unknown;

  for (const asymbol *symbol : gdb_bfd_canonicalize_symtab (abfd, false))
    {
      if ((bfd_section_flags (symbol->section) & SEC_ALLOC) == 0)
	continue;

      const char *name = bfd_asymbol_name (symbol);
      if (strcmp (name, "_kernel_debugger_info") == 0)
	return arm_pados_tls_module::kernel;
      if (strcmp (name, "__app_definition") == 0)
	module = arm_pados_tls_module::userspace;
    }

  return module;
}

/* Return the qGetTLSAddr load-module ID for OBJFILE.  Look for the image
   metadata in both the original image and its separate debug objfiles.  */

static CORE_ADDR
arm_pados_fetch_tls_load_module_address (struct objfile *objfile)
{
  for (struct objfile *image : objfile->separate_debug_objfiles ())
    {
      arm_pados_tls_module module
	= arm_pados_tls_module_for_bfd (image->obfd.get ());
      if (module != arm_pados_tls_module::unknown)
	return static_cast<CORE_ADDR> (module);
    }

  throw_error (TLS_GENERIC_ERROR,
	       _("Cannot identify the PadOS TLS domain of %s."),
	       objfile_name (objfile));
}

/* Initialize the PadOS ABI, retaining ARM bare-metal core-file support.  */

static void
arm_pados_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  arm_none_init_abi (info, gdbarch);
  set_gdbarch_fetch_tls_load_module_address
    (gdbarch, arm_pados_fetch_tls_load_module_address);
}

/* Recognize the kernel or user-space image before minimal symbols have
   been read.  PadOS uses ordinary ELF files without a dedicated EI_OSABI.  */

static enum gdb_osabi
arm_pados_osabi_sniffer (bfd *abfd)
{
  if (arm_pados_tls_module_for_bfd (abfd) != arm_pados_tls_module::unknown)
    return GDB_OSABI_PADOS;
  return GDB_OSABI_UNKNOWN;
}

INIT_GDB_FILE (arm_pados_tdep)
{
  gdbarch_register_osabi_sniffer (bfd_arch_arm, bfd_target_elf_flavour,
				  arm_pados_osabi_sniffer);
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_PADOS,
			  arm_pados_init_abi);
}
