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

#ifndef __KHIMAIRA_KCREDDB_H__
#define __KHIMAIRA_KCREDDB_H__

#include "khdefs.h"
#include<time.h>

BEGIN_C

/*! \defgroup kcdb Credentials Database */
/*@{*/

/*! \brief Maximum length in characters of short description 

    The length includes the terminating \a NULL character.
    */
#define KCDB_MAXCCH_SHORT_DESC  256

/*! \brief Maximum length in bytes of short description 

    The length includes the terminating \a NULL character.
    */
#define KCDB_MAXCB_SHORT_DESC   (sizeof(wchar_t) * KCDB_MAXCCH_SHORT_DESC)

/*! \brief Maximum length in characters of long description 

    The length includes the terminating \a NULL character.
    */
#define KCDB_MAXCCH_LONG_DESC   8192

/*! \brief Maximum length in characters of long description 

    The length includes the terminating \a NULL character.
    */
#define KCDB_MAXCB_LONG_DESC    (sizeof(wchar_t) * KCDB_MAXCCH_LONG_DESC)

/*! \brief Maximum length in characters of name 

    The length includes the terminating \a NULL character.
    */
#define KCDB_MAXCCH_NAME        256

/*! \brief Maximum length in bytes of short description 

    The length includes the terminating \a NULL character.
    */
#define KCDB_MAXCB_NAME         (sizeof(wchar_t) * KCDB_MAXCCH_NAME)

/*! \brief Automatically determine the number of bytes required

    Can be used in most places where a count of bytes is required.
    For many objects, the number of bytes that are required can be
    determined through context and may be ommited.  In such cases you
    can use the \a KCDB_CBSIZE_AUTO value to specify that the function
    is to determine the size automatically.

    \note Not all functions that take a count of bytes support the \a
        KCDB_CBSIZE_AUTO value.
*/
#define KCDB_CBSIZE_AUTO ((khm_size) -1)

struct tag_kcdb_enumeration;

/*! \brief Generic enumeration */
typedef struct tag_kcdb_enumeration *kcdb_enumeration;

/*!
\defgroup kcdb_ident Identities

Functions, macros etc. for manipulating identities.
*/

/*@{*/

/*! \brief The maximum number of characters (including terminator) that can
           be specified as an identity name */
#define KCDB_IDENT_MAXCCH_NAME 256

/*! \brief The maximum number of bytes that can be specified as an identity
           name */
#define KCDB_IDENT_MAXCB_NAME (sizeof(wchar_t) * KCDB_IDENT_MAXCCH_NAME)

/*!
\name Flags for identities */
/*@{*/

/*! \brief Create the identity if it doesn't already exist. 
    \note  Only to be used with kcdb_identity_create() */
#define KCDB_IDENT_FLAG_CREATE      0x10000000L

/*! \brief Has configuration information

    Indicates that the identity has persistent configuration
    information associated with it.
 */
#define KCDB_IDENT_FLAG_CONFIG      0x00800000L

/*! \brief Marks the identity as active.

    An active identity is one that is in active use within NetIDMgr.

    \note This flag is readonly and cannot be specified when creating
        or modifying an identity. Once an identity is deleted, it will
        no longer have this flag. */
#define KCDB_IDENT_FLAG_ACTIVE      0x02000000L

/*! \brief The identity has custom attributes assigned
 */
#define KCDB_IDENT_FLAG_ATTRIBS     0x08000000L

/*! \brief This is the default identity. 

    At most one identity will have this flag set at any given time.
    To set or reset the flag, use kcdb_identity_set_default() */
#define KCDB_IDENT_FLAG_DEFAULT     0x00000001L

/*! \brief This identity can be searched.

    The meaning of this flag is left to be interpreted by individual
    plugins. */
#define KCDB_IDENT_FLAG_SEARCHABLE  0x00000002L

/*! \brief Hidden identity.

    The identity will not show up in the identity list window.  Once
    the hidden is switched off, the identity (and all associated
    credentials) will re-appear in the window */
#define KCDB_IDENT_FLAG_HIDDEN      0x00000004L

/*! \brief Invalid identity

    For one reason or another, this identity is invalid.  This flag
    can be set by an identity provider to indicate that this identity
    does not correspond to an actual identity because an external
    entity (such as a KDC) has denied it's existence.

    The absence of this flag does not imply that the identity is
    valid.  The ::KCDB_IDENT_FLAG_VALID bit must be set for that to be
    the case.  If neither flag is set, then the status of the identity
    is not known.
*/
#define KCDB_IDENT_FLAG_INVALID     0x00000008L

/*! \brief Valid identity

    The identity has been validated through an external entity, or
    it's validity implied through the existence of credentials for the
    identity.

    The absence of this flag does not imply that the identity is
    invalid.  The ::KCDB_IDENT_FLAG_INVALID bit must be set for that
    to be the case.  If neither flag is set, then the status of the
    identity is not known.
 */
#define KCDB_IDENT_FLAG_VALID       0x00000010L

/*! \brief Expired identity

  This identity is expired.

  \deprecated This flag is no longer supported.
*/
#define KCDB_IDENT_FLAG_EXPIRED     0x00000020L
#pragma deprecated("KCDB_IDENT_FLAG_EXPIRED")

/*! \brief Empty identity

    The identity does not have actual credentials associated with it.
 */
#define KCDB_IDENT_FLAG_EMPTY       0x00000040L

/*! \brief Renewable identity

    The initial credentials associated with this identity are
    renewable.  Thus making the whole identity renewable.
 */
#define KCDB_IDENT_FLAG_RENEWABLE   0x00000080L

/*! \brief Unused

  Use of this flag is deprecated.

  \deprecated This flag was not being used.
 */
#define KCDB_IDENT_FLAG_INTERACT    0x00000100L
#pragma deprecated("KCDB_IDENT_FLAG_INTERACT")

/*! \brief Has expired credentials

  Use of this flag is deprecated.

  \deprecated This flag is no longer supported.  Plug-ins that need to
  determine whether any credentials belonging to an identity are
  expired should enumerate the corresponding credentials and check
  them for expiration.
 */
#define KCDB_IDENT_FLAG_CRED_EXP    0x00000200L
#pragma deprecated("KCDB_IDENT_FLAG_CRED_EXP")

/*! \brief Has renewable credentials

    The identity has renewable credentials associated with it.  If the
    initial credentials of the identity are renewable, then identity
    is renewable.  Hence the ::KCDB_IDENT_FLAG_RENEWABLE should also
    be set.
 */
#define KCDB_IDENT_FLAG_CRED_RENEW  0x00000400L

/*! \brief Sticky identity

    Sticky identities are identities that are always visible in the
    credentials display even if no credentials are associated with it.
 */
#define KCDB_IDENT_FLAG_STICKY      0x00000800L

/*! \brief Unknown state

    The validity of the identity cannot be determined.  This usually
    means that an authority could not be contacted.  This flag is to
    be treated as transient.  If ::KCDB_IDENT_FLAG_INVALID or
    ::KCDB_IDENT_FLAG_VALID is set for the identity, this flag is to
    be ignored.
 */
#define KCDB_IDENT_FLAG_UNKNOWN     0x00001000L

/*! \brief The identity requires a refresh

    This flag is set when a change in the root credentials set
    requires that the properties of the identity to be refreshed with
    a call to kcdb_identity_refresh().  It is automatically set by
    KCDB when the root credentials set is modified.  However, an
    identity provider may choose to set this flag indepently if a
    refresh is needed.

    kcdb_identity_refresh_all() will only refresh identities for which
    this flag is set.
 */
#define KCDB_IDENT_FLAG_NEEDREFRESH 0x00002000L

/*! \brief The private key can be exported

    Indicates that the identity provider can export the private key
    for this identity for storage in a secure key storage service.
  */
#define KCDB_IDENT_FLAG_KEY_EXPORT  0x00004000L

/*! \brief A secure key store

    Indicates that this identity serves as a key storage service for
    any identity that can export keys.
 */
#define KCDB_IDENT_FLAG_KEY_STORE   0x00008000L

/*! \brief Don't post notifications

    No ::KMSG_IDENT notifications for ::KCDB_OP_MODIFY about identity
    property changes will be generated while this flag is set.
 */
#define KCDB_IDENT_FLAG_NO_NOTIFY   0x00010000L

/*! \brief Need to post a notification

    If any modifications are made to an identity while the
    ::KCDB_IDENT_FLAG_NO_NOTIFY flag is specified, then this flag is
    set to indicate that the notification should be sent after the
    ::KCDB_IDENT_FLAG_NO_NOTIFY flag is cleared.
 */
#define KCDB_IDENT_FLAG_NEED_NOTIFY 0x00020000L

/*! \brief Identity contains destroyable credentials */
#define KCDB_IDENT_FLAG_CRED_DEST   0x00040000L

/*! \brief Identity is initializable */
#define KCDB_IDENT_FLAG_CRED_INIT   0x00080000L

/*! \brief Read/write flags mask.

    A bitmask that correspond to all the read/write flags in the mask.
*/
#define KCDB_IDENT_FLAGMASK_RDWR    0x000fffffL

/*@}*/

/*********************************************************************/

/*! \defgroup kcdb_buf Generic access to KCDB objects

    Currently, credentials and identities both hold record data types.
    This set of API's allow an application to access fields in the
    records using a single interface.  Note that credentials only
    accept regular attributes while identities can hold both
    attributes and properties.

    Handles to credentials and identities are implicitly also handles
    to records.  Thus they can be directly used as such.
*/
/*@{*/

/*! \brief Test whether a given handle is for a credential

    The function returns \a TRUE iff \a h is a valid credential handle.
    If \a h is \a NULL the return value is \a FALSE.
 */
KHMEXP khm_boolean KHMAPI
kcdb_handle_is_cred(khm_handle h);

/*! \brief Test whether a given handle is for an identity

    The function returns \a TRUE iff \a h is a valid identity handle.
    If \a h is \a NULL the return value is \a FALSE.
 */
KHMEXP khm_boolean KHMAPI
kcdb_handle_is_identity(khm_handle h);

/*! \brief Test whether a given handle is for an identity provider

    The function returns \a TRUE iff \a h is a valid identity provider
    handle.  If \a h is \a NULL the return value is \a FALSE.
 */
KHMEXP khm_boolean KHMAPI
kcdb_handle_is_identpro(khm_handle h);

/*! \brief Get an attribute from a KCDB object by attribute id.

    \param[in] hobj Handle to the KCDB object.

    \param[in] buffer The buffer that is to receive the attribute
        value.  Set this to NULL if only the required buffer size is
        to be returned.

    \param[in,out] cbbuf The number of bytes available in \a buffer.
        If \a buffer is not sufficient, returns KHM_ERROR_TOO_LONG and
        sets this to the required buffer size.

    \param[out] attr_type Receives the data type of the attribute.
        Set this to NULL if the type is not required.

    \note Set both \a buffer and \a cbbuf to NULL if only the
        existence of the attribute is to be checked.  If the attribute
        exists in this record then the function will return
        KHM_ERROR_SUCCESS, otherwise it returns KHM_ERROR_NOT_FOUND.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_buf_get_attr(khm_handle  hobj,
                  khm_int32   attr_id, 
                  khm_int32 * attr_type, 
                  void *      buffer, 
                  khm_size *  pcb_buf);

/*! \brief Get an attribute from a record by name.

    \param[in] buffer The buffer that is to receive the attribute
        value.  Set this to NULL if only the required buffer size is
        to be returned.

    \param[in,out] cbbuf The number of bytes available in \a buffer.
        If \a buffer is not sufficient, returns KHM_ERROR_TOO_LONG and
        sets this to the required buffer size.

    \note Set both \a buffer and \a cbbuf to NULL if only the
        existence of the attribute is to be checked.  If the attribute
        exists in this record then the function will return
        KHM_ERROR_SUCCESS, otherwise it returns KHM_ERROR_NOT_FOUND.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_buf_get_attrib(khm_handle  record,
                    const wchar_t *   attr_name,
                    khm_int32 * attr_type,
                    void *      buffer,
                    khm_size *  pcb_buf);

/*! \brief Get the string representation of a record attribute.

    A shortcut function which generates the string representation of a
    record attribute directly.

    \param[in] record A handle to a record

    \param[in] attr_id The attribute to retrieve

    \param[out] buffer A pointer to a string buffer which receives the
        string form of the attribute.  Set this to NULL if you only
        want to determine the size of the required buffer.

    \param[in,out] pcbbuf A pointer to a #khm_int32 that, on entry,
        holds the size of the buffer pointed to by \a buffer, and on
        exit, receives the actual number of bytes that were copied.

    \param[in] flags Flags for the string conversion. Can be set to
        one of KCDB_TS_LONG or KCDB_TS_SHORT.  The default is
        KCDB_TS_LONG.

    \retval KHM_ERROR_SUCCESS Success
    \retval KHM_ERROR_NOT_FOUND The given attribute was either invalid
        or was not defined for this record
    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid
    \retval KHM_ERROR_TOO_LONG Either \a buffer was NULL or the
        supplied buffer was insufficient
*/
KHMEXP khm_int32 KHMAPI 
kcdb_buf_get_attr_string(khm_handle  record,
                         khm_int32   attr_id,
                         wchar_t *   buffer,
                         khm_size *  pcbbuf,
                         khm_int32  flags);

/*! \brief Get the string representation of a record attribute by name.

    A shortcut function which generates the string representation of a
    record attribute directly.

    \param[in] record A handle to a record

    \param[in] attrib The name of the attribute to retrieve

    \param[out] buffer A pointer to a string buffer which receives the
        string form of the attribute.  Set this to NULL if you only
        want to determine the size of the required buffer.

    \param[in,out] pcbbuf A pointer to a #khm_int32 that, on entry,
        holds the size of the buffer pointed to by \a buffer, and on
        exit, receives the actual number of bytes that were copied.

    \param[in] flags Flags for the string conversion. Can be set to
        one of KCDB_TS_LONG or KCDB_TS_SHORT.  The default is
        KCDB_TS_LONG.

    \see kcdb_cred_get_attr_string()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_buf_get_attrib_string(khm_handle  record,
                           const wchar_t *   attr_name,
                           wchar_t *   buffer,
                           khm_size *  pcbbuf,
                           khm_int32   flags);

/*! \brief Set an attribute in a record by attribute id

    \param[in] cbbuf Number of bytes of data in \a buffer.  The
        individual data type handlers may copy in less than this many
        bytes in to the record.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_buf_set_attr(khm_handle  record,
                  khm_int32   attr_id,
                  const void *buffer,
                  khm_size    cbbuf);

/*! \brief Set an attribute in a record by name

    \param[in] cbbuf Number of bytes of data in \a buffer.  The
        individual data type handlers may copy in less than this many
        bytes in to the record.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_buf_set_attrib(khm_handle  record,
                    const wchar_t *   attr_name,
                    const void *      buffer,
                    khm_size    cbbuf);

/*! \brief Acquire a hold on a KCDB object

    Returns \a KHM_ERROR_SUCCESS if the call is successful.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_buf_hold(khm_handle hobj);

/*! \brief Release a hold on a KCDB object

    Returns \a KHM_ERROR_SUCCESS if the call is successful.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_buf_release(khm_handle record);

/*! \brief Get the next handle in an enumeration

  \param[in] e Handle to the enumeration that was obtained via a call
    to kcdb_identpro_begin_enum() or kcdb_identity_begin_enum()

  \param[in,out] ph Receives the next object in the enumeration.
    If a valid handle of the correct type is passed in on entry, that
    handle will be released during the call.
 */
KHMEXP khm_int32 KHMAPI
kcdb_enum_next(kcdb_enumeration e, khm_handle * ph);

/*! \brief Resets or rewinds an enumeration

  Resets the state of an enumeration.  Calling kcdb_enum_next() will
  return the first handle in the list.
 */
KHMEXP khm_int32 KHMAPI
kcdb_enum_reset(kcdb_enumeration e);

/*! \brief Close an enumeration handle

  Closes a handle returned by kcdb_identpro_begin_enum().
*/
KHMEXP khm_int32 KHMAPI
kcdb_enum_end(kcdb_enumeration e);

/*! \brief Get the size of an enumeration

  Returns the number of items contained in the enumeration.
 */
KHMEXP khm_int32 KHMAPI
kcdb_enum_get_size(kcdb_enumeration e,
                   khm_size * psz);

/*! \brief Generic comparison function

  Used with kcdb_enum_sort().  The two handles \a h1 and \a h2 will be
  of the type of object being enumerated.

  The return value should be as follows:

  - less than 0 : If \a h1 should be ordered prior to \a h2

  - greater than 0 : If \a h1 should be ordered after \a h2

  - equal to 0 : If \a h1 should be considered equal to \a h2

  \note Although the function may return zero, in practice the
    comparion function will not be called if \a h1 and \a h2 are
    handles to the same identity provider.

  \note Both \a h1 and \a h2 will be valid handles.
*/
typedef khm_int32 (KHMCALLBACK * kcdb_comp_func)(khm_handle h1,
                                                 khm_handle h2,
                                                 void * vparam);

/*! \brief Sort an enumeration

  Sorts the identity providers that are contained in the enumeration
  \a e using the comparison function \a f.  The parameter \a vparam
  will be passed as-is to the comparison function.
 */
KHMEXP khm_int32 KHMAPI
kcdb_enum_sort(kcdb_enumeration e,
               kcdb_comp_func   f,
               void * vparam);


/*! \brief Generic filter function

 */
typedef khm_boolean (KHMCALLBACK * kcdb_filter_func)(khm_handle h,
                                                     void * vparam);

KHMEXP khm_int32 KHMAPI
kcdb_enum_filter(kcdb_enumeration e, kcdb_filter_func f, void * vparam);

/*@}*/

/*! \name Resource acquisition
@{*/

/* General flags */

/*! \brief Skip the cache

  If this flag is specified, kcdb_get_resource() will directly query
  the owner of the resource rather than checking in the cache.  If the
  owner provides a new resource, then the cached resource is
  discarded.
 */
#define KCDB_RF_SKIPCACHE      0x00000100

/* Flags for strings */

/*! \brief Short string */
#define KCDB_RFS_SHORT         0x00000001

/* Flags for icons */

/*! \brief Small icon

  The icon that is returned should have the dimensions corresponding
  to SM_CXSMICON and SM_CYSMICON.

  If neither KCDB_RFI_SMALL or ::KCDB_RFI_TOOLBAR are specified, the
  icon should correspond to SM_CXICON and SM_CYICON.

  \see Documentation for GetSystemMetrics() in the Microsoft Platform
  SDK.
 */
#define KCDB_RFI_SMALL         0x00000001

/*! \brief Toolbar icon

  The returned icon should be sized to fit in a toolbar.  This size is
  usually halfway between SM_CXSMICON and SM_CXICON (typically 24x24).

  If neither KCDB_RFI_SMALL or ::KCDB_RFI_TOOLBAR are specified, the
  icon should correspond to SM_CXICON and SM_CYICON.

  \see Documentation for GetSystemMetrics() in the Microsoft Platform
  SDK.
 */
#define KCDB_RFI_TOOLBAR       0x00000002

/* Return flags */

/*! \brief Do not cache

  Only applicable to string resources.  If this flag is set, then the
  returned resource should not be cached.  If the same resource is
  required again, the caller should call kcdb_get_resource() again.
 */
#define KCDB_RFR_NOCACHE       0x00000001

/*! \brief KCDB Resource IDs */
typedef enum tag_kcdb_resource_id {
    KCDB_RES_T_NONE = 0,

    KCDB_RES_T_BEGINSTRING,     /* Internal marker*/

    KCDB_RES_DISPLAYNAME,       /*!< Localized display name */
    KCDB_RES_DESCRIPTION,       /*!< Localized description */
    KCDB_RES_TOOLTIP,           /*!< A tooltip */

    KCDB_RES_INSTANCE,          /*!< Name of an instance of objects
                                  belonging to this class */

    KCDB_RES_T_ENDSTRING,       /* Internal marker */

    KCDB_RES_T_BEGINICON = 1024, /* Internal marker */

    KCDB_RES_ICON_NORMAL,       /*!< Icon (normal) */
    KCDB_RES_ICON_DISABLED,     /*!< Icon (disabled) */

    KCDB_RES_T_ENDICON,         /* Internal marker */
} kcdb_resource_id;

#define KCDB_HANDLE_FROM_CREDTYPE(t) ((khm_handle)(khm_ssize) t)

/*! \brief Acquire a resource for a KCDB Object

  This is a generic interface to obtain localized strings and icons
  for various KCDB objects.  Currently this function is supported for
  credentials, credentials types, identities and identity provider
  handles.

  When calling this function for a credentials type, the type
  identifier should be cast to a ::khm_handle type, as follows:

  \code
  kcdb_get_resource(KCDB_HANDLE_FROM_CREDTYPE(credtype_id), KCDB_RES_DISPLAYNAME, KCDB_RFS_SHORT,
                    NULL, NULL, buf, &cb_buf);
  \endcode

  \param[in] h Handle for which resources are queried. \a h could be
    one of:

    - A handle to an identity
    - A handle to an identity provider
    - A credential type identifier. (See above)

  \param[in] r_id Identifier of the resource

  \param[in] flags Additional flags.  For string resources, this is a
    combination of KCDB_RFS_* constants.  For icon resources, this is
    a combination of KCDB_RFI_* constants.

  \param[out] prflags Returned flags.  This is optional and can be set
    to NULL if no flags are desired.  Returned flags are a combination
    of KCDB_RFR_* values.

  \param[in] vparam Reserved.  Must be NULL.

  \param[out] buf Receives the resource

  \param[in,out] pcb_buf Points to a ::khm_size value that on entry,
    contains the size of the buffer pointed to by \a buf.  On exit,
    this will receieve the actual number of bytes used by the
    resource.
 */
