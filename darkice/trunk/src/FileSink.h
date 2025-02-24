/*------------------------------------------------------------------------------

   Copyright (c) 2000-2007 Tyrell Corporation. All rights reserved.

   Tyrell DarkIce

   File     : FileSink.h
   Version  : $Revision$
   Author   : $Author$
   Location : $HeadURL$
   
   Copyright notice:

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License  
    as published by the Free Software Foundation; either version 3
    of the License, or (at your option) any later version.
   
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
    GNU General Public License for more details.
   
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

------------------------------------------------------------------------------*/
#ifndef FILE_SINK_H
#define FILE_SINK_H

#ifndef __cplusplus
#error This is a C++ include file
#endif


/* ============================================================ include files */

#include "Reporter.h"
#include "Sink.h"


/* ================================================================ constants */


/* =================================================================== macros */


/* =============================================================== data types */

/**
 *  File data output
 *
 *  @author  $Author$
 *  @version $Revision$
 */
class FileSink : public Sink, public virtual Reporter
{
    private:

        /**
         *  The name of the configuration related to
         *  this file sink. something like "file-0" or "file-2".
         */
        char      * configName;

        /**
         *  Name of the file represented by the FileSink.
         */
        char      * fileName;

        /**
         *  Initialize the object.
         *  
         *  @param configName the name of the configuration related to
         *         this file sink. something like "file-0" or "file-2".
         *  @param name name of the file to be represented by the object.
         *  @param addDate add a date to the filename.
         *  @param fileDateFormat optional date format of the added date
         *  @exception Exception
         */
        void
        init (  const char    * configName,
                const char    * name,
                const bool      addDate,
                const char    * fileDateFormat );

        /**
         *  De-initialize the object.
         *  
         *  @exception Exception
         */
        void
        strip ( void )                              ;

        /**
         *  Get the file name to where to move the data saved so far.
         *  Used in cut().
         *
         *  @return the file name where to move the data saved so far.
         *  @throws Exception on file operation errors
         */
        std::string
        getArchiveFileName( void )                  ;


    protected:

        /**
         *  Low-level file descriptor for the file represented by this object.
         */
        int         fileDescriptor;

        /**
         *  Actual filename f.e. when using fileAddDate.
         */
        char      * fileNameActual;

        /**
         *  Whether a date is added to the filename or not.
         */
        bool        addDate;

        /**
         *  Date format of the added date.
         */
        char      * fileDateFormat;

        /**

         *  Default constructor. Always throws an Exception.
         *  
         *  @exception Exception
         */
        inline
        FileSink ( void )                           
        {
            throw Exception( __FILE__, __LINE__);
        }


    public:

        /**
         *  Constructor by a file name.
         *
         *  @param configName the name of the configuration related to
         *         this file sink. something like "file-0" or "file-2".
         *  @param name name of the file to be represented by the object.
         *  @param addDate add a date to the filename.
         *  @param fileDateFormat optional date format of the added date
         *  @exception Exception
         */
        inline
        FileSink(   const char        * configName,
                    const char        * name,
                    const bool          addDate,
                    const char        * fileDateFormat )
        {
            init( configName, name, addDate, fileDateFormat );
        }

        /**
         *  Copy constructor.
         *
         *  @param fsink the FileSink to copy.
         *  @exception Exception
         */
        FileSink(   const FileSink &    fsink )     ;

        /**
         *  Destructor.
         *
         *  @exception Exception
         */
        inline virtual
        ~FileSink( void )                           
        {
            strip();
        }

        /**
         *  Assignment operator.
         *
         *  @param fs the FileSink to assign to this object.
         *  @return a reference to this object.
         *  @exception Exception
         */
        virtual FileSink &
        operator= ( const FileSink &    fs )        ;

        /**
         *  Get the file name this FileSink represents.
         *
         *  @return the file name this FileSink represents.
         */
        inline const char *
        getFileName ( void ) const                  throw ()
        {
            return fileName;
        }

        /**
         *  Check for the existence of the file this FileSink represents.
         *
         *  @return true if the file exists and is a regular file,
         *          false otherwise.
         */
        virtual bool
        exists ( void ) const                       throw ();

        /**
         *  Create the file.
         *
         *  @return true if creation was successful, false otherwise.
         *  @exception Exception
         */
        virtual bool
        create ( void )                             ;

        /**
         *  Open the file. Truncates the file.
         *
         *  @return true if opening was successful, false otherwise.
         *  @exception Exception
         */
        virtual bool
        open ( void )                               ;

        /**
         *  Check if the FileSink is open.
         *
         *  @return true if the FileSink is open, false otherwise.
         */
        inline virtual bool
        isOpen ( void ) const                       throw ()
        {
            return fileDescriptor != 0;
        }

        /**
         *  Check if the FileSink is ready to accept data.
         *  Blocks until the specified time for data to be available.
         *
         *  @param sec the maximum seconds to block.
         *  @param usec micro seconds to block after the full seconds.
         *  @return true if the Sink is ready to accept data, false otherwise.
         *  @exception Exception
         */
        virtual bool
        canWrite (     unsigned int    sec,
                       unsigned int    usec )       ;

        /**
         *  Write data to the FileSink.
         *
         *  @param buf the data to write.
         *  @param len number of bytes to write from buf.
         *  @return the number of bytes written (may be less than len).
         *  @exception Exception
         */
        virtual unsigned int
        write (        const void    * buf,
                       unsigned int    len )        ;

        /**
         *  This is a no-op in this FileSink.
         *
         *  @exception Exception
         */
        inline virtual void
        flush ( void )                              
        {
        }

        /**
         *  Cut what the sink has been doing so far, and start anew.
         *  This usually means separating the data sent to the sink up
         *  until now, and start saving a new chunk of data.
         */
        virtual void
        cut ( void )                                    throw ();

        /**
         *  Close the FileSink.
         *
         *  @exception Exception
         */
        virtual void
        close ( void )                              ;
};


/* ================================================= external data structures */


/* ====================================================== function prototypes */



#endif  /* FILE_SINK_H */

