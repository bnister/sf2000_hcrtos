/****************************************************************************
 * fs/vfs/fs_sendfile.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <generated/br2_autoconf.h>

#include <sys/sendfile.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <nuttx/errno.h>
#include <nuttx/fs/fs.h>

ssize_t sendfile(int outfd, int infd, off_t *offset, size_t count)
{
  FAR uint8_t *iobuffer;
  FAR uint8_t *wrbuffer;
  off_t startpos = 0;
  ssize_t nbytesread;
  ssize_t nbyteswritten;
  size_t  ntransferred;
  bool endxfr;

  /* Get the current file position. */

  if (offset)
    {
      off_t newpos;

      /* Use lseek to get the current file position */

      startpos = lseek(infd, 0, SEEK_CUR);
      if (startpos < 0)
        {
          return VFS_ERROR;
        }

      /* Use lseek again to set the new file position */

      newpos = lseek(infd, *offset, SEEK_SET);
      if (newpos < 0)
        {
          return VFS_ERROR;
        }
    }

  /* Allocate an I/O buffer */

  iobuffer = (FAR void *)malloc(CONFIG_LIB_SENDFILE_BUFSIZE);
  if (!iobuffer)
    {
      set_errno(ENOMEM);
      return VFS_ERROR;
    }

  /* Now transfer 'count' bytes from the infd to the outfd */

  for (ntransferred = 0, endxfr = false; ntransferred < count && !endxfr; )
    {
      /* Loop until the read side of the transfer comes to some conclusion */

      do
        {
          /* Read a buffer of data from the infd */

          nbytesread = read(infd, iobuffer, CONFIG_LIB_SENDFILE_BUFSIZE);

          /* Check for end of file */

          if (nbytesread == 0)
            {
              /* End of file.  Break out and return current number of bytes
               * transferred.
               */

              endxfr = true;
              break;
            }

          /* Check for a read ERROR.  EINTR is a special case.  This function
           * should break out and return an error if EINTR is returned and
           * no data has been transferred.  But what should it do if some
           * data has been transferred?  I suppose just continue?
           */

          else if (nbytesread < 0)
            {
              int errcode = get_errno();

              /* EINTR is not an error (but will still stop the copy) */

              if (errcode != EINTR || ntransferred == 0)
                {
                  /* Read error.  Break out and return the error condition. */

                  set_errno(errcode);
                  ntransferred = VFS_ERROR;
                  endxfr       = true;
                  break;
                }
            }
        }
      while (nbytesread < 0);

      /* Was anything read? */

      if (!endxfr)
        {
          /* Yes.. Loop until the read side of the transfer comes to some
           * conclusion.
           */

          wrbuffer = iobuffer;
          do
            {
              /* Write the buffer of data to the outfd */

              nbyteswritten = write(outfd, wrbuffer, nbytesread);

              /* Check for a complete (or parial) write.  write() should not
               * return zero.
               */

              if (nbyteswritten >= 0)
                {
                  /* Advance the buffer pointer and decrement the number of
                   * bytes remaining in the iobuffer.  Typically, nbytesread
                   * will now be zero.
                   */

                  wrbuffer     += nbyteswritten;
                  nbytesread   -= nbyteswritten;

                  /* Increment the total number of bytes successfully
                   * transferred.
                   */

                  ntransferred += nbyteswritten;
                }

              /* Otherwise an error occurred */

              else
                {
                  int errcode = get_errno();

                  /* Check for a read ERROR.  EINTR is a special case.  This
                   * function should break out and return an error if EINTR
                   * is returned and no data has been transferred.  But what
                   * should it do if some data has been transferred?  I
                   * suppose just continue?
                   */

                  if (errcode != EINTR || ntransferred == 0)
                    {
                      /* Write error.  Break out and return the error
                       * condition.
                       */

                      set_errno(errcode);
                      ntransferred = VFS_ERROR;
                      endxfr       = true;
                      break;
                    }
                }
            }
          while (nbytesread > 0);
        }
    }

  /* Release the I/O buffer */

  free(iobuffer);

  /* Return the current file position */

  if (offset)
    {
      /* Use lseek to get the current file position */

      off_t curpos = lseek(infd, 0, SEEK_CUR);
      if (curpos < 0)
        {
          return VFS_ERROR;
        }

      /* Return the current file position */

      *offset = curpos;

      /* Use lseek again to restore the original file position */

      startpos = lseek(infd, startpos, SEEK_SET);
      if (startpos < 0)
        {
          return VFS_ERROR;
        }
    }

  /* Finally return the number of bytes actually transferred (or ERROR
   * if any failure occurred).
   */

  return ntransferred;
}