KHMEXP khm_int32 KHMAPI
kcdb_get_resource(khm_handle h,
                  kcdb_resource_id r_id,
                  khm_int32 flags,
                  khm_int32 *prflags,
                  void * vparam,
                  void * buf,
                  khm_size * pcb_buf);

#ifdef NIMPRIVATE

KHMEXP khm_int32 KHMAPI
kcdb_cleanup_resources_by_handle(khm_handle h);

#endif

/*! \brief Resource request message

  Credentials providers receive requests for resources in the form of
  a <KMSG_CRED, KMSG_CRED_RESOURCE_REQ> message.  The \a vparam of the
  message is set to this structure.

  Identity providers receive these requests in the form of a
  <KMSG_IDENT, KMSG_IDENT_RESOURCE_REQ> message.

  Note that the \a cb_struct member will always be set to the size of
  the structure.
 */
typedef struct tag_kcdb_resource_request {
    khm_int32        magic;     /*!< Set to KCDB_RESOURCE_REQ_MAGIC */

    khm_handle       h_obj; /*!< Handle to the object being queried */
    kcdb_resource_id res_id;    /*!< Resource identifier */
    khm_restype      res_type;  /*!< Type of resource requested */
    khm_int32        flags;     /*!< Flags */
    khm_int32        rflags;    /*!< Flags to be returned */
    void *           vparam;    /*!< Parameter */
    void *           buf; /*!< Buffer which will receive the resource */
    khm_size         cb_buf;    /*!< On entry, the number of bytes
                                     available in \a buf.  On exit,
                                     this should contain the number of
                                     bytes used. */

    khm_int32        code;      /*!< Return value.  Set this to
                                     ::KHM_ERROR_SUCCESS if the call
                                     was successful, or to a suitable
                                     error code if not. */
} kcdb_resource_request;

#define KCDB_RESOURCE_REQ_MAGIC 0x31ff606d

/*@}*/

/*! \name Identity Provider Data Structures and Types
@{*/

/*! \brief Name transfer structure

    Used when the KCDB is communicating with the identity provider to
    exchange string names of identities.  See individual ::KMSG_IDENT
    message subtypes for the usage of this structure.
 */
typedef struct tag_kcdb_ident_name_xfer {
    const wchar_t * name_src;   /*!< An identity name.  Does not
                                     exceed KCDB_IDENT_MAXCCH_NAME
                                     characters including terminating
                                     NULL. */
    const wchar_t * name_alt;   /*!< An identity name.  Does not
                                     exceed KCDB_IDENT_MAXCCH_NAME
                                     characters including terminating
                                     NULL. */
    wchar_t *       name_dest;  /*!< Pointer to a buffer that is to
                                     receive a response string.  The
                                     size of the buffer in bytes is
                                     specified in \a cb_name_dest. */
    khm_size        cb_name_dest; /*!< Size of buffer pointed to by \a
                                     name_dest in bytes. */
    khm_int32       result;     /*!< Receives a result value, which is
                                     usually an error code defined in
                                     kherror.h, though it is not
                                     always. */
} kcdb_ident_name_xfer;

/*@}*/

/*! \name Identity provider interface functions

    These functions encapsulate safe calls to the current identity
    provider.  While these functions are exported, applications should
    not call these functions directly.  They are provided for use by
    the NetIDMgr core application.
@{*/

/*! \brief Find an identity provider by name

  Identity providers are named after the plug-ins that register them.
  This function returns a handle to the identity provider.  The handle
  should be released with a call to kcdb_identpro_release().

  \param[in] name Name of provider, which is the same as the name of
    the plug-in that registered the identity provider.  The name
    should be at most ::KCDB_MAXCCH_NAME characters.

  \param[out] vidpro If the call is successful, receives a held handle
    to the identity provider.
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_find(const wchar_t * name, khm_handle * vidpro);

/*! \brief Obtain the name of the identity provider

  The returned name is not localized.  It is the internal name of the
  plug-in that registered the identity provider.

  The returned name will be less than ::KCDB_MAXCCH_NAME characters.

  \param[out] pname Buffer that receives the name of the plug-in.  If
    \a pname is NULL, then the required size of the buffer will be
    returned in \a pcb_name.

  \param[in,out] pcb_name On entry, should contain the size of the
    buffer pointed to by pname.  On exit, will contain the size of the
    plug-in name including the NULL terminator.
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_name(khm_handle vidpro, wchar_t * pname, khm_size * pcb_name);

/*! \brief Get the primary credentials type for an identity provider

  The primary credentials type is registered by an identity provider
  when it first starts.  It is considered to be the type of credential
  that will form the basis for identities of this type.
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_type(khm_handle vidpro, khm_int32 * ptype);

/*! \brief Hold an identity provider

  Call kcdb_identpro_release() to undo a hold.  An identity provider
  handle should only be considered valid if it is held.  Once it is
  released, it should no longer be considered valid.
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_hold(khm_handle vidpro);

/*! \brief Release a hold obtained on an identity provider

  \see kcdb_identpro_hold()
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_release(khm_handle vidpro);

/*! \brief Begin enumerating identity providers

  This function begins an enumeration of identity provders.  If the
  call is successful, the caller will receive a pointer to an
  enumeration handle.  This handle represents a snapshot of the
  identity providers that were available at the time
  kcdb_identpro_begin_enum() was called. Once this handle is obtained,
  kcdb_enum_next(), kcdb_enum_reset() can be used to navigate the
  enumeration as follows:

  \code

  {
    kcdb_identpro_enumeration e = NULL;
    khm_size   n_p;
    khm_handle h_idpro = NULL;

    if (KHM_SUCCEEDED(kcdb_identpro_begin_enum(&e, &n_p))) {

      // Now n_p has the number of providers that are in the
      // enumeration, although we won't be using it.

      // Note that h_idpro is initialized to NULL above.
      while (KHM_SUCCEEDED(kcdb_enum_next(e, &h_idpro))) {

        // Do something with h_idpro

        // The handle in h_idpro will be automatically released during
        // the next call to kcdb_enum_next().

      }

      kcdb_enum_end(e);

      // h_idpro is guaranteed to be NULL at this point, since the last
      // call to kcdb_identpro_enum_next() failed.

    }
  }

  \endcode

  The handle to the enumeration that is returned in \a e should be
  closed by a call to kcdb_enum_end().

  \param[out] e Receives a handle to the identity provider enumeration

  \param[out] n_providers Receives the number of identity providers
    found.  Optional.  Set to NULL if this value is not required.

  \retval KHM_ERROR_SUCCESS The call succeeded.  \a e and \a
    n_providers will contain values as documented above.

  \retval KHM_ERROR_NOT_FOUND There are no identity providers to
    enumerate.

  \see kcdb_enum_next(), kcdb_enum_sort(), kcdb_enum_reset() and
  kcdb_enum_end()

  \note Enumeration handles should not be shared across thread
    boundaries.
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_begin_enum(kcdb_enumeration * e, khm_size * n_providers);

/*! \brief Query the default identity provider

  If the call is successful, vidpro recieves a held handle to the
  current default identity provider.
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_default(khm_handle * vidpro);

/*! \brief Check if two identity provider handles refer to the same provider
 */
KHMEXP khm_boolean KHMAPI
kcdb_identpro_is_equal(khm_handle idp1, khm_handle idp2);

#ifdef NIMPRIVATE

/* The follwing functions are only exported for use by the Network
   Identity Provider application, and are not meant for general use
   by plug-ins. */

KHMEXP khm_int32 KHMAPI 
kcdb_identpro_validate_name(const wchar_t * name);
#pragma deprecated(kcdb_identpro_validate_name)

/*! \brief Validate an identity name

    The name that is provided will be passed through sets of
    validations.  One set, which doesn't depend on the identity
    provider checks whether the length of the identity name and
    whether there are any invalid characters in the identity name.  If
    the name passes those tests, then the name is passed down to the
    identity provider's name validation handler.

    \param[in] vidpro Handle to the identity provider

    \param[in] name Identity name to validate

    \retval KHM_ERROR_SUCCESS The name is valid

    \retval KHM_ERROR_TOO_LONG Too many characters in name

    \retval KHM_ERROR_INVALID_NAME There were invalid characters in the name.

    \retval KHM_ERROR_NO_PROVIDER There is no identity provider;
        however the name length is valid.

    \retval KHM_ERROR_NOT_IMPLEMENTED The identity provider doesn't
        implement a name validation handler; however the name passed
        the length and character tests.

    \see ::KMSG_IDENT_VALIDATE_NAME
 */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_validate_name_ex(khm_handle vidpro, const wchar_t * name);

#ifdef KH_API_REMOVED
/*! \brief Validate an identity

    The identity itself needs to be validated.  This may involve
    communicating with an external entity.

    \see ::KMSG_IDENT_VALIDATE_IDENTITY
 */
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_validate_identity(khm_handle identity);
#endif

KHMEXP khm_int32 KHMAPI 
kcdb_identpro_canon_name(const wchar_t * name_in, 
                         wchar_t * name_out, 
                         khm_size * cb_name_out);
#pragma deprecated(kcdb_identpro_canon_name)

/*! \brief Canonicalize the name 

    \see ::KMSG_IDENT_CANON_NAME
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_canon_name_ex(khm_handle vidpro,
                            const wchar_t * name_in, 
                            wchar_t * name_out, 
                            khm_size * cb_name_out);

KHMEXP khm_int32 KHMAPI 
kcdb_identpro_compare_name(const wchar_t * name1,
                           const wchar_t * name2);
#pragma deprecated(kcdb_identpro_compare_name)

/*! \brief Compare two identity names 

    \see ::KMSG_IDENT_COMPARE_NAME
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_compare_name_ex(khm_handle vidpro,
                              const wchar_t * name1,
                              const wchar_t * name2);

/*! \brief Set the specified identity as the default 

    \see ::KMSG_IDENT_SET_DEFAULT
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_set_default_identity(khm_handle identity, khm_boolean ask_id_pro);

/*! \brief Get the default identity for the specified provider */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_default_identity(khm_handle vidpro, khm_handle * vident);

/*! \brief Set the specified identity as searchable 

    \see ::KMSG_IDENT_SET_SEARCHABLE
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_set_searchable(khm_handle identity,
                             khm_boolean searchable);

/*! \brief Update the specified identity 

    \see ::KMSG_IDENT_UPDATE
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_update(khm_handle identity);

KHMEXP khm_int32 KHMAPI 
kcdb_identpro_get_ui_cb(void * rock);
#pragma deprecated(kcdb_identpro_get_ui_cb)

/*! \brief Notify an identity provider of the creation of a new identity 

    \see ::KMSG_IDENT_NOTIFY_CREATE
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_notify_create(khm_handle identity);

/*! \brief Notify an identity provider of the creation of a configuration space for an identity

    \see ::KMSG_IDENT_NOTIFY_CONFIG
 */
KHMEXP khm_int32 KHMAPI 
kcdb_identpro_notify_config_create(khm_handle identity);

#endif  /* NIMPRIVATE */

/*@}*/

/*! \brief Check if the given name is a valid identity name

    \return TRUE or FALSE to the question, is this valid?
*/
KHMEXP khm_boolean KHMAPI 
kcdb_identity_is_valid_name(const wchar_t * name);
#pragma deprecated(kcdb_identity_is_valid_name)

/*! \brief Create or open an identity

    Similar to kcdb_identity_create_ex(), except the name of the
    identity can optionally include the name of the identity provider
    to use.  In this case, the format of the \a name parameter would
    be:

    <i>(provider name):(identity name)</i>

    If the provider name is not present, the default identity provider
    is assumed.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_create(const wchar_t *name, 
                     khm_int32 flags, 
                     khm_handle * result);

/*! \brief Create or open an identity.

    If the ::KCDB_IDENT_FLAG_CREATE flag is specified in the flags
    parameter, a new identity will be created if one does not already
    exist with the given name for the identity provider.  If an
    identity by that name already exists, then the existing identity
    will be opened. The result parameter will receive a held reference
    to the opened identity.  Use kcdb_identity_release() to release
    the handle.

    \param[in] vidpro Handle to the identity provider

    \param[in] name Name of identity to create. Should be at most
        ::KCDB_IDENT_MAXCCH_NAME characters including the NULL
        terminator.  The name provided here will not be considered to
        be a display name and hence is not required to be localized.
        A localized display name can be set for an identity by setting
        the ::KCDB_ATTR_DISPLAY_NAME attribute.

    \param[in] flags If ::KCDB_IDENT_FLAG_CREATE is specified, then
        the identity will be created if it doesn't already exist.
        Additional flags can be set here which will be assigned to the
        identity if it is created.  Additional flags have no effect if
        an existing identity is opened.

        Specifying ::KCDB_IDENT_FLAG_CONFIG here will create a
        configuration space for the identity if this is a new
        identity.  If the identity already exists and is active, a
        configuration space will not be created.

    \param[in] vparam Additional parameter for identity creation.  If
        the name of the identity is not sufficient to uniquely
        identify the identity or if additional information is required
        for the identity provider to create the identity, then this
        parameter can be used to provide that information.  The
        supplied parameter is assigned to the ::KCDB_ATTR_PARAM
        attribute of the identity before the <::KMSG_IDENT,
        ::KMSG_IDENT_NOTIFY_CREATE> notification is sent out.

    \param[out] result If the call is successful, this receives a held
        reference to the identity.  The caller should call
        kcdb_identity_release() to release the identity once it is no
        longer needed.

    \note Identity names are only unique per identity provider.

    */
KHMEXP khm_int32 KHMAPI
kcdb_identity_create_ex(khm_handle vidpro,
                        const wchar_t * name,
                        khm_int32 flags,
                        void * vparam,
                        khm_handle * result);

/*! \brief Mark an identity for deletion.

    The identity will be marked for deletion.  The
    ::KCDB_IDENT_FLAG_ACTIVE will no longer be present for this
    identity.  Once all references to the identity are released, it
    will be removed from memory.  All associated credentials will also
    be removed. */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_delete(khm_handle id);

/*! \brief Set or unset the specified flags in the specified identity.

    Only flags that are in ::KCDB_IDENT_FLAGMASK_RDWR can be specifed
    in the \a flags parameter or the \a mask parameter.  The flags set
    in the \a mask parameter of the identity will be set to the
    corresponding values in the \a flags parameter.

    If ::KCDB_IDENT_FLAG_INVALID is set using this function, then the
    ::KCDB_IDENT_FLAG_VALID will be automatically reset, and vice
    versa.  Resetting either bit does not undo this change, and will
    leave the identity's validity unspecified.  Setting either of
    ::KCDB_IDENT_FLAG_INVALID or ::KCDB_IDENT_FLAG_VALID will
    automatically reset ::KCDB_IDENT_FLAG_UNKNOWN.

    Note that setting or resetting certain flags have other semantic
    side-effects:

    - ::KCDB_IDENT_FLAG_DEFAULT : Setting this is equivalent to
      calling kcdb_identity_set_default() with \a id.  Resetting this
      is equivalent to calling kcdb_identity_set_default() with NULL.

    - ::KCDB_IDENT_FLAG_SEARCHABLE : Setting this will result in the
      identity provider getting notified of the change. If the
      identity provider indicates that searchable flag should not be
      set or reset on the identity, then kcdb_identity_set_flags()
      will return an error.

    \note kcdb_identity_set_flags() is not atomic.  Even if the
      function returns a failure code, some flags in the identity may
      have been set.  If the call to kcdb_identity_set_flags() fails,
      the caller should check the flags in the identity using
      kcdb_identity_get_flags() to check which flags have been set and
      which have failed.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_flags(khm_handle id, 
                        khm_int32 flags,
                        khm_int32 mask);

/*! \brief Return all the flags for the identity

    \note The returned flags may include internal flags.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_flags(khm_handle id, 
                        khm_int32 * flags);

KHMEXP khm_int32 KHMAPI
kcdb_identity_get_parent(khm_handle vid,
                         khm_handle *pvparent);

KHMEXP khm_int32 KHMAPI
kcdb_identity_set_parent(khm_handle vid,
                         khm_handle vparent);

/*! \brief Return the name of the identity 

    The returned name is not a localized display name.  To obtain the
    localized display name for an identity, query the
    ::KCDB_ATTR_DISPLAY_NAME attribute, or use the kcdb_get_resource()
    function.

    \param[out] buffer Buffer to copy the identity name into.  The
        maximum size of an identity name is \a KCDB_IDENT_MAXCB_NAME.
        If \a buffer is \a NULL, then the required size of the buffer
        is returned in \a pcbsize.

    \param[in,out] pcbsize Size of buffer in bytes.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_name(khm_handle id, 
                       wchar_t * buffer, 
                       khm_size * pcbsize);

/*! \brief Set the specified identity as the default

    Each identity provider maintains a default identity.  Setting an
    identity as the default will make it the default identity for the
    corresponding identity provider.  Other providers will be
    unaffected.

    \see kcdb_identity_set_flags(), kcdb_identity_set_default_int()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_default(khm_handle id);

/*! \brief Mark the specified identity as the default.

    This API is reserved for use by identity providers as a means of
    specifying which identity is default.  The difference between
    kcdb_identity_set_default() and kcdb_identity_set_default_int() is
    in semantics.  

    - kcdb_identity_set_default() is used to request the KCDB to
      designate the specified identity as the default.  When
      processing the request, the KCDB invokes the identity provider
      to do the necessary work to make the identity the default.

    - kcdb_identity_set_default_int() is used by the identity provider
      to notify the KCDB that the specified identity is the default.
      This does not result in the invocation of any other semantics to
      make the identity the default other than releasing the previous
      default identity and making the specified one the default.
 */
KHMEXP khm_int32 KHMAPI
kcdb_identity_set_default_int(khm_handle id);

/*! \deprecated Use kcdb_identity_get_default_ex() instead. */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_default(khm_handle * pvid);
#pragma deprecated(kcdb_identity_get_default)

/*! \brief Get the default identity

    Obtain a held handle to the default identity if there is one.  The
    handle must be freed using kcdb_identity_release().

    If there is no default identity, then the handle pointed to by \a
    pvid is set to \a NULL and the function returns
    KHM_ERROR_NOT_FOUND. */
KHMEXP khm_int32 KHMAPI
kcdb_identity_get_default_ex(khm_handle vidpro, khm_handle * pvid);

/*! \brief Get the configuration space for the identity. 

    If the configuration space for the identity does not exist and the
    flags parameter does not specify ::KHM_FLAG_CREATE, then the
    function will return a failure code as specified in
    ::khc_open_space().  Depending on whether or not a configuration
    space was found, the ::KCDB_IDENT_FLAG_CONFIG flag will be set or
    reset for the identity.

    \param[in] id Identity for which the configuraiton space is requested

    \param[in] flags Flags used when calling khc_open_space().  If \a
        flags specifies KHM_FLAG_CREATE, then the configuration space
        is created.

    \param[out] result The resulting handle.  If the call is
        successful, this receives a handle to the configuration space.
        Use khc_close_space() to close the handle.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_config(khm_handle id,
                         khm_int32 flags,
                         khm_handle * result);

/*! \brief Hold a reference to an identity.

    A reference to an identity (a handle) is only valid while it is
    held.  \note Once the handle is released, it can not be
    revalidated by calling kcdb_identity_hold().  Doing so would lead
    to unpredictable consequences. */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_hold(khm_handle id);

/*! \brief Release a reference to an identity.
    \see kcdb_identity_hold() */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_release(khm_handle id);

#ifdef NIMPRIVATE
/*! \brief Set the identity provider subscription

    If there was a previous subscription, that subscription will be
    automatically deleted.

    \param[in] sub New identity provider subscription
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_provider(khm_handle sub);
#endif

/*! \brief Set the primary credentials type

    The primary credentials type is designated by the identity
    provider.  As such, this function should only be called by an
    identity provider.

    The \a cred_type will become the primary credentials type for the
    identity provider that calls this function as identified by the
    calling thread.

    \note It is invalid to call this function from outside an identity
      provider plug-in thread.

*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_type(khm_int32 cred_type);

/*! \brief Retrieve the identity provider subscription

    \param[out] sub Receives the current identity provider
        subscription.  Set to NULL if only the existence of an
        identity provider needs to be checked.

    \retval KHM_ERROR_SUCCESS An identity provider exists.  If \a sub
        was not NULL, the subscription has been copied there.

    \retval KHM_ERROR_NOT_FOUND There is currently no registered
        identity provider.  If \a sub was not NULL, the handle it
        points to has been set to NULL.

    \deprecated This function should no longer be used.  Plug-ins must
        not send messages directly to identity providers.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_provider(khm_handle * sub);
#pragma deprecated(kcdb_identity_get_provider)

/*! \brief Retrieve the identity provider for an identity

    Returns a handle to the identity provider that provided this
    identity.  If the call is successful, the returned handle must be
    released wit ha call to kcdb_identpro_release().
 */
