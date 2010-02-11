/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2010 Secure Endpoints Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* $Id$ */

#ifndef __KHIMAIRA_KCONFIG_H
#define __KHIMAIRA_KCONFIG_H

#include<windows.h>
#include "khdefs.h"
#include "mstring.h"

BEGIN_C

/*! \defgroup kconf NetIDMgr Configuration Provider */
/*@{*/

/*! \brief Configuration schema descriptor record 

    The schema descriptor is a convenient way to provide a default set
    of configuration options for a part of an application.  It
    describes the configuration spaces and the values and subspaces
    contained in each space.

    \see kconf_load_schema()
*/
typedef struct tag_kconf_schema {
    wchar_t *   name;       /*!< name of the object being described.
                                Optional for KC_ENDSPACE type object,
                                but required for everything else.
                                Names can be upto KCONF_MAXCCH_NAME
                                characters in length. */
    khm_int32   type;       /*!< type of the object.  Can be one of
                                KC_SPACE, KC_ENDSPACE, KC_INT32,
                                KC_INT64, KC_STRING or KC_BINARY */
    khm_ui_8    value;      /*!< the value of the object.  It is not
                                used for KC_SPACE and KC_ENDSPACE
                                typed objects.  For a KC_STRING, this
                                contains a pointer to the string
                                value.  The string should not be
                                longer than KCONF_MAXCCH_STRING
                                characters. KC_INT32 and KC_INT64
                                objects store the value directly in
                                this field, while KC_BINARY objects do
                                not support defining a default value
                                here. */
    wchar_t *   description;/*!< a friendly description of the value
                                or configuration space. */
} kconf_schema;

/*! \name Configuration data types
  @{*/
/*! \brief Not a known type */
#define KC_NONE         0

/*! \brief When used as ::kconf_schema \a type, defines the start of a configuration space.

    There should be a subsequent KC_ENDSPACE record in the schema
    which defines the end of this configuration space.

    \a name specifies the name of the configuration space.  Optionally
    use \a description to provide a description.*/
#define KC_SPACE        1

/*! \brief Ends a configuration space started with KC_SPACE */
#define KC_ENDSPACE     2

/*! \brief A 32 bit integer

    Specifies a configuration parameter named \a name which is of this
    type.  Use \a description to provide an optional description of
    the value.

    \a value specifies a default value for this parameter in the lower
    32 bits.
*/
#define KC_INT32        3

/*! \brief A 64 bit integer 

    Specifies a configuration parameter named \a name which is of this
    type.  Use \a description to provide an optional description of
    the value.

    \a value specifies a default value for this parameter.
*/
#define KC_INT64        4

/*! \brief A unicode string 

    Specifies a configuration parameter named \a name which is of this
    type.  Use \a description to provide an optional description of
    the value.

    \a value specifies a default value for this parameter which should
    be a pointer to a NULL terminated unicode string of no more than
    ::KCONF_MAXCCH_STRING characters.
*/
#define KC_STRING       5

/*! \brief An unparsed binary stream 

    Specifies a configuration parameter named \a name which is of this
    type.  Use \a description to provide an optional description of
    the value.

    Default values are not supported for binary streams.  \a value is
    ignored.
*/
#define KC_BINARY       6

/*! \internal
  \brief Modified time
 */
#define KC_MTIME        256
/*@}*/

/*! \brief This is the root configuration space */
#define KCONF_FLAG_ROOT          0x00000001

/*! \brief Indicates the configuration store which stores user-specific information */
#define KCONF_FLAG_USER          0x00000002

/*! \brief Indicates the configuration store which stores machine-specific information */
#define KCONF_FLAG_MACHINE       0x00000004

/*! \brief Indicates the configuration store which stores the schema */
#define KCONF_FLAG_SCHEMA        0x00000008

/*! \brief Read-only  */
#define KCONF_FLAG_READONLY      0x00000010

/*! \brief Indicates that the last component of the given configuration path is to be considered to be a configuration value */
#define KCONF_FLAG_TRAILINGVALUE 0x00000020

/*! \brief Only write values back there is a change

    Any write operations using the handle with check if the value
    being written is different from the value being read from the
    handle.  It will only be written if the value is different.

    \note Note that the value being read from a handle takes schema and
    shadowed configuration handles into consideration while the value
    being written is only written to the topmost layer of
    configuration that can be written to.

    \note Note also that this flag does not affect binary values.
 */
#define KCONF_FLAG_WRITEIFMOD    0x00000040

/*! \brief Use case-insensitive comparison for KCONF_FLAG_WRITEIFMOD

    When used in combination with \a KCONF_FLAG_WRITEIFMOD , the
    string comparison used when determining whether the string read
    from the configuration handle is the same as the string being
    written will be case insensitive.  If this flag is not set, the
    comparison will be case sensitive.
 */
#define KCONF_FLAG_IFMODCI       0x00000080

/*! \brief Do not parse the configuration space name

    If set, disables the parsing of the configuration space for
    subspaces.  The space name is taken verbatim to be a configuration
    space name.  This can be used when there can be forward slashes or
    backslahes in the name which are not escaped.

    By default, the configuration space name,

    \code
    L"foo\\bar"
    \endcode

    is taken to mean the configuration space \a bar which is a
    subspace of \a foo.  If ::KCONF_FLAG_NOPARSENAME is set, then this
    is taken to mean configuration space \a foo\\bar.
 */
#define KCONF_FLAG_NOPARSENAME   0x00000100
#pragma deprecated("KCONF_FLAG_NOPARSENAME")

/*! \brief Shadow configuration

     When passed into khc_open_space(), checks whether there is a
     configuration space named _Schema that is a sibling of the
     configuration space being opened.  If so, the _Schema space is
     opened as a shadow.
 */
#define KCONF_FLAG_SHADOW        0x00000200

/*! \internal
    \brief Do not initialize providers
 */
#define KCONF_FLAG_NOINIT        0x00000400

/*! \internal
    \brief Attempt to use default providers
 */
#define KCONF_FLAG_TRYDEFATULTS  0x00000800

/*! \brief Apply operation recursively

  \see khc_unmount_provider()
 */
#define KCONF_FLAG_RECURSIVE     0x00010000

/*! \brief Maximum number of allowed characters (including terminating NULL) in a name 

    \note This is a hard limit in Windows, since we are mapping
        configuration spaces to registry keys.
*/
#define KCONF_MAXCCH_NAME 256

