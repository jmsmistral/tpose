/* util.h: program-specific utility functions

   Copyright 2015 Jonathan Sacramento.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef TPOSE_UTIL_H
# define TPOSE_UTIL_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>


/* String containing name the program is called with.  */
extern const char *program_name;

/* Set program_name, based on argv[0].
   argv0 must be a string allocated with indefinite extent, and must not be
   modified after this call.  */
extern void set_program_name (const char *argv0);

#if ENABLE_RELOCATABLE
	/* Set program_name, based on argv[0], and original installation prefix and
		directory, for relocatability.  */
	extern void set_program_name_and_installdir (const char *argv0,
																const char *orig_installprefix,
																const char *orig_installdir);
	#undef set_program_name
	#define set_program_name(ARG0) \
	  set_program_name_and_installdir (ARG0, INSTALLPREFIX, INSTALLDIR)

	/* Return the full pathname of the current executable, based on the earlier
		call to set_program_name_and_installdir.  Return NULL if unknown.  */
	extern char *get_full_program_name (void);
#endif

#define STREQ(a, b) (*(a) == *(b) && strcmp((a), (b)) == 0)

int stringToInteger(char* string);

int close_stream(FILE *stream);
void close_stdout_set_file_name(const char *file);
void close_stdout_set_ignore_EPIPE(bool ignore);
void close_stdout(void);

#endif /* TPOSE_UTIL_H */