KHMEXP khm_int32 KHMAPI
kcdb_identity_get_identpro(khm_handle h_ident, khm_handle * h_identpro);

/*! \brief Checks whether the given identity was provided by the named provider

    Return TRUE if the provider of \a h_ident is \a provider_name.
    FALSE otherwise.  If the identity is invalid, the return value is
    FALSE.
 */
KHMEXP khm_boolean KHMAPI
kcdb_identity_by_provider(khm_handle h_ident, const wchar_t * provider_name);

/*! \brief Retrieve the identity provider credentials type

    This is the credentials type that the identity provider has
    designated as the primary credentials type.

    \deprecated Use kcdb_identpro_get_type() instead.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_type(khm_int32 * ptype);
#pragma deprecated(kcdb_identity_get_type)

/*! \brief Returns TRUE if the two identities are equal

    Also returns TRUE if both identities are NULL.
 */
KHMEXP khm_boolean KHMAPI
kcdb_identity_is_equal(khm_handle identity1,
                       khm_handle identity2);

/*! \brief Set an attribute in an identity by attribute id

    \param[in] buffer A pointer to a buffer containing the data to
        assign to the attribute.  Setting \a buffer to NULL has the
        effect of removing any data that is already assigned to the
        attribute.  If \a buffer is non-NULL, then \a cbbuf should
        specify the number of bytes in \a buffer.

    \param[in] cbbuf Number of bytes of data in \a buffer.  The
        individual data type handlers may copy in less than this many
        bytes in to the credential.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_attr(khm_handle identity,
                       khm_int32 attr_id,
                       const void *buffer,
                       khm_size cbbuf);

/*! \brief Set an attribute in an identity by name

    The attribute name has to be a KCDB registered attribute or
    property.

    \param[in] cbbuf Number of bytes of data in \a buffer.  The
        individual data type handlers may copy in less than this many
        bytes in to the credential.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_attrib(khm_handle identity,
                         const wchar_t * attr_name,
                         const void * buffer,
                         khm_size cbbuf);

/*! \brief Get an attribute from an identity by attribute id.

    \param[in] buffer The buffer that is to receive the attribute
        value.  Set this to NULL if only the required buffer size is
        to be returned.

    \param[in,out] cbbuf The number of bytes available in \a buffer.
        If \a buffer is not sufficient, returns KHM_ERROR_TOO_LONG and
        sets this to the required buffer size.

    \param[out] attr_type Receives the data type of the attribute.
        Set this to NULL if the type is not required.

    \note Set both \a buffer and \a cbbuf to NULL if only the
        existence of the attribute is to be checked.  If the attribute
        exists in this identity then the function will return
        KHM_ERROR_SUCCESS, otherwise it returns KHM_ERROR_NOT_FOUND.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attr(khm_handle identity,
                       khm_int32 attr_id,
                       khm_int32 * attr_type,
                       void * buffer,
                       khm_size * pcbbuf);

/*! \brief Get an attribute from an identity by name.

    \param[in] buffer The buffer that is to receive the attribute
        value.  Set this to NULL if only the required buffer size is
        to be returned.

    \param[in,out] cbbuf The number of bytes available in \a buffer.
        If \a buffer is not sufficient, returns KHM_ERROR_TOO_LONG and
        sets this to the required buffer size.

    \note Set both \a buffer and \a cbbuf to NULL if only the
        existence of the attribute is to be checked.  If the attribute
        exists in this identity then the function will return
        KHM_ERROR_SUCCESS, otherwise it returns KHM_ERROR_NOT_FOUND.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attrib(khm_handle identity,
                         const wchar_t * attr_name,
                         khm_int32 * attr_type,
                         void * buffer,
                         khm_size * pcbbuf);

/*! \brief Get the string representation of an identity attribute.

    A shortcut function which generates the string representation of
    an identity attribute directly.

    \param[in] identity A handle to an identity

    \param[in] attr_id The attribute to retrieve

    \param[out] buffer A pointer to a string buffer which receives the
        string form of the attribute.  Set this to NULL if you only
        want to determine the size of the required buffer.

    \param[in,out] pcbbuf A pointer to a #khm_int32 that, on entry,
        holds the size of the buffer pointed to by \a buffer, and on
        exit, receives the actual number of bytes that were copied.

    \param[in] flags Flags for the string conversion. Can be set to
        one of KCDB_TS_LONG or KCDB_TS_SHORT.  The default is
        KCDB_TS_LONG.

    \retval KHM_ERROR_SUCCESS Success
    \retval KHM_ERROR_NOT_FOUND The given attribute was either invalid
        or was not defined for this identity
    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid
    \retval KHM_ERROR_TOO_LONG Either \a buffer was NULL or the
        supplied buffer was insufficient
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attr_string(khm_handle identity,
                              khm_int32 attr_id,
                              wchar_t * buffer,
                              khm_size * pcbbuf,
                              khm_int32 flags);

/*! \brief Get the string representation of an identity attribute by name.

    A shortcut function which generates the string representation of
    an identity attribute directly.

    \param[in] identity A handle to an identity

    \param[in] attrib The name of the attribute to retrieve

    \param[out] buffer A pointer to a string buffer which receives the
        string form of the attribute.  Set this to NULL if you only
        want to determine the size of the required buffer.

    \param[in,out] pcbbuf A pointer to a #khm_int32 that, on entry,
        holds the size of the buffer pointed to by \a buffer, and on
        exit, receives the actual number of bytes that were copied.

    \param[in] flags Flags for the string conversion. Can be set to
        one of KCDB_TS_LONG or KCDB_TS_SHORT.  The default is
        KCDB_TS_LONG.

    \see kcdb_identity_get_attr_string()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attrib_string(khm_handle identity,
                                const wchar_t * attr_name,
                                wchar_t * buffer,
                                khm_size * pcbbuf,
                                khm_int32 flags);

/*! \brief Enumerate identities

    \deprecated Use kcdb_identity_begin_enum() et. al. instead.
  */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_enum(khm_int32 and_flags,
                   khm_int32 eq_flags,
                   wchar_t * name_buf,
                   khm_size * pcb_buf,
                   khm_size * pn_idents);
#pragma deprecated(kcdb_identity_enum)

/*! \brief Refresh identity attributes based on root credential set

    Several flags in an identity are dependent on the credentials that
    are associated with it in the root credential set.  In addition,
    other flags in an identity depend on external factors that need to
    be verfied once in a while.  This API goes through the root
    credential set as well as consulting the identity provider to
    update an identity.

    \see kcdb_identity_refresh()
 */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_refresh(khm_handle vid);

/*! \brief Refresh all identities

    Equivalent to calling kcdb_identity_refresh() for all active
    identities.

    \see kcdb_identityt_refresh()
 */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_refresh_all(void);

/*! \brief Query the serial number for the identity

    Each identity is assigned a serial number at the time it is
    created.  The following properties are guaranteed for identity
    serial numbers:

    - Will be associated with the identity represented by \a vid as
      long as \a vid is held.

    - Will not be used to identify a different identity during this
      session.

    However, note the following:

    - If the hold on the identity is released, and the identity is
      recreated, it may be assigned a new serial number.  The serial
      number is only associated with the identity for the lifetime of
      the identity object, which might end once you release the hold.

    \param[in] vid Handle to the identity
    \param[out] pserial Receives the serial number

 */
KHMEXP khm_int32 KHMAPI
kcdb_identity_get_serial(khm_handle vid, khm_ui_8 * pserial);

/*! \brief Retrieves a short name representing an identity

  The generated name is used by several different functions that
  place several constraints on the generated name.  To satisfy these,
  the following rules are followed:

  1. If the identity was provided by a provider named "Krb5Ident",
     then the generated name will be just the identity name.  If the
     generated name is longer than KCDB_MAXCCH_NAME, then rule #3 is
     applied.

  2. If the identity was not provided by "Krb5Ident", then the
     generated name will be of the form
     "<provider-name>:<identity-name>".  If the generated name exceeds
     KCDB_MAXCCH_NAME, then rule #3 is applied.

  3. If #1 and #2 fail to produce a suitable name, then the generated
     name will be of the form "NIMIdentity:<serial-number>", where
     "<serial-number>" will be the upper-case hexadecimal
     representation of the serial number of the identity.

  The current constraints are:

  - The name should uniquely identify an identity.

  - Passing an unescaped named into kcdb_identity_create() should open
    the identity if it is still active.

  - The escaped name should be usable as a configuration space name.

  - The unescaped name should be usable as a configuration node name.

  \param[in] vid Handle to the identity

  \param[in] escape_chars Set to \a TRUE if special characters in the
      generated name should be escaped.  The special characters that
      are currently recognized are '#' and '\'.

  \param[out] buf Receives the generated short name.

  \param[in,out] pcb_buf On entry, specifies the size of \a buf in
      bytes. On exit, specifies how many bytes were used (including
      the trailing NULL).
 */
KHMEXP khm_int32 KHMAPI
kcdb_identity_get_short_name(khm_handle vid, khm_boolean escape_chars,
                             wchar_t * buf, khm_size * pcb_buf);

/*! \brief Begin enumerating identities

  This function begins an enumeration of identities.  If the call is
  successful, the caller will receive a pointer to an enumeration
  handle.  This handle represents a snapshot of the identities that
  were available at the time kcdb_identity_begin_enum() was called and
  which passed the filter criteria.  The filter criteria is based on
  the flags of the identity as follows:

  \code
  (identity->flags & and_flags) == (eq_flags & and_flags)
  \endcode

  Essentially, if a flag is set in \a and_flags, then that flag in
  the identity should equal the setting in \a eq_flags.

  Only active identities are considered as candidates for enumeration
  (see ::KCDB_IDENT_FLAG_ACTIVE).

  The generic enumeration functions kcdb_enum_next(),
  kcdb_enum_sort(), kcdb_enum_reset() can be used to navigate and
  manage the enumeration.

  \code

  {
    kcdb_enumeration e = NULL;
    khm_size   n_id;
    khm_handle h_id = NULL;

    if (KHM_SUCCEEDED(kcdb_identity_begin_enum(KCDB_IDENT_FLAG_STICKY,
                                               KCDB_IDENT_FLAG_STICKY,
                                               &e, &n_id))) {

      // Now n_id has the number of sticky active identities that were
      // included in the enumeration, although we won't be using it.

      // Note that h_id is initialized to NULL above.
      while (KHM_SUCCEEDED(kcdb_enum_next(e, &h_id))) {

        // Do something with h_id

        // The handle in h_id will be automatically released during
        // the next call to kcdb_enum_next().

      }

      kcdb_enum_end(e);

      // h_id is guaranteed to be NULL at this point, since the last
      // call to kcdb_enum_next() failed.

    }
  }

  \endcode

  The handle to the enumeration that is returned in \a e should be
  closed by a call to kcdb_enum_end().

  \param[out] e Receives a handle to the identity enumeration

  \param[out] p_nidentities Receives the number of identities found.
    This parameter is optional.  Set to NULL if this value is not
    required.

  \retval KHM_ERROR_SUCCESS The call succeeded.

  \retval KHM_ERROR_NOT_FOUND No identities found that match the
    criteria.

  \see kcdb_enum_next(), kcdb_enum_reset(), kcdb_enum_end(), kcdb_enum_sort()

  \note Enumeration handles should not be shared across thread
    boundaries.
    
 */
KHMEXP khm_int32 KHMAPI
kcdb_identity_begin_enum(khm_int32 and_flags,
                         khm_int32 eq_flags,
                         kcdb_enumeration * e,
                         khm_size * pn_identites);

/* KSMG_KCDB_IDENT notifications are structured as follows:
   type=KMSG_KCDB
   subtype=KMSG_KCDB_IDENT
   uparam=one of KCDB_OP_*
   blob=handle to identity in question */

/*@}*/


/*********************************************************************/


/*!
\defgroup kcdb_creds Credential sets and individual credentials

@{
*/


/*! \brief Credentials process function

    This function is called for each credential in a credential set
    when supplied to kcdb_credset_apply().  It should return
    KHM_ERROR_SUCCESS to continue the operation, or any other value to
    terminate the processing.

    \see kcdb_credset_apply()
*/
typedef khm_int32 
(KHMCALLBACK *kcdb_cred_apply_func)(khm_handle cred, 
                                    void * rock);

/*! \brief Credentials filter function.

    Should return non-zero if the credential passed as \a cred is to
    be "accepted".  The precise consequence of a non-zero return value
    is determined by the individual function that this call back is
    passed into.

    This function should not call any other function which may modify
    \a cred.

    \see kcdb_credset_collect_filtered()
    \see kcdb_credset_extract_filtered()
*/
typedef khm_int32 
(KHMCALLBACK *kcdb_cred_filter_func)(khm_handle cred, 
                                     khm_int32 flags, 
                                     void * rock);

/*! \defgroup kcdb_credset Credential sets */
/*@{*/

/*! \brief Create a credential set.

    Credential sets are temporary containers for credentials.  These
    can be used by plug-ins to store credentials while they are being
    enumerated from an external source.  Once all the credentials have
    been collected into the credential set, the plug-in may call
    kcdb_credset_collect() to collect the credentials into the root
    credential store.

    The user interface will only display credentials that are in the
    root credential store.  No notifications are generated for changes
    to a non-root credential set.

    Use kcdb_credset_delete() to delete the credential set once it is
    created.

    \see kcdb_credset_delete()
    \see kcdb_credset_collect()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_create(khm_handle * result);

/** \brief Delete a credential set

    \see kcdb_credset_create()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_delete(khm_handle credset);

/** \brief Collect credentials from a credential set to another credential set.

    Collecting a subset of credentials from credential set \a cs_src
    into credential set \a cs_dest involves the following steps:

    - Select all credentials from \a cs_src that matches the \a
      identity and \a type specified in the function call and add them
      to the \a cs_dest credential set if they are not there already.
      Note that if neither credential set is not the root credential
      store, then the credentials will be added by reference, while if
      it is the root credential store, the credentials will be
      duplicated, and the copies will be added to \a cs_dest.

    - If a selected credential in \a cs_src already exists in \a
      cs_dest, then update the credential in \a cs_dest with the
      credential fields in \a cs_src.  In other words, once a
      credential is found to exist in both \a cs_src and \a cs_dest,
      all the non-null fields from the credential in \a cs_src will be
      copied to the credential in \a cs_dest.  Fields which are null
      (undefined) in \a cs_src and are non-null in \a cs_dest will be
      left unmodified in \a cs_dest.

      One notable exception is the credentials' flags.  All flags in
      \a cs_src which are not included in
      ::KCDB_CRED_FLAGMASK_ADDITIVE will be copied to the
      corresponding bits in the flags of \a cs_dest.  However, flags
      that are included in ::KCDB_CRED_FLAGMASK_ADDITIVE will be added
      to the corresponding bits in \a cs_dest.

      (See notes below)

    - Remove all credentials from \a cs_dest that match the \a
      identity and \a type that do not appear in \a cs_src. (see notes
      below)

    For performance reasons, plugins should use kcdb_credset_collect()
    to update the root credentials store instead of adding and
    removing individual credentials from the root store.

    Only credentials that are associated with active identities are
    affected by kcdb_credset_collect().

    \param[in] cs_dest A handle to the destination credential set.  If
        this is \a NULL, then it is assumed to refer to the root
        credential store.

    \param[in] cs_src A handle to the source credential set.  If this
        is NULL, then it is assumed to refer to the root credential
        store.

    \param[in] identity A handle to an identity.  Setting this to NULL
        collects all identities in the credential set.

    \param[in] type A credentials type.  Setting this to
        KCDB_CREDTYPE_ALL collects all credential types in the set.

    \param[out] delta A bit mask that indicates the modifications that
        were made to \a cs_dest as a result of the collect operation.
        This is a combination of KCDB_DELTA_* values.  This parameter
        can be \a NULL if the value is not required.

    \warning If \a identity and \a type is set to a wildcard, all
        credentials in the root store that are not in this credentials
        set will be deleted.

    \note Two credentials \a A and \a B are considered equal if:
        - They refer to the same identity
        - Both have the same credential type
        - Both have the same name
        - A comparison function supplied by the credentials provider
          indicates that the credentials are equal. (see
          kcdb_credtype_register())

    \note This is the only supported way of modifying the root
        credential store.

    \note \a cs_src and \a cs_dest can not refer to the same
        credentials set.

    \note The destination credential set cannot be sealed.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_collect(khm_handle cs_dest,
                     khm_handle cs_src,
                     khm_handle identity, 
                     khm_int32 type,
                     khm_int32 * delta);

/*! \brief Credentials were added
    \see kcdb_credset_collect() */
#define KCDB_DELTA_ADD      1

/*! \brief Credentials were deleted 
    \see kcdb_credset_collect() */
#define KCDB_DELTA_DEL      2

/*! \brief Credentials were modified
    \see kcdb_credset_collect() */
#define KCDB_DELTA_MODIFY   4

/*! \brief Indicates that the credential to be filtered is from the root store.

    \see kcdb_credset_collect_filtered()
*/
#define KCDB_CREDCOLL_FILTER_ROOT   1

/*! \brief Indicates that the credential to be filtered is from the source
        credential set 
        
    \see kcdb_credset_collect_filtered() */
#define KCDB_CREDCOLL_FILTER_SRC    2

/*! \brief Indicates that the credential to be filtered is from the destination
        credential set 
        
    \see kcdb_credset_collect_filtered() */
#define KCDB_CREDCOLL_FILTER_DEST   4