/*! \brief Maximum number of allowed bytes (including terminating NULL) in a name */
#define KCONF_MAXCB_NAME (KCONF_MAXCCH_NAME * sizeof(wchar_t))

/*! \brief Maximum level of nesting for configuration spaces
 */
#define KCONF_MAX_DEPTH 16

/*! \brief Maximum number of allowed characters (including terminating NULL) in a configuration path */
#define KCONF_MAXCCH_PATH (KCONF_MAXCCH_NAME * KCONF_MAX_DEPTH)

/*! \brief Maximum number of allowed bytes (including terminating NULL) in a configuration path */
#define KCONF_MAXCB_PATH (KCONF_MAXCCH_PATH * sizeof(wchar_t))

/*! \brief Maximum number of allowed characters (including terminating NULL) in a string */
#define KCONF_MAXCCH_STRING KHM_MAXCCH_STRING

/*! \brief Maximum number of allowed bytes (including terminating NULL) in a string */
#define KCONF_MAXCB_STRING (KCONF_MAXCCH_STRING * sizeof(wchar_t))

/*! \brief Open a configuration space

    Opens the configuration space specified by \a cspace.  By default,
    the opened space includes user,machine and schema configuration
    stores.  However, you can specify a subset of these.

    If the configuration space does not exist and the \a flags specify
    KHM_FLAG_CREATE, then the configuration space is created.  The
    stores that are affected by the create operation depend on \a
    flags.  If the \a flags only specifies ::KCONF_FLAG_MACHINE, then
    the configuration space is created in the machine store.  If \a
    flags specifies any combination of stores including \a
    ::KCONF_FLAG_USER, then the configuration space is created in the
    user store.  Note that ::KCONF_FLAG_SCHEMA is readonly.

    Once opened, use khc_close_space() to close the configuration
    space.

    \param[in] parent The parent configuration space.  The path
        specified in \a cspace is relative to the parent.  Set this to
        NULL to indicate the root configuration space.  

    \param[in] cspace The configuration path.  This can be up to
        ::KCONF_MAXCCH_PATH characters in length.  Use backslashes to
        specify hiearchy.  Set this to NULL to reopen the parent
        configuration space.

    \param[in] flags Flags.  This can be a combination of KCONF_FLAG_*
        constants and KHM_FLAG_CREATE.  If none of ::KCONF_FLAG_USER,
        ::KCONF_FLAG_MACHINE or ::KCONF_FLAG_SCHEMA is specified, then
        it defaults to all three.

    \param[out] result Pointer to a handle which receives the handle
        to the opened configuration space if the call succeeds.

    \note You can re-open a configuration space with different flags
        such as ::KCONF_FLAG_MACHINE by specifying NULL for \a cspace
        and settings \a flags to the required flags.

*/
KHMEXP khm_int32 KHMAPI 
khc_open_space(khm_handle parent, const wchar_t * cspace, khm_int32 flags, 
               khm_handle * result);

/*! \brief Set the shadow space for a configuration handle

    The handle specified by \a lower becomes a shadow for the handle
    specified by \a upper.  Any configuration value that is queried in
    \a upper that does not exist in \a upper will be queried in \a
    lower.

    If \a upper already had a shadow handle, that handle will be
    replaced by \a lower.  The handle \a lower still needs to be
    closed by a call to khc_close_space().  However, closing \a lower
    will not affect \a upper which will still treat the configuration
    space pointed to by \a lower to be it's shadow.

    Shadows are specific to handles and not configuration spaces.
    Shadowing a configuration space using one handle does not affect
    any other handles which may be obtained for the same configuration
    space.

    Specify NULL for \a lower to remove any prior shadow.
 */
KHMEXP khm_int32 KHMAPI 
khc_shadow_space(khm_handle upper, khm_handle lower);

/*! \brief Close a handle opened with khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_close_space(khm_handle conf);

/*! \brief Duplicate a handle
 */
KHMEXP khm_int32 KHMAPI
khc_dup_space(khm_handle vh, khm_handle * pvh);

