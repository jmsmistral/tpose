/* util.h: program-specific utilities implementation

   Copyright 2015 Jonathan Sacramento.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
*/

#include "util.h"



static const char *file_name;

/* String containing name the program is called with.
   To be initialized by main().  */
const char *program_name = NULL;

/* Set program_name, based on argv[0].
   argv0 must be a string allocated with indefinite extent, and must not be
   modified after this call.  */
void
set_program_name (const char *argv0)
{

	/* libtool creates a temporary executable whose name is sometimes prefixed
		with "lt-" (depends on the platform).  It also makes argv[0] absolute.
		But the name of the temporary executable is a detail that should not be
		visible to the end user and to the test suite.
		Remove this "<dirname>/.libs/" or "<dirname>/.libs/lt-" prefix here.  */
	const char *slash;
	const char *base;

  /* Sanity check.  POSIX requires the invoking process to pass a non-NULL
     argv[0].  */
	if (argv0 == NULL)
	{
		/* It's a bug in the invoking program.  Help diagnosing it.  */
		fputs ("A NULL argv[0] was passed through an exec system call.\n",
		stderr);
		abort ();
	}

  slash = strrchr (argv0, '/');
  base = (slash != NULL ? slash + 1 : argv0);
  if (base - argv0 >= 7 && strncmp (base - 7, "/.libs/", 7) == 0)
    {
      argv0 = base;
      if (strncmp (base, "lt-", 3) == 0)
        {
          argv0 = base + 3;
          /* On glibc systems, remove the "lt-" prefix from the variable
             program_invocation_short_name.  */
#if HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME
          program_invocation_short_name = (char *) argv0;
#endif
        }
    }

  /* But don't strip off a leading <dirname>/ in general, because when the user
     runs
         /some/hidden/place/bin/cp foo foo
     he should get the error message
         /some/hidden/place/bin/cp: `foo' and `foo' are the same file
     not
         cp: `foo' and `foo' are the same file
   */

  program_name = argv0;

  /* On glibc systems, the error() function comes from libc and uses the
     variable program_invocation_name, not program_name.  So set this variable
     as well.  */
#if HAVE_DECL_PROGRAM_INVOCATION_NAME
  program_invocation_name = (char *) argv0;
#endif
}


int
close_stream (FILE *stream)
{
  /* const bool some_pending = (__fpending (stream) != 0); */
  const bool prev_fail = (ferror (stream) != 0);
  const bool fclose_fail = (fclose (stream) != 0);

  /* Return an error indication if there was a previous failure or if
     fclose failed, with one exception: ignore an fclose failure if
     there was no previous error, no data remains to be flushed, and
     fclose failed with EBADF.  That can happen when a program like cp
     is invoked like this 'cp a b >&-' (i.e., with standard output
     closed) and doesn't generate any output (hence no previous error
     and nothing to be flushed).  */

  if (prev_fail || (fclose_fail && (errno != EBADF)))
    {
      if (! fclose_fail)
        errno = 0;
      return EOF;
    }

  return 0;
}

/* Set the file name to be reported in the event an error is detected
   by close_stdout.  */
void
close_stdout_set_file_name (const char *file)
{
  file_name = file;
}


static bool ignore_EPIPE /* = false */;

void
close_stdout (void)
{
  if (close_stream (stdout) != 0
      && !(ignore_EPIPE && errno == EPIPE))
    {
      char const *write_error = "write error";
      if (file_name)
        fprintf (stderr, "%s: %s\n", file_name, write_error);
      else
        fprintf (stderr, "%s\n", write_error);

      exit(EXIT_FAILURE);
    }

   if (close_stream (stderr) != 0)
     exit(EXIT_FAILURE);
}



int stringToInteger
(
	char* string
) {

	char* tail;
	int val = 0;

	if((*string == '\0') || (string == NULL))
		return -1;

	errno = 0;

	val = strtol(string, &tail, 0);

	if(errno) {
		printf("Error - You have entered a very large value!\n");
		return -1; // Overflow
	}
	else {
		if(/*val == 0 &&*/ tail != NULL && *tail != '\0') {
			printf("tail = %s\n", tail);
			return -1;
		}
		else
			return val;
		//string = tail;
	}
		
}