/*! \brief Collect credentials from one credential set to another using a filter.

    Similar to kcdb_credset_collect() except instead of selecting
    credentials by matching against an identity and/or type, a filter
    function is called.  If the filter function returns non-zero for a
    credential, that credential is selected.

    Credentials in the source and destination credential sets are
    passed into the filter function.  Depending on whether the
    credential is in the source credential set or destination
    credential set, the \a flag parameter may have either \a
    KCDB_CREDCOLL_FILTER_SRC or \a KCDB_CREDCOLL_FILTER_DEST bits set.
    Also, if either one of the credential sets is the root credential
    store, then additionally \a KCDB_CREDCOLL_FILTER_ROOT would also
    be set.

    See the kcdb_credset_collect() documentation for explanations of
    the \a cs_src, \a cs_dest and \a delta parameters which perform
    identical functions.

    \param[in] filter The filter of type ::kcdb_cred_filter_func
    \param[in] rock A custom argument to be passed to the filter function.

    \see kcdb_credset_collect()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_collect_filtered(khm_handle cs_dest,
                              khm_handle cs_src,
                              kcdb_cred_filter_func filter,
                              void * rock,
                              khm_int32 * delta);

/*! \brief Flush all credentials from a credential set

    Deletes all the crednetials from the credential set.

    \param[in] credset A handle to a credential set.  Cannot be NULL.

    \note The credential set cannot be sealed
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_flush(khm_handle credset);

/*! \brief Extract credentials from one credential set to another

    Credentials from the source credential set are selected based on
    the \a identity and \a type arguements.  If a credential is
    matched, then it is added to the \a destcredset.

    If the \a sourcecredset is the root credential set, the added
    credentials are copies of the actual credentials in the root
    credential set.  Otherwise the credentials are references to the
    original credentials in the \a sourcecredset .

    \param[in] destcredset Destination credential set.  Must be valid.

    \param[in] sourcecredset The source credential set.  If set to
        NULL, extracts from the root credential set.

    \param[in] identity The identity to match in the source credential
        set.  If set to NULL, matches all identities.

    \param[in] type The credential type to match in the source credential set.
        If set to KCDB_CREDTYPE_INVALID, matches all types.

    \note This function does not check for duplicate credentials.

    \note The destination credential set cannot be sealed.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_extract(khm_handle destcredset, 
                     khm_handle sourcecredset, 
                     khm_handle identity, 
                     khm_int32 type);

/*! \brief Extract credentials from one credential set to another using a filter.

    Similar to kcdb_credset_extract() except a filter function is used
    to determine which credentials should be selected.

    \param[in] rock A custom argument to be passed in to the filter function.

    \note The destination credential set cannot be sealed.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_extract_filtered(khm_handle destcredset,
                              khm_handle sourcecredset,
                              kcdb_cred_filter_func filter,
                              void * rock);

/*! \brief Retrieve a held reference to a credential in a credential set based on index.

    \param[in] idx The index of the credential to retrieve.  This is a
        zero based index which goes from 0 ... (size of credset - 1).

    \param[out] cred The held reference to a credential.  Call 
        kcdb_cred_release() to release the credential.

    \retval KHM_ERROR_SUCCESS Success. \a cred has a held reference to the credential.
    \retval KHM_ERROR_OUT_OF_BOUNDS The index specified in \a idx is out of bounds.
    \retval KHM_ERROR_DELETED The credential at index \a idx has been marked as deleted.

    \see kcdb_cred_release()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_get_cred(khm_handle credset,
                      khm_int32 idx,
                      khm_handle * cred);

/*! \brief Search a credential set for a specific credential

    The credential set indicated by \a credset is searched for a
    credential that satisfies the predicate function \a f.  Each
    credential starting at \a idx_start is passed into the predicate
    function until it returns a non-zero value.  At this point, that
    credential is passed in to the \a cred parameter, and the index of
    the credential is passed into the \a idx parameter.

    \param[in] credset The credential set to search on.  Specify NULL
        if you want to search teh root credential set.

    \param[in] idx_start The index at which to start the search after.
        The first credential passed to the predicate function will be
        at \a idx_start + 1.  Specify -1 to start from the beginning
        of the credential set.

    \param[in] f The predicate function.  The \a flags parameter of
        the predicate function will always receive 0.

    \param[in] rock An opaque parameter to be passed to the predicate
        function \a f.

    \param[out] cred A held reference to the credential that satisfied
        the predicate function or NULL if no such credential was
        found.  Note that if a valid credential is returned, the
        calling function must release the credential using
        kcdb_cred_release().

    \param[out] idx The index of the credential passed in \a cred.
        Specify NULL if the index is not required.

    \retval KHM_ERROR_SUCCESS A credential that satisfied the
        predicate function was found and was assigned to \a cred.

    \retval KHM_ERROR_NOT_FOUND No credential was found that matched
        the predicate function.

    \note When querying credential sets that are shared between
        threads, it is possible that another thread modifies the
        credential set between successive calls to
        kcdb_credset_find_filtered().  Therefore a continued sequences of
        searches are not guaranteed to exhastively cover the
        credential set nor to not return duplicate matches.  Duplicate
        matches are possible if the order of the credentials in the
        set was changed.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_find_filtered(khm_handle credset,
                           khm_int32 idx_start,
                           kcdb_cred_filter_func f,
                           void * rock,
                           khm_handle * cred,
                           khm_int32 * idx);

/*! \brief Find matching credential

    Searches a credential set for a credential that matches the
    specified credential.  For a credential to be a match, it must
    have the same identity, credential type and name.

    \param[in] credset Credential set to search 

    \param[in] cred_src Credetial to search on

    \param[out] cred_dest receieves the matching credential if the
        search is successful.  If a handle is returend, the
        kcdb_cred_release() must be used to release the handle.  If
        the matching credential is not required, you can pass in NULL.

    \retval KHM_ERROR_SUCCESS The search was successful.  A credential
        was assigned to \a cred_dest.  The returned handle should be
        released with a call to kcdb_cred_release().

    \retval KHM_ERROR_NOT_FOUND A matching credential was not found.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_credset_find_cred(khm_handle credset,
                       khm_handle cred_src,
                       khm_handle *cred_dest);
                                               

/*! \brief Delete a credential from a credential set.

    The credential at index \a idx will be deleted.  All the
    credentials that are at indices \a idx + 1 and above will be moved
    down to fill the gap and the size of the credential set will
    decrease by one.

    Use kcdb_credset_del_cred_ref() to delete a credential by
    reference.  Using kcdb_credset_del_cred() is faster than
    kcdb_credset_del_cred_ref().

    If you call kcdb_credset_del_cred() or kcdb_credset_del_cred_ref()
    from within kcdb_credset_apply(), the credential will only be
    marked as deleted.  They will not be removed.  This means that the
    size of the credential set will not decrease.  To purge the
    deleted credentials from the set, call kcdb_credset_purge() after
    kcdb_credset_apply() completes.

    \note The credential set cannot be sealed.

    \see kcdb_credset_del_cred_ref()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_del_cred(khm_handle credset,
                      khm_int32 idx);

/*! \brief Delete a credential from a credential set by reference.

    See kcdb_credset_del_cred() for description of what happens when a
    credential is deleted from a credential set.

    \note The credential set cannot be sealed.

    \see kcdb_credset_del_cred()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_del_cred_ref(khm_handle credset,
                          khm_handle cred);

/*! \brief Add a credential to a credential set.

    The credential is added by reference.  In other words, no copy of
    the credential is made.

    \param[in] idx Index of the new credential.  This must be a value
        in the range 0..(previous size of credential set) or -1.  If
        -1 is specifed, then the credential is appended at the end of
        the set.

    \note The credential set cannot be sealed.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_add_cred(khm_handle credset,
                      khm_handle cred,
                      khm_int32 idx);

/*! \brief Get the number of credentials in a credential set.

    Credentials in a credential set may be volatile.  When
    kcdb_credeset_get_size() is called, the credential set is
    compacted to only include credentials that are active at the time.
    However, when you are iterating through the credential set, it
    might be the case that some credentials would get marked as
    deleted.  These credentials will remain in the credential set
    until the credential set is discarded or another call to
    kcdb_credset_get_size() or kdcb_credset_purge() is made.

    If the credential set is sealed, then it will not be compacted and
    will include deleted credentials as well.

    \see kcdb_credset_purge()
    \see kcdb_credset_get_cred()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_get_size(khm_handle credset,
                      khm_size * size);

/*! \brief Removes credentials that have been marked as deleted from a credential set.

    See description of \a kcdb_credset_purge() for a description of
    what happens when credntials that are contained in a credential
    set are deleted by an external entity.

    \note The credential set cannot be sealed.

    \see kcdb_credset_get_size()
    \see kcdb_credset_get_cred()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_purge(khm_handle credset);

/*! \brief Applies a function to all the credentials in a credentials set

    The given function is called for each credential in a credential
    set.  With each iteration, the function is called with a handle to
    the credential and the user defined parameter \a rock.  If the
    function returns anything other than KHM_ERROR_SUCCESS, the
    processing stops.

    \param[in] credset The credential set to apply the function to, or
        NULL if you want to apply this to the root credential set.

    \param[in] f Function to call for each credential

    \param[in] rock An opaque parameter which is to be passed to 'f'
        as the second argument.

    \retval KHM_ERROR_SUCCESS All the credentials were processed.

    \retval KHM_ERROR_EXIT The supplied function signalled the
        processing to be aborted.

    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_apply(khm_handle credset, 
                   kcdb_cred_apply_func f, 
                   void * rock);

/*! \brief Sort the contents of a credential set.

    \param[in] rock A custom argument to be passed in to the \a comp function.

    \note The credential set cannot be sealed.

    \see kcdb_cred_comp_generic()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credset_sort(khm_handle credset,
                  kcdb_comp_func comp,
                  void * rock);

/*! \brief Seal a credential set

    Sealing a credential set makes it read-only.  To unseal a
    credential set, call kcdb_credset_unseal().

    Sealing is an additive operation.  kcdb_credset_seal() can be
    called muliple times.  However, for every call to
    kcdb_credset_seal() a call to kcdb_credset_unseal() must be made
    to undo the seal.  The credential set will become unsealed when
    all the seals are released.

    Once sealed, the credential set will not allow any operation that
    might change its contents.  However, a selaed credential set can
    still be delted.

    \see kcdb_credset_unseal()
 */
KHMEXP khm_int32 KHMAPI 
kcdb_credset_seal(khm_handle credset);

/*! \brief Unseal a credential set

    Undoes what kcdb_credset_seal() did.  This does not guarantee that
    the credential set is unsealed since there may be other seals.

    \see kcdb_credset_seal()
 */
KHMEXP khm_int32 KHMAPI
kcdb_credset_unseal(khm_handle credset);

/*! \brief Defines a sort criterion for kcdb_cred_comp_generic()

    \see kcdb_cred_comp_generic()
*/
typedef struct tag_kcdb_cred_comp_field {
    khm_int32 attrib; /*!< a valid attribute ID */
    khm_int32 order; /*!< one of KCDB_CRED_COMP_INCREASING or
                       KCDB_CRED_COMP_DECREASING.  Optionally,
                       KCDB_CRED_COMP_INITIAL_FIRST may be combined
                       with either. */
} kcdb_cred_comp_field;

/*! \brief Defines the sort order for a field in ::kcdb_cred_comp_field 

    Sorts lexicographically ascending by string representation of field.
*/
#define KCDB_CRED_COMP_INCREASING 0

/*! \brief Defines the sort order for a field in ::kcdb_cred_comp_field

    Sorts lexicographically descending by string representation of
    field.
 */
#define KCDB_CRED_COMP_DECREASING 1

/*! \brief Defines the sort order for a field in ::kcdb_cred_comp_field 

    Any credentials which have the ::KCDB_CRED_FLAG_INITIAL will be
    grouped above any that don't.

    If that does not apply, then credentials from the primary
    credentials type will be sorted before others.
*/
#define KCDB_CRED_COMP_INITIAL_FIRST 2

/*! \brief Defines the sort criteria for kcdb_cred_comp_generic()

    \see kcdb_cred_comp_generic()
*/
typedef struct tag_kcdb_cred_comp_order {
    khm_int32 nFields;
    kcdb_cred_comp_field * fields;
} kcdb_cred_comp_order;

/*! \brief A generic compare function for comparing credentials.

    This function can be passed as a parameter to kcdb_credset_sort().

    The \a rock parameter to this function should be a pointer to a
    ::kcdb_cred_comp_order object.  The \a fields member of the
    ::kcdb_cred_comp_order object should point to an array of
    ::kcdb_cred_comp_field objects, each of which specifies the sort
    order in decreasing order of priority.  The number of
    ::kcdb_cred_comp_field objects in the array should correspond to
    the \a nFields member in the ::kcdb_cred_comp_order object.

    The array of ::kcdb_cred_comp_field objects define the sort
    criteria, in order.  The \a attrib member should be a valid
    attribute ID, while the \a order member determines whether the
    sort order is increasing or decreasing.  The exact meaning or
    increasing or decreasing depends on the data type of the
    attribute.

    \param[in] rock a pointer to a ::kcdb_cred_comp_order object
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_comp_generic(khm_handle cred1, 
                       khm_handle cred2, 
                       void * rock);

/*@}*/

/*! \defgroup kcdb_cred Credentials */
/*@{*/

/*! \brief Maximum number of characters in a credential name */
#define KCDB_CRED_MAXCCH_NAME 256

/*! \brief Maximum number of bytes in a credential name */
#define KCDB_CRED_MAXCB_NAME (sizeof(wchar_t) * KCDB_CRED_MAXCCH_NAME)

/*! \brief Marked as deleted */
#define KCDB_CRED_FLAG_DELETED     0x00000008

/*! \brief Renewable */
#define KCDB_CRED_FLAG_RENEWABLE   0x00000010

/*! \brief Initial

    Initial credentials form the basis of an identity.  Some
    properties of an initial credential, such as being renewable, are
    directly inherited by the identity.  An identity is also
    automatically considered valid if it contains a valid initial
    credential.
 */
#define KCDB_CRED_FLAG_INITIAL     0x00000020

/*! \brief Expired

    The credential's lifetime has ended.
 */
#define KCDB_CRED_FLAG_EXPIRED     0x00000040

/*! \brief Invalid

    The credential can no longer serve its intended function.  This
    may be because it is expired and is not renewable, or its
    renewable time period has also expired, or for some other reason.
 */
#define KCDB_CRED_FLAG_INVALID     0x00000080

/*! \brief Credential is selected

    Indicates that the credential is selected.  Note that using this
    flag may be subject to race conditions.
 */
#define KCDB_CRED_FLAG_SELECTED    0x00000100

/*! \brief Privileged credential

    The contents of the credential should be considered privileged.
    These credentials cannot be added to the root credentials set and
    are never duplicated.
 */
#define KCDB_CRED_FLAG_PRIVILEGED  0x00000200

/*! \brief Destroyable credential

    The credential can be destroyed.  This bit is set by default when
    a new credential is created, and must be turned off if the
    credential cannot be destoyed.
 */
#define KCDB_CRED_FLAG_DESTROYABLE 0x00000400

/*! \brief Bitmask indicating all known credential flags
 */
#define KCDB_CRED_FLAGMASK_ALL     0x0000ffff

/*! \brief External flags

    These are flags that are provided by the credentials providers.
    The other flags are internal to KCDB and should not be modified.
 */
#define KCDB_CRED_FLAGMASK_EXT     (KCDB_CRED_FLAG_INITIAL | KCDB_CRED_FLAG_EXPIRED | KCDB_CRED_FLAG_INVALID | KCDB_CRED_FLAG_RENEWABLE)

/*! \brief Bitmask indicating dditive flags 

    Additive flags are special flags which are added to exiting
    credentials based on new credentials when doing a collect
    operation.  See details on kcdb_credset_collect()

    \see kcdb_credset_collect()
*/
#define KCDB_CRED_FLAGMASK_ADDITIVE KCDB_CRED_FLAG_SELECTED

/*! \brief Generic credentials request

    This data structure is used as the format for a generic
    credentials reqeust for a ::KMSG_KCDB_REQUEST message.  A plugin
    typically publishes this message so that a credentials provider
    may handle it and in response, obtain the specified credential.

    While the \a identity, \a type and \a name members of the
    structure are all optional, typically one would specify all three
    or at least two for a credential provider to be able to provide
    the credential unambigously.

    Credential providers do not need to respond to ::KMSG_KCDB_REQUEST
    messages.  However, if they do, they should make sure that they
    are the only credential provider that is responding by setting the
    \a semaphore member to a non-zero value.  The \a semaphore is set
    to zero when a request is initially sent out.  When incrementing
    the semaphore, the plugin should use a thread safe mechanism to
    ensure that there are no race conditions that would allow more
    than one provider to respond to the message.
 */
typedef struct tag_kcdb_cred_request {
    khm_handle identity;        /*!< Identity of the credential.  Set
                                  to NULL if not specified. */
    khm_int32  type;            /*!< Type of the credential.  Set to
                                  KCDB_CREDTYPE_INVALID if not
                                  specified.  */
    wchar_t *  name;            /*!< Name of the credential.  Set to
                                  NULL if not specified.  */

    khm_handle dest_credset;    /*!< If non-NULL, instructs whoever is
                                  handling the request that the
                                  credential thus obtained be placed
                                  in this credential set in addition
                                  to whereever it may place newly
                                  acquired credentials.  Note that
                                  while this can be NULL if the new
                                  credential does not need to be
                                  placed in a credential set, it can
                                  not equal the root credential
                                  set.  */

    void *     vparam;        /*!< An unspecified
                                  parameter. Specific credential types
                                  may specify how this field is to be
                                  used. */

    long       semaphore;       /*!< Incremented by one when this
                                  request is answered.  Only one
                                  credential provider is allowed to
                                  answer a ::KMSG_KCDB_REQUEST
                                  message.  Initially, when the
                                  message is sent out, this member
                                  should be set to zero. */
} kcdb_cred_request;

/*! \brief Create a new credential

    \param[in] name Name of credential.  \a name cannot be NULL and cannot
        exceed \a KCDB_CRED_MAXCCH_NAME unicode characters including the 
        \a NULL terminator.
    \param[in] identity A reference to an identity.
    \param[in] cred_type A credentials type identifier for the credential.
    \param[out] result Gets a held reference to the newly created credential.
        Call kcdb_cred_release() or kcdb_cred_delete() to release the 
        reference.
    \see kcdb_cred_release()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_create(const wchar_t *   name, 
                 khm_handle  identity,
                 khm_int32   cred_type,
                 khm_handle * result);

/*! \brief Duplicate an existing credential.

    \param[out] newcred A held reference to the new credential if the call
        succeeds.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_dup(khm_handle cred,
              khm_handle * newcred);

/*! \brief Updates one credential using field values from another

    All fields that exist in \a vsrc will get copied to \a vdest and will
    overwrite any values that are already there in \a vdest.  However any
    values that exist in \a vdest taht do not exist in \a vsrc will not be
    modified.

    \retval KHM_ERROR_SUCCESS vdest was successfully updated
    \retval KHM_ERROR_EQUIVALENT all fields in vsrc were present and equivalent in vdest
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_update(khm_handle vdest,
                 khm_handle vsrc);

/*! \brief Set an attribute in a credential by name

    

    \param[in] cbbuf Number of bytes of data in \a buffer.  The
        individual data type handlers may copy in less than this many
        bytes in to the credential.  For some data types where the
        size of the buffer is fixed or can be determined from its
        contents, you can specify ::KCDB_CBSIZE_AUTO for this
        parameter.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_set_attrib(khm_handle cred, 
                     const wchar_t * name, 
                     const void * buffer, 
                     khm_size cbbuf);

/*! \brief Set an attribute in a credential by attribute id

    \param[in] buffer A pointer to a buffer containing the data to
        assign to the attribute.  Setting this to NULL has the effect
        of removing any data that is already assigned to the
        attribute.  If \a buffer is non-NULL, then \a cbbuf should
        specify the number of bytes in \a buffer.

    \param[in] cbbuf Number of bytes of data in \a buffer.  The
        individual data type handlers may copy in less than this many
        bytes in to the credential.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_set_attr(khm_handle cred, 
                   khm_int32 attr_id, 
                   const void * buffer, 
                   khm_size cbbuf);

/*! \brief Get an attribute from a credential by name.

    \param[in] buffer The buffer that is to receive the attribute
        value.  Set this to NULL if only the required buffer size is
        to be returned.

    \param[in,out] cbbuf The number of bytes available in \a buffer.
        If \a buffer is not sufficient, returns KHM_ERROR_TOO_LONG and
        sets this to the required buffer size.

    \note Set both \a buffer and \a cbbuf to NULL if only the
        existence of the attribute is to be checked.  If the attribute
        exists in this credential then the function will return
        KHM_ERROR_SUCCESS, otherwise it returns KHM_ERROR_NOT_FOUND.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_attrib(khm_handle cred, 
                     const wchar_t * name, 
                     khm_int32 * attr_type,
                     void * buffer, 
                     khm_size * cbbuf);

/*! \brief Get an attribute from a credential by attribute id.

    \param[in] buffer The buffer that is to receive the attribute
        value.  Set this to NULL if only the required buffer size is
        to be returned.

    \param[in,out] cbbuf The number of bytes available in \a buffer.
        If \a buffer is not sufficient, returns KHM_ERROR_TOO_LONG and
        sets this to the required buffer size.

    \param[out] attr_type Receives the data type of the attribute.
        Set this to NULL if the type is not required.

    \note Set both \a buffer and \a cbbuf to NULL if only the
        existence of the attribute is to be checked.  If the attribute
        exists in this credential then the function will return
        KHM_ERROR_SUCCESS, otherwise it returns KHM_ERROR_NOT_FOUND.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_attr(khm_handle cred, 
                   khm_int32 attr_id,
                   khm_int32 * attr_type,
                   void * buffer, 
                   khm_size * cbbuf);

/*! \brief Get the name of a credential.

    \param[in] buffer The buffer that is to receive the credential
        name.  Set this to NULL if only the required buffer size is to
        be returned.

    \param[in,out] cbbuf The number of bytes available in \a buffer.
        If \a buffer is not sufficient, returns KHM_ERROR_TOO_LONG and
        sets this to the required buffer size.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_name(khm_handle cred, 
                   wchar_t * buffer, 
                   khm_size * cbbuf);

/*! \brief Get the string representation of a credential attribute.

    A shortcut function which generates the string representation of a
    credential attribute directly.

    \param[in] vcred A handle to a credential

    \param[in] attr_id The attribute to retrieve

    \param[out] buffer A pointer to a string buffer which receives the
        string form of the attribute.  Set this to NULL if you only
        want to determine the size of the required buffer.

    \param[in,out] pcbbuf A pointer to a #khm_int32 that, on entry,
        holds the size of the buffer pointed to by \a buffer, and on
        exit, receives the actual number of bytes that were copied.

    \param[in] flags Flags for the string conversion. Can be set to
        one of KCDB_TS_LONG or KCDB_TS_SHORT.  The default is
        KCDB_TS_LONG.

    \retval KHM_ERROR_SUCCESS Success
    \retval KHM_ERROR_NOT_FOUND The given attribute was either invalid
        or was not defined for this credential
    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid
    \retval KHM_ERROR_TOO_LONG Either \a buffer was NULL or the
        supplied buffer was insufficient
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_attr_string(khm_handle vcred, 
                          khm_int32 attr_id,
                          wchar_t * buffer, 
                          khm_size * pcbbuf,
                          khm_int32 flags);

/*! \brief Get the string representation of a credential attribute by name.

    A shortcut function which generates the string representation of a
    credential attribute directly.

    \param[in] vcred A handle to a credential

    \param[in] attrib The name of the attribute to retrieve

    \param[out] buffer A pointer to a string buffer which receives the
        string form of the attribute.  Set this to NULL if you only
        want to determine the size of the required buffer.

    \param[in,out] pcbbuf A pointer to a #khm_int32 that, on entry,
        holds the size of the buffer pointed to by \a buffer, and on
        exit, receives the actual number of bytes that were copied.

    \param[in] flags Flags for the string conversion. Can be set to
        one of KCDB_TS_LONG or KCDB_TS_SHORT.  The default is
        KCDB_TS_LONG.

    \see kcdb_cred_get_attr_string()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_attrib_string(khm_handle cred, 
                            const wchar_t * name, 
                            wchar_t * buffer, 
                            khm_size * cbbuf,
                            khm_int32 flags) ;


/*! \brief Get a held reference to the identity associated with a credential

    Use kcdb_identity_release() to release the reference that is
    returned.

    \see kcdb_identity_relase()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_identity(khm_handle cred, 
                       khm_handle * identity);

/*! \brief Set the identity of a credential

    While it is ill-advised to change the identity of a credential
    that has been placed in one or more credential sets, there can be
    legitimate reasons for doing so.  Only change the identity of a
    credential that is not placed in a credential set or placed in a
    credential set that is only used by a single entity.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_set_identity(khm_handle vcred,
                       khm_handle id);

/*! \brief Get the serial number for the credential.

    Each credential gets assigned a serial number at the time it is
    created.  This will stay with the credential for its lifetime.

    \param[out] pserial Receives the serial number. Cannot be NULL.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_serial(khm_handle cred,
                     khm_ui_8 * pserial);

/*! \brief Get the type of the credential.

    The returned type is a credential type. Doh.

    \param[out] type Receives the type.  Cannot be NULL.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_type(khm_handle cred,
                   khm_int32 * type);

/*! \brief Retrieve flags from a credential

    The flags returned will be place in the location pointed to by \a
    flags.  Note that the specified credential must be an active
    credential for the operation to succeed.  This means the
    ::KCDB_CRED_FLAG_DELETED will never be retured by this function.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_cred_get_flags(khm_handle cred,
                    khm_int32 * flags);

/*! \brief Set the flags of a credential

    The flags specified in the \a mask parameter will be set to the
    values specified in the \a flags parameter.  The flags that are
    not included in \a mask will not be modified.

    This function can not be used to set the ::KCDB_CRED_FLAG_DELETED
    flag.  If this bit is specified in either \a flags or \a mask, it
    will be ignored.

    \see ::KCDB_CRED_FLAGMASK_ALL
 */