/*! \brief Read a string value from a configuration space

    The \a value_name parameter specifies the value to read from the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to access the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    - Otherwise, if KCONF_FLAG_SCHEMA was specified, the the schema
      store.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.

    If the value is not found in the configuration space and any
    shadowed configuration spaces, the function returns \a
    KHM_ERROR_NOT_FOUND.  In this case, the buffer is left unmodified.

    \param[in] buf Buffer to copy the string to.  Specify NULL to just
        retrieve the number of required bytes.
    
    \param[in,out] bufsize On entry, specifies the number of bytes of
        space available at the location specified by \a buf.  On exit
        specifies the number of bytes actually copied or the size of
        the required buffer if \a buf is NULL or insufficient.

    \retval KHM_ERROR_NOT_READY The configuration provider has not started
    \retval KHM_ERROR_INVALID_PARAM One or more of the supplied parameters are not valid
    \retval KHM_ERROR_TYPE_MISMATCH The specified value is not a string
    \retval KHM_ERROR_TOO_LONG \a buf was NULL or the size of the buffer was insufficient.  The required size is in bufsize.
    \retval KHM_ERROR_SUCCESS Success.  The number of bytes copied is in bufsize.
    \retval KHM_ERROR_NOT_FOUND The value was not found.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_read_string(khm_handle conf, 
                const wchar_t * value_name, 
                wchar_t * buf, 
                khm_size * bufsize);

/*! \brief Read a multi-string value from a configuration space

    The \a value_name parameter specifies the value to read from the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to access the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    - Otherwise, if KCONF_FLAG_SCHEMA was specified, the the schema
      store.

    A multi-string is a pseudo data type.  The value in the
    configuration store should contain a CSV string.  Each comma
    separated value in the CSV string is considered to be a separate
    value.  Empty values are not allowed. The buffer pointed to by \a
    buf will receive these values in the form of a series of NULL
    terminated strings terminated by an empty string (or equivalently,
    the last string will be terminated by a double NULL).

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.

    If the value is not found in the configuration space and any
    shadowed configuration spaces, the function returns \a
    KHM_ERROR_NOT_FOUND.  In this case, the buffer is left unmodified.

    \param[in] buf Buffer to copy the multi-string to.  Specify NULL
        to just retrieve the number of required bytes.
    
    \param[in,out] bufsize On entry, specifies the number of bytes of
        space available at the location specified by \a buf.  On exit
        specifies the number of bytes actually copied or the size of
        the required buffer if \a buf is NULL or insufficient.

    \retval KHM_ERROR_NOT_READY The configuration provider has not started
    \retval KHM_ERROR_INVALID_PARAM One or more of the supplied parameters are not valid
    \retval KHM_ERROR_TYPE_MISMATCH The specified value is not a string
    \retval KHM_ERROR_TOO_LONG \a buf was NULL or the size of the buffer was insufficient.  The required size is in bufsize.
    \retval KHM_ERROR_SUCCESS Success.  The number of bytes copied is in bufsize.
    \retval KHM_ERROR_NOT_FOUND The value was not found.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_read_multi_string(khm_handle conf, 
                      const wchar_t * value_name, 
                      wchar_t * buf, 
                      khm_size * bufsize);

/*! \brief Read a 32 bit integer value from a configuration space

    The \a value_name parameter specifies the value to read from the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to access the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    - Otherwise, if KCONF_FLAG_SCHEMA was specified, the the schema
      store.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.

    If the value is not found in the configuration space and any
    shadowed configuration spaces, the function returns \a
    KHM_ERROR_NOT_FOUND.  In this case, the buffer is left unmodified.

    \param[in] conf Handle to a configuration space
    \param[in] value The value to query
    \param[out] buf The buffer to receive the value

    \retval KHM_ERROR_NOT_READY The configuration provider has not started.
    \retval KHM_ERROR_SUCCESS Success.  The value that was read was placed in \a buf
    \retval KHM_ERROR_NOT_FOUND The specified value was not found
    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid
    \retval KHM_ERROR_TYPE_MISMATCH The specified value was found but was not of the correct type.
    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_read_int32(khm_handle conf, 
               const wchar_t * value_name, 
               khm_int32 * buf);

/*! \brief Read a 64 bit integer value from a configuration space

    The \a value_name parameter specifies the value to read from the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to access the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    - Otherwise, if KCONF_FLAG_SCHEMA was specified, the the schema
      store.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.

    If the value is not found in the configuration space and any
    shadowed configuration spaces, the function returns \a
    KHM_ERROR_NOT_FOUND.  In this case, the buffer is left unmodified.

    \param[in] conf Handle to a configuration space
    \param[in] value_name The value to query
    \param[out] buf The buffer to receive the value

    \retval KHM_ERROR_NOT_READY The configuration provider has not started
    \retval KHM_ERROR_SUCCESS Success.  The value that was read was placed in \a buf
    \retval KHM_ERROR_NOT_FOUND The specified value was not found
    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid
    \retval KHM_ERROR_TYPE_MISMATCH The specified value was found but was not the correct data type.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_read_int64(khm_handle conf, 
               const wchar_t * value_name, 
               khm_int64 * buf);

/*! \brief Read a binary value from a configuration space

    The \a value_name parameter specifies the value to read from the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to access the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three. Also note that the schema store (KCONF_FLAG_SCHEMA) does
    not support binary values.

    If the value is not found in the configuration space and any
    shadowed configuration spaces, the function returns \a
    KHM_ERROR_NOT_FOUND.  In this case, the buffer is left unmodified.

    \param[in] buf Buffer to copy the string to.  Specify NULL to just
        retrieve the number of required bytes.
    
    \param[in,out] bufsize On entry, specifies the number of bytes of
        space available at the location specified by \a buf.  On exit
        specifies the number of bytes actually copied or the size of
        the required buffer if \a buf is NULL or insufficient.

    \retval KHM_ERROR_SUCCESS Success. The data was copied to \a buf.  The number of bytes copied is stored in \a bufsize
    \retval KHM_ERROR_NOT_FOUND The specified value was not found
    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_read_binary(khm_handle conf, 
                const wchar_t * value_name, 
                void * buf, 
                khm_size * bufsize);

/*! \brief Write a string value to a configuration space

    The \a value_name parameter specifies the value to write to the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to write the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If \a KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if \a KCONF_FLAG_MACHINE was specified, then the
      machine configuration space.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.  Also note that the schema store (KCONF_FLAG_SCHEMA) is
    readonly.

    If the \a KCONF_FLAG_WRITEIFMOD flag is specified in the call to
    khc_open_space() for obtaining the configuration handle, the
    specified string will only be written if it is different from the
    value being read from the handle.

    If the \a KCONF_FLAG_IFMODCI flag is specified along with the \a
    KCONF_FLAG_WRITEIFMOD flag, then the string comparison used will
    be case insensitive.

    \param[in] conf Handle to a configuration space
    \param[in] value_name Name of value to write
    \param[in] buf A NULL terminated unicode string not exceeding KCONF_MAXCCH_STRING in characters including terminating NULL

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_write_string(khm_handle conf, 
                 const wchar_t * value_name, 
                 const wchar_t * buf);

/*! \brief Write a multi-string value to a configuration space

    The \a value_name parameter specifies the value to write to the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to write the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    A multi-string is a pseudo data type.  The buffer pointed to by \a
    buf should contain a sequence of NULL terminated strings
    terminated by an empty string (or equivalently, the last string
    should terminate with a double NULL).  This will be stored in the
    value as a CSV string.

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.  Also note that the schema store (KCONF_FLAG_SCHEMA) is
    readonly.

    If the \a KCONF_FLAG_WRITEIFMOD flag is specified in the call to
    khc_open_space() for obtaining the configuration handle, the
    specified string will only be written if it is different from the
    value being read from the handle.

    If the \a KCONF_FLAG_IFMODCI flag is specified along with the \a
    KCONF_FLAG_WRITEIFMOD flag, then the string comparison used will
    be case insensitive.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_write_multi_string(khm_handle conf, 
                       const wchar_t * value_name, 
                       wchar_t * buf);

/*! \brief Write a 32 bit integer value to a configuration space

    The \a value_name parameter specifies the value to write to the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to write the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.  Also note that the schema store (KCONF_FLAG_SCHEMA) is
    readonly.

    If the \a KCONF_FLAG_WRITEIFMOD flag is specified in the call to
    khc_open_space() for obtaining the configuration handle, the
    specified string will only be written if it is different from the
    value being read from the handle.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_write_int32(khm_handle conf, 
                const wchar_t * value_name, 
                khm_int32 buf);

/*! \brief Write a 64 bit integer value to a configuration space

    The \a value_name parameter specifies the value to write to the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to write the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.  Also note that the schema store (KCONF_FLAG_SCHEMA) is
    readonly.

    If the \a KCONF_FLAG_WRITEIFMOD flag is specified in the call to
    khc_open_space() for obtaining the configuration handle, the
    specified string will only be written if it is different from the
    value being read from the handle.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_write_int64(khm_handle conf, 
                const wchar_t * value_name, 
                khm_int64 buf);

/*! \brief Write a binary value to a configuration space

    The \a value_name parameter specifies the value to write to the
    configuration space.  This can be either a value name or a value
    path consisting of a series nested configuration space names
    followed by the value name all separated by backslashes or forward
    slashes.

    For example: If \a conf is a handle to the configuration space \c
    'A/B/C', then the value name \c 'D/E/v' refers to the value named
    \c 'v' in the configuration space \c 'A/B/C/D/E'.

    The specific configuration store that is used to write the value
    depends on the flags that were specified in the call to
    khc_open_space().  The precedence of configuration stores are as
    follows:

    - If KCONF_FLAG_USER was specified, then the user configuration
      space.

    - Otherwise, if KCONF_FLAG_MACHINE was specified, then the machine
      configuration space.

    Note that not specifying any of the configuration store specifiers
    in the call to khc_open_space() is equivalent to specifying all
    three.  Also note that the schema store (KCONF_FLAG_SCHEMA) is
    readonly.

    \see khc_open_space()
*/
KHMEXP khm_int32 KHMAPI 
khc_write_binary(khm_handle conf, 
                 const wchar_t * value_name, 
                 const void * buf, 
                 khm_size bufsize);