KHMEXP khm_int32 KHMAPI 
kcdb_cred_set_flags(khm_handle cred,
                    khm_int32 flags,
                    khm_int32 mask);

/*! \brief Hold a reference to a credential.

    Use kcdb_cred_release() to release the reference.

    \see kcdb_cred_release()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_hold(khm_handle cred);

/*! \brief Release a held reference to a credential.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_release(khm_handle cred);

/*! \brief Delete a credential.

    The credential will be marked for deletion and will continue to
    exist until all held references are released.  If the credential
    is bound to a credential set or the root credential store, it will
    be removed from the respective container.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_cred_delete(khm_handle cred);

/*! \brief Compare an attribute of two credentials by name.

    \return The return value is dependent on the type of the attribute
    and indicate a weak ordering of the attribute values of the two
    credentials.  If one or both credentials do not contain the
    attribute, the return value is 0, which signifies that no ordering
    can be determined.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_creds_comp_attrib(khm_handle cred1, 
                       khm_handle cred2, 
                       const wchar_t * name);

/*! \brief Compare an attribute of two credentials by attribute id.

    \return The return value is dependent on the type of the attribute
    and indicate a weak ordering of the attribute values of the two
    credentials.  If one or both credentials do not contain the
    attribute, the return value is 0, which signifies that no ordering
    can be determined.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_creds_comp_attr(khm_handle cred1, 
                     khm_handle cred2, 
                     khm_int32 attr_id);

/*! \brief Compare two credentials for equivalence

    \return Non-zero if the two credentials are equal.  Zero otherwise.
    \note Two credentials are considered equal if all the following hold:
        - Both refer to the same identity.
        - Both have the same name.
        - Both have the same type.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_creds_is_equal(khm_handle cred1,
                    khm_handle cred2);

/*@}*/
/*@}*/

/********************************************************************/

/*! \defgroup kcdb_type Credential attribute types

@{*/

/*! \brief Convert a field to a string

    Provides a string representation of a field in a credential.  The
    data buffer can be assumed to be valid.

    On entry, \a s_buf can be NULL if only the required size of the
    buffer is to be returned.  \a pcb_s_buf should be non-NULL and
    should point to a valid variable of type ::khm_size that will, on
    entry, contain the size of the buffer pointed to by \a s_buf if \a
    s_buf is not \a NULL, and on exit will contain the number of bytes
    consumed in \a s_buf, or the required size of the buffer if \a
    s_buf was NULL or the size of the buffer was insufficient.

    The implementation should verify the parameters that are passed in
    to the function.

    The data pointed to by \a data should not be modified in any way.

    \param[in] data Valid pointer to a block of data

    \param[in] cb_data Number of bytes in data block pointed to by \a
        data

    \param[out] s_buf Buffer to receive the string representation of
        data.  If the data type flags has KCDB_TYPE_FLAG_CB_AUTO, then
        this parameter could be set to KCDB_CBSIZE_AUTO.  In this
        case, the function should compute the size of the input buffer
        assuming that the input buffer is valid.

    \param[in,out] pcb_s_buf On entry, contains the size of the buffer
        pointed to by \a s_buf, and on exit, contains the number of
        bytes used by the string representation of the data including
        the NULL terminator

    \param[in] flags Flags for formatting the string

    \retval KHM_ERROR_SUCCESS The string representation of the data
        field was successfully copied to \a s_buf and the size of the
        buffer used was copied to \a pcb_s_buf.

    \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid

    \retval KHM_ERROR_TOO_LONG Either \a s_buf was \a NULL or the size
        indicated by \a pcb_s_buf was too small to contain the string
        representation of the value.  The required size of the buffer
        is in \a pcb_s_buf.

    \note This documents the expected behavior of this prototype function

    \see ::kcdb_type
 */
typedef khm_int32 
(KHMCALLBACK *kcdb_dtf_toString)(const void *     data,
                                 khm_size         cb_data,
                                 wchar_t *        s_buf,
                                 khm_size *       pcb_s_buf,
                                 khm_int32        flags);

/*! \brief Verifies whetehr the given buffer contains valid data

    The function should examine the buffer and the size of the buffer
    and determine whether or not the buffer contains valid data for
    this data type.

    The data field pointed to by \a data should not be modified in any
    way.

    \param[in] data A pointer to a data buffer

    \param[in] cb_data The number of bytes in the data buffer. If the
        data type flags has KCDB_TYPE_FLAG_CB_AUTO, then this
        parameter could be set to KCDB_CBSIZE_AUTO.  In this case, the
        function should compute the size of the input buffer assuming
        that the input buffer is valid.

    \return TRUE if the data is valid, FALSE otherwise.

    \note This documents the expected behavior of this prototype function

    \see ::kcdb_type
*/
typedef khm_boolean 
(KHMCALLBACK *kcdb_dtf_isValid)(const void *     data,
                                khm_size         cb_data);

/*! \brief Compare two fields

    Compare the two data fields and return a value indicating their
    relative ordering.  The return value follows the same
    specification as strcmp().

    Both data buffers that are passed in can be assumed to be valid.

    None of the data buffers should be modified in any way.

    \param[in] data_l Valid pointer to first data buffer

    \param[in] cb_data_l Number of bytes in \a data_l. If the data
        type flags has KCDB_TYPE_FLAG_CB_AUTO, then this parameter
        could be set to KCDB_CBSIZE_AUTO.  In this case, the function
        should compute the size of the input buffer assuming that the
        input buffer is valid.

    \param[in] data_r Valid pointer to second data buffer

    \param[in] cb_data_r Number of bytes in \a data_r. If the data
        type flags has KCDB_TYPE_FLAG_CB_AUTO, then this parameter
        could be set to KCDB_CBSIZE_AUTO.  In this case, the function
        should compute the size of the input buffer assuming that the
        input buffer is valid.

    \return The return value should be
        - Less than zero if \a data_l &lt; \a data_r
        - Equal to zero if \a data_l == \a data_r or if this data type can not be compared
        - Greater than zero if \a data_l &gt; \a data_r

    \note This documents the expected behavior of this prototype function

    \see ::kcdb_type
*/
typedef khm_int32 
(KHMCALLBACK *kcdb_dtf_comp)(const void *     data_l,
                             khm_size         cb_data_l,
                             const void *     data_r,
                             khm_size         cb_data_r);

/*! \brief Duplicate a data field

    Duplicates a data field.  The buffer pointed to by \a data_src
    contains a valid field.  The function should copy the field with
    appropriate adjustments to \a data_dst.

    The \a data_dst parameter can be NULL if only the required size of
    the buffer is needed.  In this case, teh function should set \a
    pcb_data_dst to the number of bytes required and then return
    KHM_ERROR_TOO_LONG.

    \param[in] data_src Pointer to a valid data buffer

    \param[in] cb_data_src Number of bytes in \a data_src. If the data
        type flags has KCDB_TYPE_FLAG_CB_AUTO, then this parameter
        could be set to KCDB_CBSIZE_AUTO.  In this case, the function
        should compute the size of the input buffer assuming that the
        input buffer is valid.

    \param[out] data_dst Poitner to destination buffer.  Could be NULL
       if only the required size of the destination buffer is to be
       returned.

    \param[in,out] pcb_data_dst On entry specifies the number of bytes
        in \a data_dst, and on exit should contain the number of bytes
        copied.

    \retval KHM_ERROR_SUCCESS The data was successfully copied.  The
        number of bytes copied is in \a pcb_data_dst

    \retval KHM_ERROR_INVALID_PARAM One or more parameters is incorrect.

    \retval KHM_ERROR_TOO_LONG Either \a data_dst was NULL or the size
        of the buffer was insufficient.  The required size is in \a
        pcb_data_dst

    \note This documents the expected behavior of this prototype function

    \see ::kcdb_type
 */
typedef khm_int32 
(KHMCALLBACK *kcdb_dtf_dup)(const void * data_src,
                            khm_size cb_data_src,
                            void * data_dst,
                            khm_size * pcb_data_dst);

/*! \brief A data type descriptor.

    Handles basic operation for a specific data type.

    \see \ref cred_data_types
*/
typedef struct tag_kcdb_type {
    wchar_t *   name;
    khm_int32   id;
    khm_int32   flags;

    khm_size    cb_min;
    khm_size    cb_max;

    kcdb_dtf_toString    toString;
        /*!< Provides a string representation for a value.  */

    kcdb_dtf_isValid     isValid;
        /*!< Returns true of the value is valid for this data type */

    kcdb_dtf_comp        comp;
        /*!< Compare two values and return \a strcmp style return value */

    kcdb_dtf_dup         dup;
        /*!< Duplicate a value into a secondary buffer */
} kcdb_type;

/*! \name Flags for kcdb_type::toString
@{*/
/*! \brief Specify that the short form of the string representation should be returned. 

    Flags for #kcdb_type::toString.  The flag specifies how long the
    string representation should be.  The specific length of a short
    or long description is not restricted and it is up to the
    implementation to choose how to interpret the flags.

    Usually, KCDB_TS_SHORT is specified when the amount of space that
    is available to display the string is very restricted.  It may be
    the case that the string is truncated to facilitate displaying in
    a constrainted space.  
*/
#define KCDB_TS_SHORT   KCDB_RFS_SHORT

/*! \brief Specify that the long form of the string representation should be returned 

    Flags for #kcdb_type::toString.  The flag specifies how long the
    string representation should be.  The specific length of a short
    or long description is not restricted and it is up to the
    implementation to choose how to interpret the flags.

*/
#define KCDB_TS_LONG    0
/*@}*/

/*! \brief The maximum number of bytes allowed for a value of any type */
#define KCDB_TYPE_MAXCB 16384

/*! \name Flags for kcdb_type
@{*/

/*! \brief The type supports KCDB_CBSIZE_AUTO.

    Used for types where the size of the object can be determined
    through context or by the object content.  Such as for objects
    that have a fixed size or unicode strings that have a terminator.

    This implies that ALL the object manipulation callbacks that are
    defined in this type definition support the KCDB_CBSIZE_AUTO
    value.
*/
#define KCDB_TYPE_FLAG_CB_AUTO      0x00000010

/*! \brief The \a cb_min member is valid.

    The \a cb_min member defines the minimum number of bytes that an
    object of this type will consume.

    \note If this flag is used in conjunction with \a
    KCDB_TYPE_FLAG_CB_MAX then, \a cb_min must be less than or equal
    to \a cb_max. 
*/
#define KCDB_TYPE_FLAG_CB_MIN       0x00000080

/*! \brief The \a cb_max member is valid.

    The \a cb_max member defines the maximum number of bytes that an
    object of this type will consume.

    \note If this flag is used in conjunction with \a
        KCDB_TYPE_FLAG_CB_MIN then, \a cb_min must be less than or
        equal to \a cb_max. */
#define KCDB_TYPE_FLAG_CB_MAX       0x00000100

/*! \brief Denotes that objects of this type have a fixed size.

    If this flags is specified, then the type definition must also
    specify cb_min and cb_max, which must both be the same value.

    \note Implies \a KCDB_TYPE_FLAG_CB_AUTO, \a KCDB_TYPE_FLAG_CB_MIN
        and \a KCDB_TYPE_FLAG_CB_MAX. Pay special attention to the
        implication of \a KCDB_TYPE_FLAG_AUTO.
*/
#define KCDB_TYPE_FLAG_CB_FIXED (KCDB_TYPE_FLAG_CB_AUTO|KCDB_TYPE_FLAG_CB_MIN|KCDB_TYPE_FLAG_CB_MAX)

/*@}*/

KHMEXP khm_int32 KHMAPI 
kcdb_type_get_id(const wchar_t *name, khm_int32 * id);

/*! \brief Return the type descriptor for a given type id

    \param[out] info Receives a held reference to a type descriptor.
        Use kcdb_type_release_info() to release the handle.  If the \a
        info parameter is NULL, the function returns KHM_ERROR_SUCCESS
        if \a id is a valid type id, and returns KHM_ERROR_NOT_FOUND
        otherwise.

    \see kcdb_type_release_info()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_type_get_info(khm_int32 id, kcdb_type ** info);

/*! \brief Release a reference to a type info structure

    Releases the reference to the type information obtained with a
    prior call to kcdb_type_get_info().
 */
KHMEXP khm_int32 KHMAPI 
kcdb_type_release_info(kcdb_type * info);

/*! \brief Get the name of a type

    Retrieves the non-localized name of the specified type.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_type_get_name(khm_int32 id, 
                   wchar_t * buffer, 
                   khm_size * cbbuf);

/*! \brief Register a credentials attribute type

    The credentials type record pointed to by \a type defines a new
    credential attribute type.  The \a id member of \a type may be set
    to KCDB_TYPE_INVALID to indicate that an attribute ID is to be
    generated automatically.

    \param[in] type The type descriptor
    \param[out] new_id Receives the identifier for the credential attribute type.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_type_register(const kcdb_type * type, 
                   khm_int32 * new_id);

/*! \brief Unregister a credential attribute type

    Removes the registration for the specified credentials attribute
    type.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_type_unregister(khm_int32 id);

KHMEXP khm_int32 KHMAPI 
kcdb_type_get_next_free(khm_int32 * id);

/*! \name Conversion functions
@{*/
/*! \brief Convert a time_t value to FILETIME
*/
KHMEXP void KHMAPI 
TimetToFileTime( time_t t, LPFILETIME pft );

/*! \brief Convert a time_t interval to a FILETIME interval
*/
KHMEXP void KHMAPI 
TimetToFileTimeInterval(time_t t, LPFILETIME pft);

/*! \brief Convert a FILETIME interval to seconds
*/
KHMEXP long KHMAPI 
FtIntervalToSeconds(const FILETIME * pft);

/*! \brief Convert a FILETIME interval to milliseconds
*/
KHMEXP long KHMAPI 
FtIntervalToMilliseconds(const FILETIME * pft);

/*! \brief Convert a FILETIME to a 64 bit int
*/
KHMEXP khm_int64 KHMAPI
FtToInt(const FILETIME * pft);

/*! \brief Convert a 64 bit int to a FILETIME
*/
KHMEXP FILETIME KHMAPI
IntToFt(khm_int64 i);

/*! \brief Calculate the difference between two FILETIMEs

    Returns the value of ft1 - ft2
 */
KHMEXP FILETIME KHMAPI
FtSub(const FILETIME * ft1, const FILETIME * ft2);

/*! \brief Calculate the sum of two FILETIMEs

    Return the value of ft1 + ft2
 */
KHMEXP FILETIME KHMAPI
FtAdd(const FILETIME * ft1, const FILETIME * ft2);

#define FtIsZero(pft) (FtToInt(pft) == 0)

/*! \brief Convert a FILETIME inverval to a string
*/
KHMEXP khm_int32 KHMAPI 
FtIntervalToString(const FILETIME * data, 
                   wchar_t * buffer, 
                   khm_size * cb_buf);


/*! \brief Equivalence threshold

  The max difference between two timers (in seconds) that can exist
  where we consider them to be equivalent. */
#define TT_TIMEEQ_ERROR_SMALL 1

/*! \brief Large equivalence threshold

  The max absolute difference between two timers (in seconds) that can
  exist where we consider both timers to be in the same timeslot. */
#define TT_TIMEEQ_ERROR 20

/*! \brief The minimum half time interval */
#define TT_MIN_HALFLIFE_INTERVAL 60

/*! \brief Expired threshold

  The maximum lifetime at which we consider a credential is as good
  as dead as far as notifications are concerned in seconds*/
#define TT_EXPIRED_THRESHOLD 60

/*! \brief Maximum allowed clock-skew */
#define TT_CLOCKSKEW_THRESHOLD 300

#define SECONDS_TO_FT(s)  ((s) * 10000000i64)
#define FT_TO_MS(ft)      ((DWORD)((ft) / 10000i64))
#define FT_TO_SECONDS(ft) (FT_TO_MS(ft) / 1000)

/* as above, in FILETIME units of 100ns */
#define FT_MIN_HALFLIFE_INTERVAL SECONDS_TO_FT(TT_MIN_HALFLIFE_INTERVAL)

#define FT_EXPIRED_THRESHOLD SECONDS_TO_FT(TT_EXPIRED_THRESHOLD)


#define FTSE_INTERVAL      0x00010000
#define FTSE_RELATIVE      0x00020000
#define FTSE_RELATIVE_DAY  0x00040000

/*! \brief Convert a FILETIME to a string

  Generates a string representation of a \a FILETIME value.  The
  representation is determined by \a flags as follows:

  - ::FTSE_INTERVAL : The \a pft represents an interval.  If this flag
    is not specified, it will be assumed that the \a pft represents an
    absolute \a FILETIME value.

  - ::FTSE_RELATIVE : The string representation should be relative to
    the current time.  For example if the current time is 5pm and \a
    pft specifies 6pm, then the relative representation would be "in 1
    hour".  This flag can't be used with ::FTSE_INTERVAL.

  - ::FTSE_RELATIVE_DAY : Similar to ::FTSE_RELATIVE, but specifies
    the time in absolute terms while specifying the day in relative
    terms.  E.g.  If the current time is 5pm Saturday and \a pft
    specifies 6pm on Sunday, the this representation would give
    "tomorrow at 6pm".

  - ::KCDB_TS_SHORT : Make the string short.  If there are alternate
    representations, choose the shortest.

 */
KHMEXP khm_int32 KHMAPI
FtToStringEx(const FILETIME * pft, khm_int32 flags,
             long * ms_to_change,
             wchar_t * buffer, khm_size * cb_buf);

/*! \brief Parse a string representing an interval into a FILETIME interval

    The string is a localized string which should look like the
    following:

    \code
    [number unit] [number unit]...
    \endcode

    where \a number is an integer while \a unit is a localized
    (possibly abbreviated) unit specification.  The value of the
    described interval is calculated as the sum of each \a number in
    \a units.  For example :

    \code
    1 hour 36 minutes
    \endcode

    would result in an interval specification that's equivalent to 1
    hour and 36 minutes.  Of course there is no restriction on the
    order in which the \a number \a unit specifications are given and
    the same unit may be repeated multiple times.

    \retval KHM_ERROR_INVALID_PARAM The given string was invalid or had
        a token that could not be parsed.  It can also mean that \a
        pft was NULL or \a str was NULL.

    \retval KHM_ERROR_SUCCESS The string was successfully parsed and
        the result was placed in \a pft.
*/
KHMEXP khm_int32 KHMAPI 
IntervalStringToFt(FILETIME * pft, const wchar_t * str);

/*! \brief Return number of milliseconds till next representation change

   Returns the number of milliseconds that must elapse away from the
   interval specified in pft \a for the representation of pft to
   change from whatever it is right now as returned by
   FtIntervalToString().

   Returns 0 if the representation is not expected to change.
*/
KHMEXP long KHMAPI 
FtIntervalMsToRepChange(const FILETIME * pft);

/*! \brief Convert a safe ANSI string to a Unicode string

    The resulting string is guaranteed to be NULL terminated and
    within the size limit set by \a cbwstr.

    If the whole string cannot be converted, \a wstr is set to an
    empty string.

    \return the number of characters converted.  This is always either
        the length of the string \a astr or 0.
*/
KHMEXP int KHMAPI 
AnsiStrToUnicode( wchar_t * wstr, size_t cbwstr, const char * astr);

/*! \brief Convert a Unicode string to ANSI

    The resulting string is guaranteed to be NULL terminated and
    within the size limit set by \a cbdest.

    \return the number of characters converted.  This is always either
        the length of the string \a src or 0.
*/
KHMEXP int KHMAPI 
UnicodeStrToAnsi( char * dest, size_t cbdest, const wchar_t * src);

/*! \brief Similar to printf, except uses FormatMessage to expand inserts

 */
KHMEXP khm_int32
FormatString(wchar_t * buf, khm_size cb_buf, const wchar_t * format, ...);
/*@}*/

/*! \name Standard type identifiers and names 
@{*/

/*! Maximum identifier number */
#define KCDB_TYPE_MAX_ID 255

/*! \brief Invalid type

    Used by functions that return a type identifier to indicate that
    the returned type identifier is invalid.  Also used to indicate
    that a type identifier is not available */
#define KCDB_TYPE_INVALID (-1)

/*! \brief All types

    Used by filters to indicate that all types are allowed.
*/
#define KCDB_TYPE_ALL       KCDB_TYPE_INVALID

/*! \brief Void

    No data.  This is not an actual data type.
 */
#define KCDB_TYPE_VOID      0

/*! \brief String

    NULL terminated Unicode string.  The byte count for a string
    attribute always includes the terminating NULL.
 */
#define KCDB_TYPE_STRING    1

/*! \brief Data

    A date/time represented in FILETIME format.
 */
#define KCDB_TYPE_DATE      2

/*! \brief Interval

    An interval of time represented as the difference between two
    FILETIME values.
 */
#define KCDB_TYPE_INTERVAL  3

/*! \brief 32-bit integer

    A 32-bit signed integer.
 */
#define KCDB_TYPE_INT32     4

/*! \brief 64-bit integer

    A 64-bit integer.
 */
#define KCDB_TYPE_INT64     5

/*! \brief Raw data

    A raw data buffer.
 */
#define KCDB_TYPE_DATA      6

#define KCDB_TYPENAME_VOID      L"Void"
#define KCDB_TYPENAME_STRING    L"String"
#define KCDB_TYPENAME_DATE      L"Date"
#define KCDB_TYPENAME_INTERVAL  L"Interval"
#define KCDB_TYPENAME_INT32     L"Int32"
#define KCDB_TYPENAME_INT64     L"Int64"
#define KCDB_TYPENAME_DATA      L"Data"
/*@}*/
/*@}*/

/********************************************************************/

/*! \defgroup kcdb_credattr Credential attributes */
/*@{*/

/*! \brief Prototype callback function for computed data types.

    If the flags for a particular attribute specifies that the value
    is computed, then a callback function should be specified.  The
    callback function will be called with a handle to a credential
    along with the attribute ID for the requested attribute.  The
    function should place the computed value in \a buffer.  The size
    of the buffer in bytes is specifed in \a cbsize.  However, if \a
    buffer is \a NULL, then the required buffer size should be placed
    in \a cbsize.
 */
typedef khm_int32 
(KHMCALLBACK *kcdb_attrib_compute_cb)(khm_handle cred, 
                                 khm_int32 id,
                                 void * buffer,
                                 khm_size * cbsize);

/*! \brief Credential attribute descriptor

    \see kcdb_attrib_register()
*/
typedef struct tag_kcdb_attrib {
    wchar_t * name;             /*!< Name.  (Not localized,
                                  required) */
    khm_int32 id;               /*!< Identifier.  When registering,
                                  this can be set to
                                  ::KCDB_ATTR_INVALID if a unique
                                  identifier is to be generated. */
    khm_int32 alt_id;           /*!< Alternate identifier.  If the \a
                                  flags specify
                                  ::KCDB_ATTR_FLAG_ALTVIEW, then this
                                  field should specify the identifier
                                  of the canonical attribute from
                                  which this attribute is derived. */
    khm_int32 flags;            /*!< Flags. Combination of \ref
                                  kcdb_credattr_flags "attribute
                                  flags" */

    khm_int32 type;             /*!< Type of the attribute.  Must be valid. */

    wchar_t * short_desc;       /*!< Short description. (Localized,
                                  optional) */

    wchar_t * long_desc;        /*!< Long description. (Localized,
                                  optional) */

    kcdb_attrib_compute_cb compute_cb;
                                /*!< Callback.  Required if \a flags
                                  specify ::KCDB_ATTR_FLAG_COMPUTED. */

    khm_size compute_min_cbsize;
                                /*!< Minimum number of bytes required
                                  to store this attribute.  Required
                                  if ::KCDB_ATTR_FLAG_COMPUTED is
                                  specified.*/
    khm_size compute_max_cbsize;
                                /*!< Maximum number of bytes required
                                  to store this attribute.  Required
                                  if ::KCDB_ATTR_FLAG_COMPUTED is
                                  specified.*/
} kcdb_attrib;

/*! \brief Retrieve the ID of a named attribute */
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_get_id(const wchar_t *name, 
                   khm_int32 * id);

/*! \brief Register an attribute

    \param[out] new_id Receives the ID of the newly registered
        attribute.  If the \a id member of the ::kcdb_attrib object is
        set to KCDB_ATTR_INVALID, then a unique ID is generated. */
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_register(const kcdb_attrib * attrib, 
                     khm_int32 * new_id);

/*! \brief Retrieve the attribute descriptor for an attribute 

    The descriptor that is returned must be released through a call to
    kcdb_attrib_release_info()

    If only the validity of the attribute identifier needs to be
    checked, you can pass in NULL for \a attrib.  In this case, if the
    identifier is valid, then the funciton will return
    KHM_ERROR_SUCCESS, otherwise it will return KHM_ERROR_NOT_FOUND.
    
    \see kcdb_attrib_release_info()
    */
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_get_info(khm_int32 id, 
                     kcdb_attrib ** attrib);

/*! \brief Release an attribute descriptor

    \see kcdb_attrib_get_info()
    */
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_release_info(kcdb_attrib * attrib);

/*! \brief Unregister an attribute 

    Once an attribute ID has been unregistered, it may be reclaimed by
    a subsequent call to kcdb_attrib_register().
*/
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_unregister(khm_int32 id);

/*! \brief Retrieve the description of an attribute 

    \param[in] flags Specify \a KCDB_TS_SHORT to retrieve the short description. */
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_describe(khm_int32 id, 
                     wchar_t * buffer, 
                     khm_size * cbsize, 
                     khm_int32 flags);

/*! \brief Count attributes

    Counts the number of attributes that match the given criteria.
    The criteria is specified against the flags of the attribute.  An
    attribute is a match if its flags satisfy the condition below:

    \code
    (attrib.flags & and_flags) == (eq_flags & and_flags)
    \endcode

    The number of attributes that match are returned in \a pcount.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_get_count(khm_int32 and_flags,
                      khm_int32 eq_flags,
                      khm_size * pcount);

/*! \brief List attribute identifiers

    Lists the identifiers of the attributes that match the given
    criteria.  The criteria is specified against the flags of the
    attribute.  An attribute is a match if the following condition is
    satisfied:

    \code
    (attrib.flags & and_flags) == (eq_flags & and_flags)
    \endcode

    The list of attributes found are copied to the \a khm_int32 array
    specified in \a plist.  The number of elements available in the
    buffer \a plist is specified in \a pcsize.  On exit, \a pcsize
    will hold the actual number of attribute identifiers copied to the
    array.

    \param[in] and_flags See above
    \param[in] eq_flags See above
    \param[in] plist A khm_int32 array
    \param[in,out] pcsize On entry, holds the number of elements
        available in the array pointed to by \a plist.  On exit, holds
        the number of elements copied to the array.

    \retval KHM_ERROR_SUCCESS The list of attribute identifiers have
        been copied.
    \retval KHM_ERROR_TOO_LONG The list was too long to fit in the
        supplied buffer.  As many elements as possible have been
        copied to the \a plist array and the required number of
        elements has been written to \a pcsize.

    \note The \a pcsize parameter specifies the number of khm_int32
        elements in the array and not the number of bytes in the
        array.  This is different from the usual size parameters used
        in the NetIDMgr API.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_attrib_get_ids(khm_int32 and_flags,
                    khm_int32 eq_flags,
                    khm_int32 * plist,
                    khm_size * pcsize);

/*! \defgroup kcdb_credattr_flags Attribute flags */
/*@{*/
/*! \brief The attribute is required */
#define KCDB_ATTR_FLAG_REQUIRED 0x00000008

/*! \brief The attribute is computed.

    If this flag is set, the \a compute_cb, \a compute_min_cbsize and
    \a compute_max_cbsize members of the ::kcdb_attrib attribute
    descriptor must be assigned valid values.
*/
#define KCDB_ATTR_FLAG_COMPUTED 0x00000010

/*! \brief System attribute.

    This cannot be specified for a custom attribute.  Implies that the
    value of the attribute is given by the credentials database
    itself.
*/
#define KCDB_ATTR_FLAG_SYSTEM   0x00000020

/*! \brief Hidden

    The attribute is not meant to be displayed to the user.  Setting
    this flag prevents this attribute from being listed in the list of
    available data fields in the UI.
*/
#define KCDB_ATTR_FLAG_HIDDEN   0x00000040

/*! \brief Property

    The attribute is a property.  The main difference between regular
    attributes and properties are that properties are not allocated
    off the credentials record.  Hence, a property can not be used as
    a credentials field.  Other objects such as identities can hold
    property sets.  A property set can hold both regular attributes as
    well as properties.
*/
#define KCDB_ATTR_FLAG_PROPERTY 0x00000080

/*! \brief Volatile

    A volatile property is one whose value changes often, such as
    ::KCDB_ATTR_TIMELEFT.  Some controls will make use of additional
    logic to deal with such values, or not display them at all.
 */
#define KCDB_ATTR_FLAG_VOLATILE 0x00000100

/*! \brief Alternate view

    The attribute is actually an alternate representation of another
    attribute.  The Canonical attribute name is specified in \a
    alt_id.

    Sometimes a certain attribute may need to be represented in
    different ways.  You can register multiple attributes for each
    view.  However, you should also provide a canonical attribute for
    whenever the canonical set of attributes of the credential is
    required.
 */
#define KCDB_ATTR_FLAG_ALTVIEW  0x00000200

/*! \brief Transient attribute

    A transient attribute is one whose absence is meaningful.  When
    updating one record using another, if a transient attribute is
    absent in the source but present in the destination, then the
    attribute is removed from the destination.
*/
#define KCDB_ATTR_FLAG_TRANSIENT 0x00000400

/*@}*/

/*! \defgroup kcdb_credattr_idnames Standard attribute IDs and names */
/*@{*/

/*! \name Attribute related constants */
/*@{*/
/*! \brief Maximum valid attribute ID */
#define KCDB_ATTR_MAX_ID        255

/*! \brief Minimum valid property ID */
#define KCDB_ATTR_MIN_PROP_ID   4096

/*! \brief Maximum number of properties */
#define KCDB_ATTR_MAX_PROPS     128

/*! \brief Maximum valid property ID */
#define KCDB_ATTR_MAX_PROP_ID (KCDB_ATTR_MIN_PROP_ID + KCDB_ATTR_MAX_PROPS - 1)

/*! \brief Invalid ID */
#define KCDB_ATTR_INVALID   (-1)

/*! \brief First custom attribute ID */
#define KCDB_ATTRID_USER        20

/*@}*/

/*!\name Attribute identifiers  */
/*@{*/
/*! \brief Name of the credential

    - \b Type: STRING
    - \b Flags: REQUIRED, COMPUTED, SYSTEM
 */
#define KCDB_ATTR_NAME          0

/*! \brief The identity handle for the credential

    - \b Type: INT64
    - \b Flags: REQUIRED, COMPUTED, SYSTEM, HIDDEN

    \note The handle returned in by specifying this attribute to
        kcdb_cred_get_attr() or kcdb_cred_get_attrib() is not held.
        While the identity is implicitly held for the duration that
        the credential is held, it is not recommended to obtain a
        handle to the identity using this method.  Use
        kcdb_cred_get_identity() instead.
*/
#define KCDB_ATTR_ID            1

/*! \brief The name of the identity

    - \b Type: STRING
    - \b Flags: REQUIRED, COMPUTED, SYSTEM, ALTVIEW, HIDDEN
    - \b Alt ID: KCDB_ATTR_ID
 */
#define KCDB_ATTR_ID_NAME       2

/*! \brief The type of the credential

    - \b Type: INT32
    - \b Flags: REQUIRED, COMPUTED, SYSTEM, HIDDEN
*/
#define KCDB_ATTR_TYPE          3

/*! \brief Type name for the credential 

    - \b Type: STRING
    - \b Flags: REQUIRED, COMPUTED, SYSTEM, ALTVIEW
    - \b Alt ID: KCDB_ATTR_TYPE
*/
#define KCDB_ATTR_TYPE_NAME     4

/*! \brief Name of the parent credential 

    - \b Type: STRING
    - \b Flags: SYSTEM, HIDDEN
*/
#define KCDB_ATTR_PARENT_NAME   5

/*! \brief Issed on 

    - \b Type: DATE
    - \b Flags: SYSTEM
*/
#define KCDB_ATTR_ISSUE         6

/*! \brief Expires on 

    - \b Type: DATE
    - \b Flags: SYSTEM
*/
#define KCDB_ATTR_EXPIRE        7

/*! \brief Renewable period expires on 

    - \b Type: DATE
    - \b Flags: SYSTEM
*/
#define KCDB_ATTR_RENEW_EXPIRE  8

/*! \brief Time left till expiration 

    - \b Type: INTERVAL
    - \b Flags: SYSTEM, COMPUTED, VOLATILE, ALTVIEW
    - \b Alt ID: KCDB_ATTR_EXPIRE
*/
#define KCDB_ATTR_TIMELEFT      9

/*! \brief Renewable time left

    - \b Type: INTERVAL
    - \b Flags: SYSTEM, COMPUTED, VOLATILE, ALTVIEW
    - \b Alt ID: KCDB_ATTR_RENEW_EXPIRE
 */
#define KCDB_ATTR_RENEW_TIMELEFT 10

/*! \brief Location of the credential

    - \b Type: STRING
    - \b Flags: SYSTEM
*/
#define KCDB_ATTR_LOCATION      11

/*! \brief Lifetime of the credential 

    - \b Type: INTERVAL
    - \b Flags: SYSTEM
*/
#define KCDB_ATTR_LIFETIME      12

/*! \brief Renewable lifetime of the credential

    - \b Type: INTERVAL
    - \b Flags: SYSTEM
 */
#define KCDB_ATTR_RENEW_LIFETIME 13

/*! \brief Flags for the credential

    - \b Type: INT32
    - \b Flags: REQUIRED, COMPUTED, SYSTEM, HIDDEN
 */
#define KCDB_ATTR_FLAGS         14

/*! \brief Display name

  This is the display name that will be used to display the credential
  or identity to the user. If this attribute is not found, then
  KCDB_ATTR_NAME will be used instead.

  The display name is expected to be localized, while the name is not.

    - \b Type: STRING
    - \b Flags: SYSTEM
 */
#define KCDB_ATTR_DISPLAY_NAME  15

/*! \brief Time at which the object was last updated

    - \b Type: DATE
    - \b Flags: SYSTEM, COMPUTED
 */
#define KCDB_ATTR_LAST_UPDATE   16

/*! \brief Display name

    - \b Type: STRING
    - \b Flags: COMPUTED, SYSTEM, ALTVIEW
    - \b Alt ID: KCDB_ATTR_ID
 */
#define KCDB_ATTR_ID_DISPLAY_NAME 17

/*! \brief Status string

    - \b Type: STRING
    - \b Flags: SYSTEM
  */
#define KCDB_ATTR_STATUS        18

/* KCDB_ATTRID_USER is 20 */

/*! \brief Number of credentials

  Number of credentials in the root credentials set that are
  associated with this identity.

    - \b Type: INT32
    - \b Flags: SYSTEM, COMPUTED, PROPERTY
 */
#define KCDB_ATTR_N_CREDS       (KCDB_ATTR_MIN_PROP_ID + 0)

/*! \brief Number of identity credentials

  Number of identity credentials in the root credentials set that are
  associated with this identity.

  An identity credential is a credential that is of the identity
  credentials type.

    - \b Type: INT32
    - \b Flags: SYSTEM, COMPUTED, PROPERTY

 */
#define KCDB_ATTR_N_IDCREDS     (KCDB_ATTR_MIN_PROP_ID + 1)

/*! \brief Number of initial credentials

  Number of identity credentials in the root credentials set that are
  associated with this identity and are marked as initial credentials.

    - \b Type: INT32
    - \b Flags: SYSTEM, COMPUTED, PROPERTY
*/
#define KCDB_ATTR_N_INITCREDS   (KCDB_ATTR_MIN_PROP_ID + 2)

/*! \brief Extended parameter

  When kcdb_identity_create_ex() is called to create a new identity,
  the caller can specify a parameter that the identity provider may
  use.  This parameter is stored in ths attribute.

    - \b Type: DATA
    - \b Flags: SYSTEM, PROPERTY, HIDDEN
 */
#define KCDB_ATTR_PARAM         (KCDB_ATTR_MIN_PROP_ID + 3)

/*! \brief Renewal threshold

  This is set to the renewal threshold of the identity as a FILETIME
  interval.  If renewals are disabled for this identity, the value
  would be 0.  If renewals should be performed using the half-life
  algorithm, the value would be 1.

    - \b Type: INTERVAL
    - \b Flags: SYSTEM, PROPERTY, HIDDEN, COMPUTED
 */
#define KCDB_ATTR_THR_RENEW     (KCDB_ATTR_MIN_PROP_ID + 4)

/*! \brief Warning threshold

  This is set to the warning threshold of the identity as a FILETIME
  interval.  If warnings are disabled for this identity, the value
  would be 0.

    - \b Type: INTERVAL
    - \b Flags: SYSTEM, PROPERTY, HIDDEN, COMPUTED
 */
#define KCDB_ATTR_THR_WARN      (KCDB_ATTR_MIN_PROP_ID + 5)

/*! \brief Critical threshold

  This is set to the critical threshold of the identity as a FILETIME
  interval.  If warnings are disabled for this identity, the value
  would be 0.

    - \b Type: INTERVAL
    - \b Flags: SYSTEM, PROPERTY, HIDDEN, COMPUTED
 */
#define KCDB_ATTR_THR_CRIT      (KCDB_ATTR_MIN_PROP_ID + 6)

/*! \brief E-Mail

  E-mail address associated with identity.  This is an optional
  property that should be provided by an identity provider or a
  credentials provider when the attribute is relevant.
 */
#define KCDB_ATTR_NAME_EMAIL    (KCDB_ATTR_MIN_PROP_ID + 7)

/*! \brief Domain

  DNS domain associated with identity.  This is an optional property
  that should be provided by an identity provider or a credentials
  provider when the attribute is relevant.
 */
#define KCDB_ATTR_NAME_DOMAIN   (KCDB_ATTR_MIN_PROP_ID + 8)

/*@}*/

/*!\name Attribute names */
/*@{ */

#define KCDB_ATTRNAME_NAME          L"Name"
#define KCDB_ATTRNAME_ID            L"Identity"
#define KCDB_ATTRNAME_ID_NAME       L"IdentityName"
#define KCDB_ATTRNAME_ID_DISPLAY_NAME L"IdentityDisplayName"
#define KCDB_ATTRNAME_TYPE          L"TypeId"
#define KCDB_ATTRNAME_TYPE_NAME     L"TypeName"
#define KCDB_ATTRNAME_FLAGS         L"Flags"

#define KCDB_ATTRNAME_PARENT_NAME   L"Parent"
#define KCDB_ATTRNAME_ISSUE         L"Issued"
#define KCDB_ATTRNAME_EXPIRE        L"Expires"
#define KCDB_ATTRNAME_RENEW_EXPIRE  L"RenewExpires"
#define KCDB_ATTRNAME_TIMELEFT      L"TimeLeft"
#define KCDB_ATTRNAME_RENEW_TIMELEFT L"RenewTimeLeft"
#define KCDB_ATTRNAME_LOCATION      L"Location"
#define KCDB_ATTRNAME_LIFETIME      L"Lifetime"
#define KCDB_ATTRNAME_RENEW_LIFETIME L"RenewLifetime"
#define KCDB_ATTRNAME_DISPLAY_NAME  L"DisplayName"
#define KCDB_ATTRNAME_STATUS        L"Status"
#define KCDB_ATTRNAME_LAST_UPDATE   L"LastUpdate"
#define KCDB_ATTRNAME_N_CREDS       L"NCredentials"
#define KCDB_ATTRNAME_N_IDCREDS     L"NIDCredentials"
#define KCDB_ATTRNAME_N_INITCREDS   L"NInitCredentials"
#define KCDB_ATTRNAME_PARAM         L"ExtendedParameter"
#define KCDB_ATTRNAME_THR_RENEW     L"IdentityRenewalThreshold"
#define KCDB_ATTRNAME_THR_WARN      L"IdentityWarningThreshold"
#define KCDB_ATTRNAME_THR_CRIT      L"IdentityCriticalThreshold"
#define KCDB_ATTRNAME_NAME_EMAIL    L"IdentityNameEmail"
#define KCDB_ATTRNAME_NAME_DOMAIN   L"IdentityNameDomain"
/*@}*/

/*@}*/

/*@}*/

/*****************************************************************************/

/*! \defgroup kcdb_credtype Credential types */
/*@{*/

/*! \brief Credential type descriptor */
typedef struct tag_kcdb_credtype {
    wchar_t * name;     /*!< name (less than KCDB_MAXCB_NAME bytes) */
    khm_int32 id;
    wchar_t * short_desc;       /*!< short localized description (less
                                  than KCDB_MAXCB_SHORT_DESC bytes) */
    wchar_t * long_desc;        /*!< long localized descriptionn (less
                                  than KCDB_MAXCB_LONG_DESC bytes) */
    khm_handle sub;             /*!< Subscription for credentials type
                                  hander.  This should be a valid
                                  subscription constructed through a
                                  call to kmq_create_subscription()
                                  and must handle KMSG_CRED messages
                                  that are marked as being sent to
                                  type specific subscriptions.

                                  The subscription will be
                                  automatically deleted with a call to
                                  kmq_delete_subscription() when the
                                  credentials type is unregistered.*/

    kcdb_comp_func is_equal; /*!< Used as an additional clause when
                                  comparing two credentials for
                                  equality.  The function is actually
                                  a comparison function, it should
                                  return zero if the two credentials
                                  are equal and non-zero if they are
                                  not.  The addtional \a rock
                                  parameter is always zero.

                                  It can be assumed that the identity,
                                  name and credentials type have
                                  already been found to be equal among
                                  the credentials and the credential
                                  type is the type that is being
                                  registered.*/

#ifdef _WIN32
    HICON icon;
#endif
} kcdb_credtype;