/*! \brief Get the type of a value in a configuration space

    \return The return value is the type of the specified value, or
        KC_NONE if the value does not exist.
 */
KHMEXP khm_int32 KHMAPI 
khc_get_type(khm_handle conf, const wchar_t * value_name);

/*! \brief Check which configuration stores contain a specific value.

    Each value in a configuration space can be contained in zero or
    more configuration stores.  Use this function to determine which
    configuration stores contain the specific value.

    The returned bitmask always indicates a subset of the
    configuration stores that were specified when opening the
    configuration space corresponding to \a conf.

    If the specified handle is shadowed (see khc_shadow_space()) and
    the value is not found in any of the visible stores for the
    topmost handle, each of the shadowed handles will be tried in turn
    until the value is found.  The return value will correspond to the
    handle where the value is first found.

    \return A combination of ::KCONF_FLAG_MACHINE, ::KCONF_FLAG_USER
        and ::KCONF_FLAG_SCHEMA indicating which stores contain the
        value.
 */
KHMEXP khm_int32 KHMAPI 
khc_value_exists(khm_handle conf, const wchar_t * value);

/*! \brief Remove a value from a configuration space

    Removes a value from one or more configuration stores.

    A value can exist in multiple configuration stores.  Only the
    values that are stored in writable stores can be removed.  When
    the function searches for values to remove, it will only look in
    configuration stores that are specified in the handle.  In
    addition, the configuration stores affected can be further
    narrowed by specifying them in the \a flags parameter.  If \a
    flags is zero, then all the stores visible to the handle are
    searched.  If \a flags specifies ::KCONF_FLAG_USER or
    ::KCONF_FLAG_MACHINE or both, then only the specified stores are
    searched, provided that the stores are visible to the handle.

    This function only operates on the topmost configuration space
    visible to the handle.  If the configuration handle is shadowed,
    the shadowed configuration spaces are unaffected by the removal.

    \param[in] conf Handle to configuration space to remove value from

    \param[in] value_name Value to remove

    \param[in] flags Specifies which configuration stores will be
        affected by the removal.  See above.

    \retval KHM_ERROR_SUCCESS The value was removed from all the
        specified configuration stores.

    \retval KHM_ERROR_NOT_FOUND The value was not found.

    \retval KHM_ERROR_UNKNOWN An unknown error occurred while trying
        to remove the value.

    \retval KHM_ERROR_PARTIAL The value was successfully removed from
        one or more stores, but the operation failed on one or more
        other stores.
 */
KHMEXP khm_int32 KHMAPI
khc_remove_value(khm_handle conf, const wchar_t * value_name, khm_int32 flags);

/*! \brief Retrieves the last write time for a configuration space

    The last write time is based on the visible configuration stores.
    The visibility can be further narrowed down using the \a flags
    parameter.

    \param[in] conf Handle to configuration space to query.

    \param[in] flags Narrow down the visibility.  If the \a flags
        parameter is 0, then all configuration stores accessible
        through \a conf are visible.  If not, then only the
        configuration stores specified in \a flags and visible from \a
        conf are visible for the query.

    \param[out] last_w_time Receives the last write time as a \a
        FILETIME value.  If more than one configuration store is
        visible, then the returned value will be the most recent last
        write time.

    \note The schema store does not have last write times.  Therefore
        this function call will only succeed if the user or machine
        configuration stores are visible and the configuration space
        is defined in one of these stores.

    \retval KHM_ERROR_SUCCESS The query was successful.

    \retval KHM_ERROR_NOT_FOUND The configuration space name was not
        found in the visible configuration stores.

    \retval KHM_ERROR_INVALID_PARAM One or more parameters were
        invalid.
  */
KHMEXP khm_int32 KHMAPI
khc_get_last_write_time(khm_handle conf, khm_int32 flags, FILETIME * last_w_time);

/*! \brief Get the name of a configuration space

    \param[in] conf Handle to a configuration space

    \param[out] buf The buffer to receive the name.  Set to NULL if
        only the size of the buffer is required.

    \param[in,out] bufsize On entry, holds the size of the buffer
        pointed to by \a buf.  On exit, holds the number of bytes
        copied into the buffer including the NULL terminator.
 */
KHMEXP khm_int32 KHMAPI 
khc_get_config_space_name(khm_handle conf, 
                          wchar_t * buf, 
                          khm_size * bufsize);

/*! \brief Get a handle to the parent space

    \param[in] conf Handle to a configuration space

    \param[out] parent Handle to the parent configuration space if the
        call succeeds.  Receives NULL otherwise.  The returned handle
        must be closed using khc_close_space()
 */
KHMEXP khm_int32 KHMAPI 
khc_get_config_space_parent(khm_handle conf, 
                            khm_handle * parent);

/*! \brief Load a configuration schema into the specified configuration space

    \param[in] conf Handle to a configuration space or NULL to use the
        root configuration space.

    \param[in] schema The schema to load.  The schema is assumed to be
        well formed.

    \see khc_unload_schema()
 */
KHMEXP khm_int32 KHMAPI 
khc_load_schema(khm_handle conf, 
                const kconf_schema * schema);

/*! \brief Unload a schema from a configuration space
 */
KHMEXP khm_int32 KHMAPI 
khc_unload_schema(khm_handle conf, 
                  const kconf_schema * schema);

/*! \brief Enumerate the subspaces of a configuration space

    Prepares a configuration space for enumeration and returns the
    child spaces in no particular order.

    \param[in] conf The configuration space to enumerate child spaces

    \param[in] prev The previous configuration space returned by
        khc_enum_subspaces() or NULL if this is the first call.  If
        this is not NULL, then the handle passed in \a prev will be
        freed.

    \param[out] next If \a prev was NULL, receives the first sub space
        found in \a conf.  You must \b either call
        khc_enum_subspaces() again with the returned handle or call
        khc_close_space() to free the returned handle if no more
        subspaces are required.  \a next can point to the same handle
        specified in \a prev.

    \retval KHM_ERROR_SUCCESS The call succeeded.  There is a valid
        handle to a configuration space in \a first_subspace.

    \retval KHM_ERROR_INVALID_PARAM Either \a conf or \a prev was not a
        valid configuration space handle or \a first_subspace is NULL.
        Note that \a prev can be NULL.

    \retval KHM_ERROR_NOT_FOUND There were no subspaces in the
        configuration space pointed to by \a conf.

    \note The configuration spaces that are enumerated directly belong
        to the configuration space given by \a conf.  This function
        does not enumerate subspaces of shadowed configuration spaces
        (see khc_shadow_space()).  Even if \a conf was obtained on a
        restricted domain (i.e. you specified one or more
        configuration stores when you openend the handle and didn't
        include all the configuration stores. See khc_open_space()),
        the subspaces that are returned are the union of all
        configuration spaces in all the configuration stores.  This is
        not a bug.  This is a feature.  In NetIDMgr, a configuartion
        space exists if some configuration store defines it (or it was
        created with a call to khc_open_space() even if no
        configuration store defines it yet).  This is the tradeoff you
        make when using a layered configuration system.

	However, the returned handle has the same domain restrictions
	as \a conf.
 */
KHMEXP khm_int32 KHMAPI 
khc_enum_subspaces(khm_handle conf,
                   khm_handle prev,
                   khm_handle * next);

/*! \brief Remove a configuration space

    The configuration space will be marked for removal.  Once all the
    handles for the space have been released, it will be deleted.  The
    configuration stores that will be affected are the write enabled
    configuration stores for the handle.
 */
KHMEXP khm_int32 KHMAPI
khc_remove_space(khm_handle conf);
/*@}*/

/*! \page kconf_p_handles Using the configuration provider handle

  When a configuration provider is mounted, the system calls the \a
  init() method of the provider and passes in a handle to a
  configuration space.  This handle is different from the usual
  configuration handle returned by khc_open_space() etc. and can only
  be used by the configuration provider only with the following APIs:

  - khc_mount_provider()

  - khc_unmount_provider()

  Usage of this handle is as described in the documentation for each
  of the functions.  However, in general, the provider handle for a
  configuration space is not restricted to any particular
  configuration store and is always considered read/write.

  These handles do NOT need to closed using khc_close_space().
  Attempting to use it with any other API will result in an error and
  is otherwise harmless.
 */

/*! \brief Configuration provider API

  While configuration information is typically stored on the Windows
  registry, plug-ins can provide their own configuration store
  providers.  This structure is used to establish a set of callbacks
  that the configuration subsystem will use to communicate with your
  provider.

  In order to use a provider, you will need to do the following:

  - Implement callback handlers for all the callbacks in the
    ::khc_provider_interface structure.

  - Fill in a ::khc_provider_interface structure.

  - Obtain a handle to the parent configuration space within which you
    want to register the provider.

  - Call khc_mount_provider() using the obtained handle and the
    ::khc_provider_interface structure.

  Once you are done with with the provider, you can unmount it using
  the khc_unmount_provider() call.

  During operation, calls to the registered callbacks may be made from
  any thread.  However, all calls to provider are serialized.
  Therefore, your provider may NOT assume that it would be called from
  any specific thread.

  The first call to a provider will always be
  khc_provider_interface::init() while the last call will always be
  khc_provider_interface::exit().

  A configuration space provider can only operate on a single space.
  However, when processing the \a init() callback or \a begin_enum()
  callback, it may mount as many child spaces as necessary.
 */