/*! \brief Maximum value of a credential type identifier

    Credential type identifiers are assigned serially unless the
    process registering the credential type sets a specific identity.
    The maximum identifier number places a hard limit to the number of
    credential types that can be registered at one time, which is
    KCDB_CREDTYPE_MAX_ID + 1.
 */
#define KCDB_CREDTYPE_MAX_ID 31

/*! \brief Specify all credential types

    This value is used by functions which filter credentials based on
    credential types.  Specifying this value tells the filter to
    accept all credential types.
 */
#define KCDB_CREDTYPE_ALL (-1)

/*! \brief Automatically determine a credential type identifier

    Used with kcdb_credtype_register() to specify that the credential
    type identifier should be automatically determined to avoid
    collisions.
 */
#define KCDB_CREDTYPE_AUTO (-2)

/*! \brief An invalid credential type

    Even though any non positive credential type ID is invalid
    anywhere where a specific credential type ID is required, this
    value is provided for explicit indication that the credential type
    is invalid.  Also it makes code more readable to have a constant
    that shouts out INVALID.

*/
#define KCDB_CREDTYPE_INVALID (-3)

/*! \brief Macro predicate for testing whether a credtype is valid

    Returns TRUE if the given credtype is valid.  This is a safe
    macro.
*/
#define KCDB_CREDTYPE_IS_VALID(t) ((t) >= 0)

/*! \brief Register a credentials type.

    The information given in the \a type parameter is used to register
    a new credential type.  Note that the \a name member of the \a
    type should be unique among all credential types.

    You can specify ::KCDB_CREDTYPE_AUTO as the \a id member of \a
    type to let kcdb_credtype_register() determine a suitable
    credential type identifier.  You can subsequently call
    kcdb_credtype_get_id() to retrieve the generated id or pass a
    valid pointer to a khm_int32 type variable as \a new_id.

    \param[in] type Credential type descriptor

    \param[out] new_id The credential type identifier that this type
        was registered as.

    \retval KHM_ERROR_SUCCESS The credential type was successfully registered.

    \retval KHM_ERROR_INVALID_PARAM One or more of the parameters were invalid

    \retval KHM_ERROR_TOO_LONG One or more of the string fields in \a
        type exceeded the character limit for that field.

    \retval KHM_ERROR_NO_RESOURCES When autogenerating credential type
        identifiers, this value indicates that the maximum number of
        credential types have been registered.  No more registrations
        can be accepted unless some credentials type is unregisred.

    \retval KHM_ERROR_DUPLICATE The \a name or \a id that was
        specified is already in use.
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credtype_register(const kcdb_credtype * type, 
                       khm_int32 * new_id);

/*! \brief Return a held reference to a \a kcdb_credtype object describing the credential type.

    The reference points to a static internal object of type \a
    kcdb_credtype.  Use the kcdb_credtype_release_info() function to
    release the reference.

    Also, the structure passed in as the \a type argument to
    kcdb_credtype_register() is not valid as a credential type
    descriptor.  Use kcdb_credtype_get_info() to obtain the actual
    credential type descriptor.

    \param[in] id Credentials type identifier.

    \param[out] type Receives the credentials descriptor handle.  If
        \a type is NULL, then no handle is returned.  However, the
        function will still return \a KHM_ERROR_SUCCESS if the \a id
        parameter passed in is a valid credentials type identifier.

    \see kcdb_credtype_release_info()
    \see kcdb_credtype_register()
*/
KHMEXP khm_int32 KHMAPI 
kcdb_credtype_get_info(khm_int32 id, 
                       kcdb_credtype ** type);

/*! \brief Release a reference to a \a kcdb_credtype object

    Undoes the hold obtained on a \a kcdb_credtype object from a
    previous call to kcdb_credtype_get_info().

    \see kcdb_credtype_get_info()
 */
KHMEXP khm_int32 KHMAPI 
kcdb_credtype_release_info(kcdb_credtype * type);

/*! \brief Unregister a credentials type

    Undoes the registration performed by kcdb_credtype_register().

    This should only be done when the credentials provider is being
    unloaded.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_credtype_unregister(khm_int32 id);

/*! \brief Retrieve the name of a credentials type

    Given a credentials type identifier, retrieves the name.  The name
    is not localized and serves as a persistent identifier of the
    credentials type.

    \param[out] buf The buffer to receive the name.  Could be \a NULL
        if only the length of the buffer is required.

    \param[in,out] cbbuf On entry, specifies the size of the buffer
        pointed to by \a buf if \a buf is not NULL.  On exit, contains
        the number of bytes copied to \a buf or the required size of
        the buffer.

    \retval KHM_ERROR_SUCCESS The call succeeded.

    \retval KHM_ERROR_TOO_LONG Either \a buf was NULL or the supplied
        buffer was not large enough.  The required size is in \a cbbuf.

    \retval KHM_ERROR_INVALID_PARAM Invalid parameter.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_credtype_get_name(khm_int32 id,
                       wchar_t * buf,
                       khm_size * cbbuf);

/*! \brief Retrieve the type specific subscription for a type

    Given a credentials type, this function returns the credentials
    type specific subcription.  It may return NULL if the subscription
    is not available.

    \note The returned handle should NOT be released.
 */
KHMEXP khm_handle KHMAPI 
kcdb_credtype_get_sub(khm_int32 id);

/*! \brief Get the description of a credentials type

   Unlike the name of a credential type, the description is localized.

   \param[in] id Credentials type identifier

   \param[out] buf Receives the description.  Can bet set to NULL if
       only the size of the buffer is required.

   \param[in,out] cbbuf On entry, specifies the size of the buffer
       pointed to by \a buf.  On exit, specifies the required size of
       the buffer or the number of bytes copied, depending on whether
       the call succeeded or not.

   \param[in] flags Specify ::KCDB_TS_SHORT if the short version of
       the description is desired if there is more than one.

   \retval KHM_ERROR_SUCCESS The call succeeded
   \retval KHM_ERROR_TOO_LONG Either \a buf was NULL or the supplied buffer was insufficient.  The required size is specified in \a cbbuf.
   \retval KHM_ERROR_INVALID_PARAM One or more parameters were invalid.
 */
KHMEXP khm_int32 KHMAPI 
kcdb_credtype_describe(khm_int32 id,
                       wchar_t * buf,
                       khm_size * cbbuf,
                       khm_int32 flags);

/*! \brief Look up the identifier of a credentials type by name

    Given a name, looks up the identifier.

    \param[in] name Name of the credentials type
    \param[out] id Receives the identifier if the call succeeds

 */
KHMEXP khm_int32 KHMAPI 
kcdb_credtype_get_id(const wchar_t * name, 
                     khm_int32 * id);

/*@}*/

/********************************************************************/

/* Notification operation constants */

#define KCDB_OP_INSERT      1
#define KCDB_OP_DELETE      2
#define KCDB_OP_MODIFY      3
#define KCDB_OP_ACTIVATE    4
#define KCDB_OP_DEACTIVATE  5
#define KCDB_OP_HIDE        6
#define KCDB_OP_UNHIDE      7
#define KCDB_OP_SETSEARCH   8
#define KCDB_OP_UNSETSEARCH 9
#define KCDB_OP_NEW_DEFAULT 10
#define KCDB_OP_DELCONFIG   11
#define KCDB_OP_RESUPDATE   12
#define KCDB_OP_NEWPARENT   13

/*@}*/

END_C

#ifdef __cplusplus

#include<kconfig.h>

namespace nim {

template<class T> class _EnumerationImpl {
protected:
    kcdb_enumeration e;
    khm_size n;
    T *pT;

public:
    _EnumerationImpl(kcdb_enumeration _e, khm_size _n) {
        e = _e;
        n = _n;
        pT = new T();
        Next();
    }

    ~_EnumerationImpl() {
        kcdb_enum_end(e);
        delete pT;
    }

    T* Next() {
        khm_handle h = pT->Detach();
        kcdb_enum_next(e, &h);
        pT->Attach(h);
        if (h != NULL)
            return pT;
        else
            return NULL;
    }

    void Reset() {
        kcdb_enum_reset(e);
        Next();
    }

    bool AtEnd() const {
        return static_cast<khm_handle>(*pT) == NULL;
    }

    T* operator ++() {
        return Next();
    }

    T& operator *() {
        return *pT;
    }

    T* operator ->() {
        return pT;
    }

    khm_size Size() const {
        return n;
    }

    typedef khm_int32 (KHMCALLBACK * Comparator)(const T& e1, const T& e2, void * param);
    typedef khm_boolean (KHMCALLBACK * FilterProc)(const T& e, void * param);

private:
    struct ComparatorData {
        Comparator f;
        void * param;
    };

    static khm_int32 KHMCALLBACK InternalComparator(khm_handle h1, khm_handle h2, void * param) {
        ComparatorData * d = reinterpret_cast<ComparatorData *>(param);
        T t1(h1, false);
        T t2(h2, false);
        return d->f(t1, t2, d->param);
    }

    struct FilterProcData {
        FilterProc f;
        void * param;
    };

    static khm_boolean KHMCALLBACK InternalFilterProc(khm_handle h, void * param) {
        FilterProcData * d = reinterpret_cast<FilterProcData *>(param);
        T t(h, false);
        return d->f(t, d->param);
    }

public:

    khm_int32 Sort(Comparator f, void * param = NULL) {
        khm_int32 rv;
        ComparatorData d;

        d.f = f;
        d.param = param;
        rv = kcdb_enum_sort(e, InternalComparator, &d);
        Reset();
        return rv;
    }

    khm_int32 Sort(kcdb_comp_func f, void * param = NULL) {
        khm_int32 rv;

        rv = kcdb_enum_sort(e, f, param);
        Reset();
        return rv;
    }

    khm_int32 Filter(FilterProc f, void * param = NULL) {
        khm_int32 rv;
        FilterProcData d;

        d.f = f;
        d.param = param;
        rv = kcdb_enum_filter(e, InternalFilterProc, &d);
        return rv;
    }
};

template<class T> class _BufferImpl {

public:

    T& Attach(khm_handle _h) {
        T *pT = static_cast<T*>(this);
        if (pT->h)
            pT->Release();
        pT->h = _h;
        return *pT;
    }

    khm_handle Detach() {
        T *pT = static_cast<T*>(this);
        khm_handle th = pT->h;
        if (pT->h) {
            pT->h = NULL;
        }
        return th;
    }

    T& Assign(khm_handle _h) {
        T *pT = static_cast<T*>(this);
        if (pT->h)
            pT->Release();
        pT->h = _h;
        if (pT->h)
            pT->Hold();
        return *pT;
    }

    void Unassign() {
        T *pT = static_cast<T*>(this);
        pT->Assign(NULL);
    }

    operator khm_handle () const {
        const T *pT = static_cast<const T*>(this);
        return pT->h;
    }

    khm_handle GetHandle() {
        T *pT = static_cast<T*>(this);
        pT->Hold();
        return pT->h;
    }

    std::wstring GetAttribStringObj(const wchar_t * attr_name, khm_int32 flags = 0) const {
        const T *pT = static_cast<const T*>(this);
        std::wstring ret;
        khm_size cb_buf;
        wchar_t * temp = NULL;
        khm_int32 rv;

        rv = pT->GetAttribString(attr_name, NULL, &cb_buf, flags);
        while (rv == KHM_ERROR_TOO_LONG) {
            wchar_t * t2 = static_cast<wchar_t*>(PREALLOC(temp, cb_buf));
            if (t2 == NULL)
                break;
            temp = t2;
            rv = pT->GetAttribString(attr_name, temp, &cb_buf, flags);
        }

        if (KHM_SUCCEEDED(rv))
            ret = temp;
        if (temp)
            PFREE(temp);

        return ret;
    }

    std::wstring GetAttribStringObj(khm_int32 attr_id, khm_int32 flags = 0) const {
        const T *pT = static_cast<const T*>(this);
        std::wstring ret;
        khm_size cb_buf = 0;
        wchar_t * temp = NULL;
        khm_int32 rv = KHM_ERROR_TOO_LONG;

        while (rv == KHM_ERROR_TOO_LONG) {
            if (cb_buf != 0) {
                wchar_t * t2;
                t2 = reinterpret_cast<wchar_t *>(PREALLOC(temp, cb_buf));
                if (t2 == NULL) {
                    if (temp) {
                        PFREE(temp);
                        temp = NULL;
                    }
                    break;
                }
                temp = t2;
            }
            rv = pT->GetAttribString(attr_id, temp, &cb_buf, flags);
        }

        if (KHM_SUCCEEDED(rv))
            ret = temp;
        if (temp)
            PFREE(temp);

        return ret;
    }

    khm_int32 GetAttribInt32(khm_int32 attr_id, khm_int32 def = 0) const {
        khm_int32 t = 0;

        if (KHM_FAILED(GetObject(attr_id, t)))
            return def;
        else
            return t;
    }

    khm_int64 GetAttribFileTimeAsInt(khm_int32 attr_id, khm_int64 def = 0) const {
        FILETIME ft;

        if (KHM_FAILED(GetObject(attr_id, ft)))
            return def;
        else
            return FtToInt(&ft);
    }

    FILETIME GetAttribFileTime(khm_int32 attr_id) const {
        FILETIME ft = {0,0};

        GetObject(attr_id, ft);
        return ft;
    }

    template<class U> khm_int32 GetObject(const wchar_t * attr_name, U& target) const {
        const T *pT = static_cast<const T*>(this);
        khm_size cb = sizeof(target);
        return pT->GetAttrib(attr_name, &target, &cb);
    }

    template<class U> khm_int32 GetObject(khm_int32 attr_id, U& target) const {
        const T *pT = static_cast<const T*>(this);
        khm_size cb = sizeof(target);
        return pT->GetAttrib(attr_id, &target, &cb);
    }

    template<class U> khm_int32 SetObject(const wchar_t * attr_name, const U& target) {
        T *pT = static_cast<T*>(this);
        return pT->SetAttrib(attr_name, &target, sizeof(target));
    }

    template<class U> khm_int32 SetObject(khm_int32 attr_id, const U& target) {
        T *pT = static_cast<T*>(this);
        return pT->SetAttrib(attr_id, &target, sizeof(target));
    }

    bool Exists(const wchar_t * attr_name) const {
        const T *pT = static_cast<const T*>(this);
        return KHM_SUCCEEDED(pT->GetAttrib(attr_name, NULL, NULL));
    }
        
    bool Exists(khm_int32 attr_id) const {
        const T *pT = static_cast<const T*>(this);
        return KHM_SUCCEEDED(pT->GetAttrib(attr_id, NULL, NULL));
    }

    khm_int32 GetResource(kcdb_resource_id r_id, khm_int32 flags, khm_int32 *prflags,
                          void * vparam, void * buf, khm_size * pcb_buf) const {
        const T *pT = static_cast<const T*>(this);
        return kcdb_get_resource(pT->h, r_id, flags, prflags, vparam, buf, pcb_buf);
    }

    std::wstring GetResourceString(kcdb_resource_id r_id, khm_int32 flags = 0) const {
        const T *pT = static_cast<const T*>(this);
        std::wstring ret;
        wchar_t * temp = NULL;
        khm_size cb = 0;
        khm_int32 rv;

        rv = pT->GetResource(r_id, flags, NULL, NULL, NULL, &cb);
        while (rv == KHM_ERROR_TOO_LONG) {
            wchar_t * t2 = (wchar_t *) PREALLOC(temp, cb);
            if (t2 == NULL)
                break;
            temp = t2;
            rv = pT->GetResource(r_id, flags, NULL, NULL, temp, &cb);
        }

        if (KHM_SUCCEEDED(rv))
            ret = temp;
        if (temp)
            PFREE(temp);

        return ret;
    }

    HICON GetResourceIcon(kcdb_resource_id r_id, khm_int32 flags = 0) const {
        const T *pT = static_cast<const T*>(this);
        HICON icon = NULL;
        khm_size cb = sizeof(icon);

        if (KHM_SUCCEEDED(pT->GetResource(r_id, flags, NULL, NULL, &icon, &cb)))
            return icon;
        else
            return NULL;
    }

    bool operator == (T const &that) const {
        const T *pT = static_cast<const T*>(this);
        return pT->IsEqualTo(that);
    }

    bool operator == (khm_handle that) const {
        const T *pT = static_cast<const T*>(this);
        return pT->IsEqualTo(that);
    }

    bool operator != (T const &that) const {
        const T *pT = static_cast<const T*>(this);
        return !pT->IsEqualTo(that);
    }

    bool operator != (khm_handle that) const {
        const T *pT = static_cast<const T*>(this);
        return !pT->IsEqualTo(that);
    }

    bool IsNull() const {
        const T *pT = static_cast<const T*>(this);

        return (pT->h == NULL);
    }

    typedef _EnumerationImpl<T> Enumeration;
};

template <class T, class U> class _InfoImpl {
protected:
    U         *info;
    khm_int32  last_error;

    _InfoImpl() {
        info = NULL;
        last_error = KHM_ERROR_NOT_READY;
    }

public:
    khm_int32 GetLastError() const {
        const T *pT = static_cast<const T*>(this);
        return (pT->info == NULL)? KHM_ERROR_NOT_READY: pT->last_error;
    }

    const U* operator -> () {
        T *pT = static_cast<T*>(this);
        return pT->info;
    }

    bool InError() const {
        const T *pT = static_cast<const T*>(this);
        return KHM_FAILED(pT->GetLastError());
    }
};

class Buffer : public _BufferImpl<Buffer> {
protected:
    khm_handle h;

    friend class _BufferImpl<Buffer>;

public:
    Buffer() {
        h = NULL;
    }

    explicit
    Buffer(khm_handle buf, bool take_ownership = true) {
        h = buf;
        if (!take_ownership)
            Hold();
    }

    Buffer(const Buffer &that) {
        h = that.h;
        Hold();
    }

    ~Buffer() {
        Release();
        h = NULL; 
    }

    Buffer& operator = (const Buffer& that) {
        if (this != &that)
            return Assign(that.h);
        else
            return *this;
    }

    Buffer& operator = (khm_handle _h) {
        return Assign(_h);
    }

    khm_int32 GetAttrib(const wchar_t * attr_name, void * buffer, khm_size * pcb_buf) const {
        return kcdb_buf_get_attrib(h, attr_name, NULL, buffer, pcb_buf);
    }

    khm_int32 GetAttrib(khm_int32 attr_id, void * buffer, khm_size * pcb_buf) const {
        return kcdb_buf_get_attr(h, attr_id, NULL, buffer, pcb_buf);
    }

    khm_int32 SetAttrib(const wchar_t * attr_name, const void * buffer, khm_size cb_buffer) {
        return kcdb_buf_set_attrib(h, attr_name, buffer, cb_buffer);
    }

    khm_int32 SetAttrib(khm_int32 attr_id, const void * buffer, khm_size cb_buffer) {
        return kcdb_buf_set_attr(h, attr_id, buffer, cb_buffer);
    }

    khm_int32 GetAttribString(const wchar_t * attr_name, wchar_t * buffer, khm_size * pcb_buf, khm_int32 flags) const {
        return kcdb_buf_get_attrib_string(h, attr_name, buffer, pcb_buf, flags);
    }

    khm_int32 GetAttribString(khm_int32 attr_id, wchar_t * buffer, khm_size * pcb_buf, khm_int32 flags) const {
        return kcdb_buf_get_attr_string(h, attr_id, buffer, pcb_buf, flags);
    }

    khm_int32 Hold() {
        return kcdb_buf_hold(h);
    }

    khm_int32 Release() {
        return kcdb_buf_release(h);
    }

    bool IsEqualTo(Buffer const &that) const {
        return (h == that.h);
    }

    bool IsEqualTo(khm_handle that) const {
        return (h == that);
    }
};

class CredentialTypeInfo : public _InfoImpl<CredentialTypeInfo, kcdb_credtype> {
private:
    void Release() {
        if (info) {
            kcdb_credtype_release_info(info);
            info = NULL;
        }
    }

    friend class _InfoImpl<CredentialTypeInfo, kcdb_credtype>;

public:
    CredentialTypeInfo() {}

    CredentialTypeInfo(khm_int32 _ctype) {
        Lookup(_ctype);
    }

    CredentialTypeInfo(const CredentialTypeInfo& that) {
        if (that.info != NULL)
            Lookup(that.info->id);
        else
            info = NULL;
    }

    ~CredentialTypeInfo() {
        Release();
    }

    khm_int32 Lookup(khm_int32 _ctype) {
        last_error = kcdb_credtype_get_info(_ctype, &info);
        return last_error;
    }
};

class CredentialType : public _BufferImpl<CredentialType> {
protected:
    khm_int32 ctype;

    friend class _BufferImpl<CredentialType>;

public:
    CredentialType() {
        ctype = KCDB_CREDTYPE_INVALID;
    }