typedef struct khc_provider_interface {
    /*! \brief Provider version

      Should be set to ::KHC_PROVIDER_V1.
     */
    khm_int32 version;

    /*! \brief Mount provider

      Called to mount the provider on a configuration space.

      \param[in] sp_handle Handle to configuration space.  This is not
          a regular configuration space handle and can only be used
          with a select few kconfig APIs.  See \ref kconf_p_handles .

      \param[in] path The full path to the configuration space on
          which this provider is being mounted.  See notes.

      \param[in] flags Flags which designate the store and whether the
          store should be created if it doesn't already exist.  The
          only flags that are passed is EXACTLY ONE OF
          ::KCONF_FLAG_USER, ::KCONF_FLAG_MACHINE or
          ::KCONF_FLAG_SCHEMA along with ::KHM_FLAG_CREATE.  If the
          ::KHM_FLAG_CREATE flag is specified, the configuration store
          should be created if it doesn't already exist.

      \param[in] context A context handle that was used when calling
          khc_mount_provider().  This is application defined.

      \param[out] r_nodeHandle Upon successful completion, a context
          handle should be returned through this parameter.  This
          handle will be used in all subsequent calls up until \a
          exit().

      The path passed in through the \a path parameter is the full
      path to the configuration space.  E.g.:

      \code
      L"\\PluginManager\\Modules\\Foo"
      \endcode

      The full path always starts with a backslash and ends with the
      last component name (no trailing backslash).

      One of the following values should be returned:

      \retval KHM_ERROR_SUCCESS The provider was successfully
          initialized and a valid context handle was returned in \a
          r_nodeHandle.

      \retval KHM_ERROR_NOT_FOUND A backing configuration store was
          not found or this provider can not be mounted at this store
          or space.  In this case the provider will be immediately
          unmounted from the configuration space.  The \a exit()
          method will NOT be invoked.
     */
    khm_int32 (KHMCALLBACK * init)(khm_handle sp_handle,
                                   const wchar_t * path, khm_int32 flags,
                                   void * context, void ** r_nodeHandle);

    /*! \brief Unmount a provider

      Each successful call of \a init() will be coupled with a call to
      \a exit().  This is the last invocation of the provider for this
      mount.  The provider will be immediately unmounted after this
      method is called.

      The return value of this method is ignored.

      It is the responsibility of the provider to perform any clean-up
      operations.  Note that it is possible for \a exit() to be called
      before all open handles have been released.
     */
    khm_int32 (KHMCALLBACK * exit)(void * nodeHandle);

    /*! \brief Notify open handle

      Used to notify the provider that a handle is to be created that
      uses this configuration store.  This function can be called more
      than once if more than one handle is being created.

      Each call to \a open() will be coupled with a call to \a close()
      to indicate that the allocated handle is being freed.  The only
      exception is if the configuration store is unloaded before all
      open handles are released, in which case the provider will
      receive an \a exit() call before all the \a close() calls are
      sent.

      The provider is expected to take any steps necessary to provider
      access to the underlying configuration store.  If no open
      handles are in existence, the configuration store can be assumed
      to be in an idle state.

     */
    khm_int32 (KHMCALLBACK * open)(void * nodeHandle);

    /*! \brief Notify close handle

      Complements \a open().

      \see khc_provider_interface::open()
     */
    khm_int32 (KHMCALLBACK * close)(void * nodeHandle);

    /*! \brief Remove configuration store

      Called to remove the configuration store represented by \a
      nodeHandle.  The provider is expected to remove any backing
      storage used to provide configuration information for this
      configuration space.

      \note The provider wil be unloaded immediately following this
      call. I.e. The khc_provider_interface::remove() call will be
      immediately followed by a khc_provider_interface::exit() call.
     */
    khm_int32 (KHMCALLBACK * remove)(void * nodeHandle);

    /*! \brief Create or open a child configuration store
     
      In response the provider is expected to open or create a child
      configuration store.

      \param[in] nodeHandle Handle to the configuration store within
          which the child store should be created or openend.

      \param[in] name Name of the child configuration store.

      \param[in] flags Identifies the store and signals whether it
          should be created if it doesn't already exist.  See remarks
          below.

      Exactly one of ::KCONF_FLAG_USER, ::KCONF_FLAG_MACHINE or
      ::KCONF_FLAG_SCHEMA will be present in flags.  This flag
      indicates which configuration store this call applies to and
      always corresponds to the configuration store that the current
      provider has been mounted on.  In addition, ::KHM_FLAG_CREATE
      may also be present, in which case the provider should attempt
      to create the store if it doesn't already exist.
     */
    khm_int32 (KHMCALLBACK * create)(void * nodeHandle, const wchar_t * name, khm_int32 flags);
    khm_int32 (KHMCALLBACK * begin_enum)(void * nodeHandle);
    khm_int32 (KHMCALLBACK * get_mtime)(void * nodeHandle, FILETIME * mtime);

    /*! \brief Read a value

      \param[in] nodeHandle A handle that was acquired by calling \a init()

      \param[in] valuename  Name of the value to read

      \param[in,out] pvtype The expected type of the value.  If the
          type of the value found in the store cannot be converted to
          this type, then the function should return
          ::KHM_ERROR_TYPE_MISMATCH.  A type of ::KC_NONE or a \a NULL
          \a pvtype matches any type.  On successful return, this
          should be set to the actual type of the value.

      \param[out] buffer Buffer to receive the data.  This can be NULL.
          See remarks below.

      \param[in,out] pcb Pointer to size of \a buffer in bytes.  This can
          be NULL. See remarks below.

      Buffer manangement follows the pattern used throughout the
      Network Identity Manager framework.

      - If \a buffer is \a NULL AND \a pcb is NOT \a NULL, then the
        required size of the buffer should be stored in the value
        pointed to by \a pcb and the return value should be
        ::KHM_ERROR_TOO_LONG. (Assuming the value exists).

      - If \a buffer is NOT \a NULL AND \a pcb is NOT \a NULL AND the
        size is sufficient to store the found value, then the value
        should be stored in \a buffer and the size in \a pcb should be
        updated with the actual size of the found value.  The return
        value should ::KHM_ERROR_SUCCESS.

      - If both \a buffer and \a pcb is \a NULL, then the caller is
        requesting that the existence of the value be reported.  In
        this case, the return value should be ::KHM_ERROR_SUCCESS or
        ::KHM_ERROR_NOT_FOUND depending on whether the value was found
        or not.

      If the expected type of the value is ::KC_NONE, it should be
      interpreted as \a any.  In which case any value found in the
      configuration store matching the name should be considered for
      return.

      If the value found in the configuration store is not of the
      correct type, it may be converted to the expected type before
      returning.  If the value cannot be converted, then the function
      should behave as if the requested value was not found.

      \retval KHM_ERROR_SUCCESS Should mean that the value was found
          and successfully copied (in full) to the buffer pointed to
          by \a buffer and that the number of copied bytes were stored
          in the value pointed to by \a pcb.  If both \a buffer and \a
          pcb is NULL, this means that the value exists with the
          expected type in this configuration store.

      \retval KHM_ERROR_TOO_LONG Should mean that the supplied buffer
          was insufficient to store the data and that the required
          length of the buffer in bytes was stored in the value
          pointed to by \a pcb.

      \retval KHM_ERROR_NOT_FOUND The requested value was not found in
          this configuration store.

      \retval KHM_ERROR_TYPE_MISMATCH The requested value was found
          but the type of the found value was incompatible with the
          expected type.

      \retval KHM_ERROR_NOT_READY The configuration store is not ready
          to be read at this time.  Further action may be necessary to
          allow the request to go through.
     */
    khm_int32 (KHMCALLBACK * read_value)(void * nodeHandle, const wchar_t * valuename,
                                         khm_int32 * vtype, void * buffer, khm_size * pcb);

    /*! \brief Write a value

      \param[in] nodeHandle Handle acquired by calling \a init()

      \param[in] valuename Name of value being written.

      \param[in] vtype Type of value being written.

      \param[in] buffer Buffer containing value data.

      \param[in] cb Size (in bytes) of the value being written.

      The value must be stored in the configuration store represented
      by \a nodeHandle.  Upon successful completion, one of the
      following return values should be returned.

      \retval KHM_ERROR_SUCCESS The value was successfully written.

      \retval KHM_ERROR_INVALID_PARAM The format of the data was
          inconsistent with the type.  This can be caused by a size
          mismatch or a ::KC_STRING value not having a proper \a NUL
          terminator or being too long.

      \retval KHM_ERROR_TYPE_MISMATCH The expected type of the value
          is different from what was being requested.

      \retval KHM_ERROR_NOT_READY The configuration store is not ready
          to be written to at this time.  Further action may be necessary
          to prepare the store for writing.

      \retval KHM_ERROR_READONLY The configuration store is read-only.
          Or the current user is not allowed to write to the
          configuration store.
     */
    khm_int32 (KHMCALLBACK * write_value)(void * nodeHandle, const wchar_t * valuename,
                                          khm_int32 vtype, const void * buffer, khm_size cb);

    /*! \brief Remove value

      \param[in] nodeHandle Handle acquired by calling \a init()

      \param[in] valuename Name of value to remove

      The value should be removed if it exists.  The return value
      should be one of the following:

      \retval KHM_ERROR_SUCCESS The value was removed successfully.

      \retval KHM_ERROR_NOT_FOUND The value was not found.

      \retval KHM_ERROR_NOT_READY The configuration space is not ready
          to complete the operation at this time.

      \retval KHM_ERROR_READONLY The configuration space is read-only.
          Values can't be removed.
     */
    khm_int32 (KHMCALLBACK * remove_value)(void * nodeHandle, const wchar_t * valuename);

} khc_provider_interface;