    CredentialType(khm_int32 _ctype) {
        ctype = _ctype;
    }

    CredentialType(const wchar_t * type_name) {
        Lookup(type_name);
    }

    CredentialType(const CredentialType& that) {
        ctype = that.ctype;
    }

    ~CredentialType() { }

    CredentialType& operator = (const CredentialType& that) {
        if (this != &that) {
            ctype = that.ctype;
        }
        return *this;
    }

    CredentialType& operator = (khm_int32 _ctype) {
        ctype = _ctype;
        return *this;
    }

    khm_int32 Lookup(const wchar_t * type_name) {
        return kcdb_credtype_get_id(type_name, &ctype);
    }

    bool IsEqualTo(CredentialType const &that) const {
        return ctype == that.ctype;
    }

    khm_int32 GetResource(kcdb_resource_id r_id, khm_int32 flags, khm_int32 *prflags,
                          void * vparam, void * buf, khm_size * pcb_buf) const {
        return kcdb_get_resource(KCDB_HANDLE_FROM_CREDTYPE(ctype),
                                 r_id, flags, prflags, vparam, buf, pcb_buf);
    }

    operator khm_int32 () const {
        return ctype;
    }

    CredentialTypeInfo GetInfo() const {
        return CredentialTypeInfo(ctype);
    }

    bool operator == (const CredentialType & that) const {
        return ctype == that.ctype;
    }

    bool operator == (khm_int32 that) const {
        return ctype == that;
    }

    bool operator != (const CredentialType & that) const {
        return ctype != that.ctype;
    }

    bool operator != (khm_int32 that) const {
        return ctype != that;
    }
};

class Identity;

class IdentityProvider : public _BufferImpl<IdentityProvider> {
protected:
    khm_handle h;

    friend class _BufferImpl<IdentityProvider>;

public:
    IdentityProvider() {
        h = NULL;
    }

    explicit
    IdentityProvider(khm_handle _h, bool take_ownership = true) {
        h = _h;
        if (!take_ownership)
            Hold();
    }

    IdentityProvider(const wchar_t * name) {
        kcdb_identpro_find(name, &h);
    }

    IdentityProvider(const IdentityProvider &that) {
        h = that.h;
        Hold();
    }

    ~IdentityProvider() {
        Release();
        h = NULL;
    }

    IdentityProvider& operator = (const IdentityProvider& that) {
        if (this != &that)
            return Assign(that.h);
        else
            return *this;
    }

    IdentityProvider& operator = (khm_handle _h) {
        return Assign(_h);
    }

    khm_int32 Hold() {
        return kcdb_identpro_hold(h);
    }

    khm_int32 Release() {
        return kcdb_identpro_release(h);
    }

    bool IsEqualTo(IdentityProvider const &that) const {
        return !!kcdb_identpro_is_equal(h, that.h);
    }

    bool IsEqualTo(khm_handle that) const {
        return !!kcdb_identpro_is_equal(h, that);
    }

    std::wstring GetName() const {
        wchar_t wname[KCDB_MAXCCH_NAME];
        khm_size cb = sizeof(wname);

        if (KHM_FAILED(kcdb_identpro_get_name(h, wname, &cb)))
            wname[0] = L'\0';

        return std::wstring(wname);
    }

    CredentialType GetType() const {
        khm_int32 ictype = KCDB_CREDTYPE_INVALID;
        kcdb_identpro_get_type(h, &ictype);

        return CredentialType(ictype);
    }

    Identity GetDefaultIdentity() const;

    static Enumeration Enum() {
        khm_size n = 0;
        kcdb_enumeration e = NULL;

        kcdb_identpro_begin_enum(&e, &n);

        return Enumeration(e, n);
    }

    static IdentityProvider GetDefault() {
        khm_handle h = NULL;

        kcdb_identpro_get_default(&h);
        return IdentityProvider(h);
    }
};

class Identity : public _BufferImpl<Identity> {
protected:
    khm_handle h;

    friend class _BufferImpl<Identity>;

public:
    Identity() { h = NULL; }

    explicit
    Identity(khm_handle _identity, bool take_ownership = true) {
        h = _identity;
        if (!take_ownership)
            Hold();
    }

    Identity(IdentityProvider &provider, const wchar_t * name, khm_int32 flags, void * vparam = NULL) {
        h = NULL;
        Create(provider, name, flags, vparam);
    }

    Identity(const Identity &that) {
        h = that.h;
        Hold();
    }

    ~Identity() { 
        Release();
        h = NULL; 
    }

    Identity& operator = (const Identity& that) {
        if (this != &that)
            return Assign(that.h);
        else
            return *this;
    }

    Identity& operator = (khm_handle _h) {
        return Assign(_h);
    }

    khm_int32 Create(IdentityProvider &provider, const wchar_t * name, khm_int32 flags,
                     void * vparam = NULL) {
        Release();
        h = NULL;
        return kcdb_identity_create_ex(static_cast<khm_handle>(provider), name, flags,
                                       vparam, &h);
    }

    khm_int32 GetAttrib(const wchar_t * attr_name, void * buffer, khm_size * pcb_buf) const {
        return kcdb_identity_get_attrib(h, attr_name, NULL, buffer, pcb_buf);
    }

    khm_int32 GetAttrib(khm_int32 attr_id, void * buffer, khm_size * pcb_buf) const {
        return kcdb_identity_get_attr(h, attr_id, NULL, buffer, pcb_buf);
    }

    khm_int32 SetAttrib(const wchar_t * attr_name, const void * buffer, khm_size cb_buffer) {
        return kcdb_identity_set_attrib(h, attr_name, buffer, cb_buffer);
    }

    khm_int32 SetAttrib(khm_int32 attr_id, const void * buffer, khm_size cb_buffer) {
        return kcdb_identity_set_attr(h, attr_id, buffer, cb_buffer);
    }

    khm_int32 GetAttribString(const wchar_t * attr_name, wchar_t * buffer, khm_size * pcb_buf, khm_int32 flags = 0) const {
        return kcdb_identity_get_attrib_string(h, attr_name, buffer, pcb_buf, flags);
    }

    khm_int32 GetAttribString(khm_int32 attr_id, wchar_t * buffer, khm_size * pcb_buf, khm_int32 flags = 0) const {
        return kcdb_identity_get_attr_string(h, attr_id, buffer, pcb_buf, flags);
    }

    khm_int32 Hold() {
        return kcdb_identity_hold(h);
    }

    khm_int32 Release() {
        return kcdb_identity_release(h);
    }

    bool IsEqualTo(Identity const &that) const {
        return !!kcdb_identity_is_equal(h, that.h);
    }

    bool IsEqualTo(khm_handle that) const {
        return !!kcdb_identity_is_equal(h, that);
    }

    khm_int32 SetFlags(khm_int32 flags, khm_int32 mask) {
        return kcdb_identity_set_flags(h, flags, mask);
    }

    khm_int32 GetFlags() const {
        khm_int32 f = 0;
        kcdb_identity_get_flags(h, &f);
        return f;
    }

    khm_ui_8 GetSerial() const {
        khm_ui_8 serial = 0;
        kcdb_identity_get_serial(h, &serial);
        return serial;
    }

    std::wstring GetName() const {
        wchar_t wname[KCDB_IDENT_MAXCCH_NAME];
        khm_size cb = sizeof(wname);

        if (KHM_FAILED(kcdb_identity_get_name(h, wname, &cb)))
            wname[0] = L'\0';

        return std::wstring(wname);
    }

    CredentialType GetType() const {
        khm_int32 ctype = KCDB_CREDTYPE_INVALID;
        khm_size cb = sizeof(ctype);

        kcdb_identity_get_attr(h, KCDB_ATTR_TYPE, NULL, &ctype, &cb);

        if (ctype >= 0)
            return CredentialType(ctype);
        else
            return CredentialType();
    }

    IdentityProvider GetProvider() const {
        khm_handle pro = NULL;
        kcdb_identity_get_identpro(h, &pro);
        return IdentityProvider(pro);
    }

    ConfigSpace GetConfig(khm_int32 flags) const {
        khm_handle csp = NULL;

        /* TODO: should this also set up the shadow configuration space? */

        if (KHM_SUCCEEDED(kcdb_identity_get_config(h, flags, &csp)))
            return ConfigSpace(csp);
        else
            return ConfigSpace();
    }

    Identity GetParent() const {
        khm_handle p = NULL;

        kcdb_identity_get_parent(h, &p);
        return Identity(p);
    }

    khm_int32 SetParent(Identity &p) {
        return kcdb_identity_set_parent(h, static_cast<khm_handle>(p));
    }

    khm_int32 SetDefault() {
        return kcdb_identity_set_default(h);
    }

    static Enumeration Enum(khm_int32 and_flags, khm_int32 eq_flags) {
        kcdb_enumeration e = NULL;
        khm_size n = 0;

        kcdb_identity_begin_enum(and_flags, eq_flags, &e, &n);
        return Enumeration(e, n);
    }
};

inline Identity IdentityProvider::GetDefaultIdentity() const {
    khm_handle h_id = NULL;
    kcdb_identpro_get_default_identity(h, &h_id);

    return Identity(h_id);
}

class Credential : public _BufferImpl<Credential> {
protected:
    khm_handle h;

    friend class _BufferImpl<Credential>;

public:
    Credential() {
        h = NULL;
    }

    Credential(const wchar_t * name, Identity & identity, CredentialType & ctype) {
        kcdb_cred_create(name, static_cast<khm_handle>(identity),
                         static_cast<khm_int32>(ctype), &h);
    }

    explicit
    Credential(khm_handle _h, bool take_ownership = true) {
        h = _h;
        if (!take_ownership)
            Hold();
    }

    Credential(const Credential &that) {
        h = that.h;
        Hold();
    }

    ~Credential() {
        if (h) {
            Release();
            h = NULL;
        }
    }

    Credential& operator = (const Credential& that) {
        if (this != &that)
            return Assign(that.h);
        else
            return *this;
    }

    Credential& operator = (khm_handle _h) {
        return Assign(_h);
    }

    khm_int32 GetAttrib(const wchar_t * attr_name, void * buffer, khm_size * pcb_buf) const {
        return kcdb_cred_get_attrib(h, attr_name, NULL, buffer, pcb_buf);
    }

    khm_int32 GetAttrib(khm_int32 attr_id, void * buffer, khm_size * pcb_buf) const {
        return kcdb_cred_get_attr(h, attr_id, NULL, buffer, pcb_buf);
    }

    khm_int32 SetAttrib(const wchar_t * attr_name, const void * buffer, khm_size cb_buffer) {
        return kcdb_cred_set_attrib(h, attr_name, buffer, cb_buffer);
    }

    khm_int32 SetAttrib(khm_int32 attr_id, const void * buffer, khm_size cb_buffer) {
        return kcdb_cred_set_attr(h, attr_id, buffer, cb_buffer);
    }

    khm_int32 GetAttribString(const wchar_t * attr_name, wchar_t * buffer, khm_size * pcb_buf, khm_int32 flags = 0) const {
        return kcdb_cred_get_attrib_string(h, attr_name, buffer, pcb_buf, flags);
    }

    khm_int32 GetAttribString(khm_int32 attr_id, wchar_t * buffer, khm_size * pcb_buf, khm_int32 flags = 0) const {
        return kcdb_cred_get_attr_string(h, attr_id, buffer, pcb_buf, flags);
    }

    khm_int32 Hold() {
        return kcdb_cred_hold(h);
    }

    khm_int32 Release() {
        return kcdb_cred_release(h);
    }

    bool IsEqualTo(Credential const &that) const {
        return !!kcdb_creds_is_equal(h, that.h);
    }

    bool IsEqualTo(khm_handle that) const {
        return !!kcdb_creds_is_equal(h, that);
    }

    std::wstring GetName() const {
        wchar_t name[KCDB_MAXCCH_NAME] = L"";
        khm_size cb = sizeof(name);

        kcdb_cred_get_name(h, name, &cb);
        return std::wstring(name);
    }

    Identity GetIdentity() const {
        khm_handle h_id = NULL;

        kcdb_cred_get_identity(h, &h_id);

        return Identity(h_id);
    }

    CredentialType GetType() const {
        khm_int32 ctype = KCDB_CREDTYPE_INVALID;
        kcdb_cred_get_type(h, &ctype);
        return CredentialType(ctype);
    }

    khm_int32 GetFlags() const {
        khm_int32 flags = 0;
        kcdb_cred_get_flags(h, &flags);
        return flags;
    }

    void SetFlags(khm_int32 flags, khm_int32 mask) {
        kcdb_cred_set_flags(h, flags, mask);
    }

    khm_int32 CompareAttrib(const Credential &that, const wchar_t * attrib_name) const {
        return kcdb_creds_comp_attrib(h, static_cast<khm_handle>(that), attrib_name);
    }

    khm_int32 CompareAttrib(const Credential &that, khm_int32 attr_id) const {
        return kcdb_creds_comp_attr(h, static_cast<khm_handle>(that), attr_id);
    }
};


class CredentialSetEnumeration;

class CredentialSet {
protected:
    khm_handle h;
    bool  delete_after_use;

public:
    CredentialSet() {
        h = NULL; 
        delete_after_use = false; 
        Create(); 
    }

    explicit
    CredentialSet(khm_handle _h) {
        h = NULL;
        delete_after_use = false;
        Attach(_h);
    }

    CredentialSet(const CredentialSet& _that) {
        h = NULL;
        delete_after_use = false;
        Assign(_that.h);
    }

    ~CredentialSet() {
        Detach();
    }

    CredentialSet& Attach(khm_handle _h) {
        Detach();
        h = _h;
        delete_after_use = true;
        return *this;
    }

    khm_handle Detach() {
        khm_handle th;
        if (delete_after_use) {
            kcdb_credset_delete(h);
        }
        th = h;
        h = NULL;
        delete_after_use = false;
        return th;
    }

    CredentialSet& Assign(khm_handle _h) {
        Detach();
        h = _h;
        delete_after_use = false;
        return *this;
    }

    void Unassign() {
        Detach();
    }

    CredentialSet& operator = (const CredentialSet& that) {
        if (this != &that)
            return Assign(that.h);
        else
            return *this;
    }

    CredentialSet& operator = (khm_handle _h) {
        return Assign(_h);
    }

    khm_int32 Create() {
        khm_int32 rv;

        Detach();
        rv = kcdb_credset_create(&h);
        if (KHM_SUCCEEDED(rv))
            delete_after_use = true;
        return rv;
    }

    khm_int32 Collect(CredentialSet * cs_from,
                      khm_handle identity = NULL,
                      khm_int32  ctype = KCDB_CREDTYPE_ALL,
                      khm_int32 * delta = NULL) {
        if (h == NULL)
            return KHM_ERROR_INVALID_OPERATION;
        return kcdb_credset_collect(h, (cs_from ? cs_from->h : NULL),
                                    identity, ctype, delta);
    }

    khm_int32 Collect(CredentialSet * cs_from,
                      kcdb_cred_filter_func filter, void * vparam,
                      khm_int32 * delta = NULL) {
        if (h == NULL)
            return KHM_ERROR_INVALID_OPERATION;
        return kcdb_credset_collect_filtered(h, (cs_from ? cs_from->h : NULL),
                                             filter, vparam, delta);
    }

    khm_int32 Flush() {
        return kcdb_credset_flush(h);
    }

    Credential Get(khm_int32 idx) const {
        khm_handle cred = NULL;
        if (KHM_SUCCEEDED(kcdb_credset_get_cred(h, idx, &cred))) {
            return Credential(cred);
        } else {
            return Credential(NULL);
        }
    }

    Credential operator [] (khm_int32 idx) const {
        return Get(idx);
    }

    khm_int32 Delete(khm_int32 idx) {
        return kcdb_credset_del_cred(h, idx);
    }

    khm_int32 Delete(Credential & c) {
        return kcdb_credset_del_cred_ref(h, static_cast<khm_handle>(c));
    }

    khm_int32 Add(Credential & c, khm_int32 idx = -1) {
        return kcdb_credset_add_cred(h, static_cast<khm_handle>(c), idx);
    }

    khm_int32 Add(khm_handle cred, khm_int32 idx = -1) {
        return kcdb_credset_add_cred(h, cred, idx);
    }

    khm_size Size() const {
        khm_size s = 0;
        kcdb_credset_get_size(h, &s);
        return s;
    }

    khm_int32 Purge() {
        return kcdb_credset_purge(h);
    }

    khm_int32 Apply(kcdb_cred_apply_func f, void * vparam) {
        return kcdb_credset_apply(h, f, vparam);
    }

    khm_int32 Sort(kcdb_comp_func comp, void * vparam) {
        return kcdb_credset_sort(h, comp, vparam);
    }

    khm_int32 Seal() {
        return kcdb_credset_seal(h);
    }

    khm_int32 Unseal() {
        return kcdb_credset_unseal(h);
    }

    operator khm_handle () {
        return h;
    }

    typedef CredentialSetEnumeration Enumeration;

    Enumeration Enum() const;

    typedef khm_int32 (KHMCALLBACK * ApplyFunction)(Credential& c, void * param);

    struct _InternalApplyData {
        ApplyFunction f;
        void * f_data;
    };

private:
    static khm_int32 KHMCALLBACK _InternalApplyFunction(khm_handle cred, void * pv) {
        struct _InternalApplyData * data = static_cast<_InternalApplyData *>(pv);
        Credential _cred;
        _cred = cred;
        return (*data->f)(_cred, data->f_data);
    }

public:
    khm_int32 Apply(ApplyFunction f, void * param) {
        struct _InternalApplyData data;
        data.f = f;
        data.f_data = param;
        return kcdb_credset_apply(h, _InternalApplyFunction, &data);
    }
};

class CredentialSetEnumeration {
protected:
    CredentialSet cs;
    Credential    current;
    khm_int32     next_idx;

private:
    CredentialSetEnumeration(const CredentialSet * _cs) : cs(*_cs) {
        Reset();
    }
        
    friend class CredentialSet;

public:
    Credential& Next() {
        if ((khm_size) next_idx >= cs.Size())
            current = (khm_handle) NULL;
        else
            current = cs.Get(next_idx++);
        return current;
    }

    void Reset() {
        next_idx = 0;
        Next();
    }

    bool AtEnd() const {
        return static_cast<khm_handle>(current) == NULL;
    }

    Credential& operator * () {
        return current;
    }

    Credential& operator -> () {
        return current;
    }

    Credential& operator ++ () {
        return Next();
    }
};

inline CredentialSet::Enumeration CredentialSet::Enum() const {
    return Enumeration(this);
}

class AttributeTypeInfo : public _InfoImpl<AttributeTypeInfo, kcdb_type> {
private:
    void Release() {
        if (info) {
            kcdb_type_release_info(info);
            info = NULL;
        }
    }

    friend _InfoImpl<AttributeTypeInfo, kcdb_type>;

public:
    AttributeTypeInfo() {
    }

    AttributeTypeInfo(khm_int32 id) {
        Lookup(id);
    }

    AttributeTypeInfo(const wchar_t * name) {
        Lookup(name);
    }

    AttributeTypeInfo(const AttributeTypeInfo& that) {
        if (that.info != NULL) {
            Lookup(that.info->id);
        } else {
            Release();
        }
    }

    ~AttributeTypeInfo() {
        Release();
    }

    khm_int32 Lookup(khm_int32 id) {
        last_error = kcdb_type_get_info(id, &info);
    }

    khm_int32 Lookup(const wchar_t * name) {
        khm_int32 id = KCDB_TYPE_INVALID;
        Release();
        last_error = kcdb_type_get_id(name, &id);
        if (KHM_SUCCEEDED(last_error)) {
            last_error = kcdb_type_get_info(id, &info);
        }
    }
};

class AttributeInfo : public _InfoImpl<AttributeInfo, kcdb_attrib> {
private:
    friend class _InfoImpl<AttributeInfo, kcdb_attrib>;

    void Release() {
        if (info)
            kcdb_attrib_release_info(info);
        info = NULL;
    }

public:
    AttributeInfo() {
    }

    AttributeInfo(khm_int32 attr_id) {
        Lookup(attr_id);
    }

    AttributeInfo(const wchar_t * attr_name) {
        Lookup(attr_name);
    }

    AttributeInfo(const AttributeInfo& that) {
        if (that.info != NULL)
            Lookup(that.info->id);
        else
            Release();
    }

    ~AttributeInfo() {
        Release();
    }

    khm_int32 Lookup(khm_int32 attr_id) {
        Release();
        return last_error = kcdb_attrib_get_info(attr_id, &info);
    }

    khm_int32 Lookup(const wchar_t * name) {
        khm_int32 attr_id;

        Release();
        if (KHM_SUCCEEDED(last_error = kcdb_attrib_get_id(name, &attr_id)))
            return Lookup(attr_id);
        else
            return last_error;
    }

    std::wstring GetDescription(khm_int32 flags = 0) const {
        std::wstring desc;

        if (info == NULL ||
            (info->short_desc == NULL && info->long_desc == NULL))
            desc = L"";
        else if (info->long_desc == NULL || (flags & KCDB_TS_SHORT) == KCDB_TS_SHORT)
            desc = info->short_desc;
        else
            desc = info->long_desc;
        return desc;
    }

    AttributeTypeInfo GetTypeInfo() const {
        return AttributeTypeInfo(info->type);
    }
};
}

#endif

#endif