#define KHC_PROVIDER_V1 1

/*! \brief Mount a provider on a configuration space

  Can be called to mount a configuration provider on a configuration
  space. The configuration space does not need to exist prior to the
  call, although the parent configuration space must exist.

  \param[in] conf Handle to parent configuration space.

  \param[in] name Name of configuration space to mount provider on.
      This must be a name and not a path.  I.e. \a name cannot contain
      any backslashes.

  \param[in] flags Flags indicating which stores should be affected by
      the new provider.  See remarks.

  \param[in] provider Provider callback data.

  \param[in] context Context.  Passed to the \a init() method of the
      provider.

  \param[out] ret_conf If the call is successful, this receives a
      handle to the configuration space indicated by \a name.

  The configuration stores that will be affected are indicated by the
  \a flags parameter, which can be a combination of ::KCONF_FLAG_USER,
  ::KCONF_FLAG_MACHINE, ::KCONF_FLAG_SCHEMA.

  In addition ::KHM_FLAG_CREATE can also be specified in \a flags.
  This flag does not affect the behavior of khc_mount_provider(), but
  it will be passed on to the init() function of the provider.

  If ::KCONF_FLAG_RECURSIVE is specified in \a flags, then the
  provider is assumed to be recursive provider.  In other words, if \a
  p provides the user store for configuration space \a s, then it also
  provides the user store for all children of \a s.  In this case,
  after successfully associating the provider with the configuration
  space, the provider will be associated with all the child spaces as
  well.

  For each configuration store specified in \a flags, the provider
  indicated by \a provider will be mounted by calling the \a init()
  method.  Each invocation will specify a different store.

  \retval KHM_ERROR_SUCCESS Successfully mounted at least one
  configuration store.

  \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid.

  \retval KHM_ERROR_NO_PROVIDER The provider failed to initialize.

 */
KHMEXP khm_int32 KHMAPI
khc_mount_provider(khm_handle conf, const wchar_t * name, khm_int32 flags,
                   const khc_provider_interface * provider,
                   void * context, khm_handle * ret_conf);

/*! \brief  Unmount a configuration provider

  Unmounts a provider from the specified configuration space and
  stores.

  \param[in] conf The configuration space to unmount provider from.

  \param[in] flags Specifies the configuration stores that will be
      affected.  The flags can be a combination of ::KCONF_FLAG_USER,
      ::KCONF_FLAG_MACHINE and ::KCONF_FLAG_SCHEMA.  The provider
      associated with configuration store corresponding to each
      supplied flag will be unmounted.

      In addition, the ::KCONF_FLAG_RECURSIVE flag may be specified to
      apply the unmount operation to the entire tree starting at the
      configuration store specified by \a conf.

  Only the configuration stores visible to the handle specified in \a
  conf will be affected.  Setting \a conf to NULL indicates the root
  configuration space with all configuration stores visible.

  The following command removes the given provider from the user store
  of all the configuration spaces in the system:

  \code
  khc_unmount_provider(NULL, &my_provider, KCONF_FLAG_USER|KCONF_FLAG_RECURSIVE);
  \endcode

  \see khc_mount_provider()
 */
KHMEXP khm_int32 KHMAPI
khc_unmount_provider(khm_handle conf, const khc_provider_interface * provider,
                     khm_int32 flags);

/*! \defgroup kconf_st Memory stores
@{
 */

KHMEXP khm_int32 KHMAPI
khc_memory_store_create(khm_handle * ret_sp);

KHMEXP khm_int32 KHMAPI
khc_memory_store_add(khm_handle sp, const wchar_t * name, khm_int32 type,
                     const void * data, khm_size cb);

typedef void (KHMCALLBACK * khc_store_enum_cb)(khm_int32 type, const wchar_t * name,
                                               const void * data, khm_size cb, void * ctx);

KHMEXP khm_int32 KHMAPI
khc_memory_store_enum(khm_handle sp, khc_store_enum_cb cb, void * ctx);

KHMEXP khm_int32 KHMAPI
khc_memory_store_hold(khm_handle sp);

KHMEXP khm_int32 KHMAPI
khc_memory_store_release(khm_handle sp);

KHMEXP khm_int32 KHMAPI
khc_memory_store_mount(khm_handle csp_parent, khm_int32 store, khm_handle sp, khm_handle * ret);

KHMEXP khm_int32 KHMAPI
khc_memory_store_unmount(khm_handle sp);

typedef struct khc_memory_store_notify {
    void (KHMCALLBACK *notify_init)(void * context, khm_handle s);
    void (KHMCALLBACK *notify_exit)(void * context, khm_handle s);
    void (KHMCALLBACK *notify_open)(void * context, khm_handle s);
    void (KHMCALLBACK *notify_close)(void * context, khm_handle s);
    void (KHMCALLBACK *notify_modify)(void * context, khm_handle s);
} khc_memory_store_notify;

KHMEXP khm_int32 KHMAPI
khc_memory_store_set_notify_interface(khm_handle sp, const khc_memory_store_notify * pnotify,
                                      void * context);

/*@}*/

END_C

#ifdef __cplusplus
#include <string>

namespace nim {

class ConfigSpace {
protected:
    khm_handle csp;
    khm_int32  last_error;

public:
    ConfigSpace()  {
        csp = NULL; 
        last_error = KHM_ERROR_NOT_READY;
    }

    ConfigSpace(khm_handle _csp) {
        csp = _csp;
        last_error = KHM_ERROR_SUCCESS;
    }

    ConfigSpace(const wchar_t * name, khm_int32 flags = 0) {
        csp = NULL;
        last_error = khc_open_space(NULL, name, flags, &csp);
    }

    ConfigSpace(ConfigSpace & parent, const wchar_t * name, khm_int32 flags = 0) {
        csp = NULL;
        last_error = khc_open_space(parent.csp, name, flags, &csp);
    }

    ConfigSpace(const ConfigSpace& that) {
        csp = NULL;
        last_error = khc_dup_space(that.csp, &csp);
    }

    ~ConfigSpace() {
        Close();
    }

    operator khm_handle () const {
        return csp;
    }

    khm_int32 Close() {
        if (csp) {
            last_error = khc_close_space(csp);
            csp = NULL;
            return last_error;
        } else {
            return KHM_ERROR_SUCCESS;
        }
    }

    khm_int32 Open(const wchar_t * name, khm_int32 flags = 0) {
        Close();
        return last_error = khc_open_space(NULL, name, flags, &csp);
    }

    khm_int32 Open(ConfigSpace & parent, const wchar_t * name, khm_int32 flags = 0) {
        Close();
        return last_error = khc_open_space(parent.csp, name, flags, &csp);
    }

    khm_int32 GetLastError() const { return (csp) ? last_error : KHM_ERROR_NOT_READY; }

    FILETIME GetLastWriteTime(khm_int32 flags = 0) {
        FILETIME rv = {0,0};
        last_error = khc_get_last_write_time(csp, flags, &rv);
        return rv;
    }

    khm_int32 GetInt32(const wchar_t * name, khm_int32 def = 0) {
        khm_int32 val = def;
        last_error = khc_read_int32(csp, name, &val);
        return val;
    }

    khm_int64 GetInt64(const wchar_t * name, khm_int64 def = 0) {
        khm_int64 val = 0;
        last_error = khc_read_int64(csp, name, &val);
        return val;
    }

    std::wstring GetString(const wchar_t * name, const wchar_t * def = L"") {
        khm_size cb = 0;
        int n_tries_left = 5;

        while ((last_error = khc_read_string(csp, name, NULL, &cb)) == KHM_ERROR_TOO_LONG &&
               n_tries_left > 0) {
            wchar_t * wbuffer = NULL;

            wbuffer = static_cast<wchar_t *>(PMALLOC(cb));
            last_error = khc_read_string(csp, name, wbuffer, &cb);
            if (KHM_SUCCEEDED(last_error)) {
                std::wstring val(wbuffer);
                PFREE(wbuffer);
                return val;
            }

            PFREE(wbuffer);
            n_tries_left--;
        }

        return std::wstring(def);
    }

    void * GetBinary(const wchar_t * name, void * buffer, khm_size & cb) {
        last_error = khc_read_binary(csp, name, buffer, &cb);
        return (KHM_SUCCEEDED(last_error))? buffer : NULL;
    }

    template <class target>
    target& GetObject(const wchar_t * name, target& obj) {
        khm_size cb = sizeof(obj);
        last_error = khc_read_binary(csp, name, &obj, &cb);
        return obj;
    }

    multi_string GetMultiString(const wchar_t * name) {
        khm_size cb = 0;

        do {
            last_error = khc_read_multi_string(csp, name, NULL, &cb);
            if (last_error != KHM_ERROR_TOO_LONG) break;

            wchar_t * wbuffer = NULL;

            wbuffer = static_cast<wchar_t *>(PMALLOC(cb));
            last_error = khc_read_multi_string(csp, name, wbuffer, &cb);
            if (KHM_FAILED(last_error)) {
                PFREE(wbuffer);
                break;
            }

            multi_string val(wbuffer);

            PFREE(wbuffer);

            return val;

        } while (false);

        multi_string val;

        return val;
    }

    void Set(const wchar_t * name, khm_int32 val) {
        last_error = khc_write_int32(csp, name, val);
    }

    void Set(const wchar_t * name, bool val) {
        last_error = khc_write_int32(csp, name, (khm_int32)((val)? 1 : 0));
    }

    void Set(const wchar_t * name, khm_int64 val) {
        last_error = khc_write_int64(csp, name, val);
    }

    void Set(const wchar_t * name, std::wstring & val) {
        last_error = khc_write_string(csp, name, val.c_str());
    }

    void Set(const wchar_t * name, const wchar_t * val) {
        last_error = khc_write_string(csp, name, val);
    }

    void Set(const wchar_t * name, multi_string val) {
        wchar_t * wval = val.new_c_multi_string();
        last_error = khc_write_multi_string(csp, name, wval);
        PFREE(wval);
    }

    void Set(const wchar_t * name, const void * data, khm_size cb_data) {
        last_error = khc_write_binary(csp, name, data, cb_data);
    }

    template <class target>
    void SetObject(const wchar_t * name, target& t) {
        Set(name, &t, sizeof(t));
    }

    khm_int32 Exists(const wchar_t * name) const {
        return khc_value_exists(csp, name);
    }
};
}
#endif

#endif
