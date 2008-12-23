/*
 * Copyright (c) 2005-2008 Massachusetts Institute of Technology
 * Copyright (c) 2008 Secure Endpoints Inc.
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

#ifndef __KHIMAIRA_KHCONFIGUI_H
#define __KHIMAIRA_KHCONFIGUI_H

/*! \addtogroup khui
@{ */

/*! \defgroup khui_cfg Configuration Panels

    Configuration panels are the primary means from which the user is
    presented with an interface to change NetIDMgr and plugin
    configuration.

@{ */

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Configuration window notification message

    This is the message that will be used to notify dialog panels.

    The format of the message is :
    - uMsg : KHUI_WM_CFG_NOTIFY
    - HIWORD(wParam) : one of ::khui_wm_cfg_notifications

    \note This is the same as ::KHUI_WM_NC_NOTIFY
 */
#define KHUI_WM_CFG_NOTIFY (WM_APP + 0x101)

/*! \brief Configuration notifications

    These are sent thorugh a ::KHUI_WM_CFG_NOTIFY message.

    The format of the message is :
    - uMsg : KHUI_WM_CFG_NOTIFY
    - HIWORD(wParam) : one of ::khui_wm_cfg_notifications
 */
enum khui_wm_cfg_notifications {
    WMCFG_SHOW_NODE = 1,
    /*!< Sent to the configuration dialog to request that the panel
      for the specified node be shown.  The \a lParam message
      parameter will contain a held ::khui_config_node handle.  The
      sender of the mssage is responsible for releasing the handle.*/

    WMCFG_UPDATE_STATE = 2,
    /*!< Sent to the configuration dialog to indicate that the state
      flags for the specified configuration node have changed.

      - LOWORD(wParam) : new flags
      - lParam : ::khui_config_node for the node*/

    WMCFG_APPLY = 3,
    /*!< Sent to all the configuration panels when the user clicks the
      'Apply' button or the 'Ok' button.  The panels are responsible
      for applying the configuration changes and updating their flags
      using khui_cfg_set_flags(). */

    WMCFG_SYNC_NODE_LIST = 4,
    /*!< Sent from the UI library to the configuration window to
      notify the window that the node list has changed.  This message
      is sent synchronously before the node is removed. */

    WMCFG_INIT_PANEL = 5,
    /*!< Sent to all the configuration subpanels after they are
       created.  The subpanels should return TRUE if the subpanel does
       not apply to the context in which they were created. */
};

/*! \brief Registration information for a configuration node

    \see khui_cfg_register()
*/
typedef struct tag_khui_config_node_reg {
    const wchar_t * name;       /*!< Internal identifier
                                  (not-localized, required).  The name
                                  is required to be unique among
                                  sibling nodes.  However it is not
                                  required to be unique globally.  The
                                  size of the name is constrained by
                                  ::KHUI_MAXCCH_NAME*/

    const wchar_t * short_desc; /*!< Short description (Localized,
                                  required).  This is the name which
                                  identifies the node within a
                                  collection of siblings.  The size of
                                  the string is constrained by
                                  ::KHUI_MAXCCH_SHORT_DESC*/

    const wchar_t * long_desc;  /*!< Global name of the node.
                                  (Localized, required).  This
                                  uniquely identifies the node in the
                                  collection of all configuration
                                  nodes.  The size of the string is
                                  constrained by
                                  ::KHUI_MAXCCH_LONG_DESC.*/

    HMODULE   h_module;         /*!< Module which contains the dialog
                                  resource specified in \a
                                  dlg_template */

    LPWSTR    dlg_template;     /*!< Dialog template for the
                                  configuration window */

    DLGPROC   dlg_proc;         /*!< Dialog procedure */

    khm_int32 flags;            /*!< Flags.  Can be a combination of
                                  ::KHUI_CNFLAG_SORT_CHILDREN,
                                  ::KHUI_CNFLAG_SUBPANEL, and ::KHUI_CNFLAG_PLURAL*/

} khui_config_node_reg;

/* We should extend the data structure to include a parameter value.
   This parameter can, for the case of identities, be an identity
   handle. */

/*! \brief Sort the child nodes by short description */
#define KHUI_CNFLAG_SORT_CHILDREN 0x0001

/*! \brief Is a subpanel */
#define KHUI_CNFLAG_SUBPANEL      0x0002

/*! \brief Node represents a subpanel that is replicated for all child nodes

  If this flags is specified when registering a configuration node,
  khui_cfg_register() will automatically add the
  ::KHUI_CNFLAG_SUBPANEL flag.
*/
#define KHUI_CNFLAG_PLURAL        0x0004

/*! \brief System node

    \note For internal use by the NetIDMgr application.  Do not use.
*/
#define KHUI_CNFLAG_SYSTEM        0x0010

/*! \brief Settings have been modified

    Settings for this configuration panel have been modified.  This
    flag should be cleared once the settings have been successfully
    applied.
 */
#define KHUI_CNFLAG_MODIFIED      0x0100

/*! \brief Settings have been applied

    Set once any modified settings were successfully applied.
 */
#define KHUI_CNFLAG_APPLIED       0x0200

/*! \brief Flag mask for static flags

  These are the set of flags that do not change for the lifetime of a
  configuration node.  They are the only flags (and can only be
  specified) when calling khui_cfg_register().
 */
#define KHUI_CNFLAGMASK_STATIC    0x00ff

/*! \brief Flag mask for dynamic flags

  These are the set of flags that may change during the lifetime of a
  configuration node.  They are the only flags that can be specified
  when calling khui_cfg_set_flags() and khui_cfg_set_flags_inst().
 */
#define KHUI_CNFLAGMASK_DYNAMIC   0x0f00

/*! \brief Width of a configuration dialog in dialog units

    ::CFGDLG_WIDTH and ::CFGDLG_HEIGHT specify the dimensions of a
    configuration dialog width and height in dialog units.  The dialog
    will be created as a child of the configuration dialog and placed
    within it.
 */
#define CFGDLG_WIDTH 255

/*! \brief Height of a configuration dialog in dialog units 

    \see ::CFGDLG_WIDTH
*/
#define CFGDLG_HEIGHT 182

/*! \brief Width of a configuration tab dialog in dialog units

    ::CFGDLG_TAB_WIDTH and ::CFGDLG_TAB_HEIGHT specify the dimensions
    (in dialog units) of a dialog that will be placed within a tab
    control for dialogs where multiple display panels need to be
    shown.
 */
#define CFGDLG_TAB_WIDTH 235

/*! \brief Height of configuration tab dialog in dialog units

    \see ::CFGDLG_TAB_WIDTH
 */
#define CFGDLG_TAB_HEIGHT 151

/*! \brief A handle to a configuration node

    \see khui_cfg_open(), khui_cfg_release(), khui_cfg_hold()
*/
typedef khm_handle khui_config_node;

/*! \brief Initialization data passed in to a subpanel 

    When creating a subpanel, a pointer to the following strucutred
    will be passed in as the creation parameter for the dialog.
*/
typedef struct tag_khui_config_init_data {
    khui_config_node ctx_node;  /*!< The node under which the current
                                  dialog subpanel is being created. */

    khui_config_node this_node; /*!< The node which provided the
                                  registration information for the
                                  creation of the subpanel. */

    khui_config_node ref_node;  /*!< The parent node of the subpanel
                                  node.  In nodes which have the
                                  ::KHUI_CNFLAG_PLURAL, this would be
                                  different from the \a ctx_node. */
} khui_config_init_data;

/*! \brief Register a configuration node

    The caller fills the registration information in the
    ::khui_config_node_reg structre.  If the call succeeds, the
    function will return KHM_ERROR_SUCCESS.

    \param[in] parent Parent of the node to be registered.  Set to
        NULL if the parent is the root node.

    \param[in] reg Registration information

    \retval KHM_ERROR_SUCCESS Success
    \retval KHM_ERROR_INVALID_PARAM One or more parameters, or fields
        of reg were invalid
    \retval KHM_ERROR_DUPLICATE A node with the same name exists as a
        child of the specified parent node.

    \note For top level configuration nodes, the name (not the short
        or long description) of the node can not be the same as the
        name of a custom action.  See khui_action_create().
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_register(khui_config_node parent,
                  const khui_config_node_reg * reg);

/*!\brief Open a configuration node by name

    If successful, the \a result parameter will receive a handle to
    the configuration node.  Use khui_cfg_release() to release
    the handle.

    \param[in] parent Parent node.  Set to NULL to specify root node.
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_open(khui_config_node parent,
              const wchar_t * name,
              khui_config_node * result);

/*! \brief Remove a configuration node

    Marks a configuration node as deleted.  Once all the handles,
    including the handle specified in \a node have been released, it
    will be deleted.
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_remove(khui_config_node node);

/*! \brief Hold a handle to a configuration node

    Obtains an additional hold on the handle specified by \a node.
    The hold must be released with a call to \a
    khui_cfg_release()
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_hold(khui_config_node node);

/*! \brief Release a handle to a configuration node

    \see khui_cfg_hold()
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_release(khui_config_node node);

/*! \brief Get the parent of a node

    Returns a held handle to the parent of the node, or NULL if the
    current node is a top level node.  The returned handle must be
    released with khui_cfg_release().

    \retval KHM_ERROR_SUCCESS The handle to the parent node is in \a result
    \retval KHM_ERROR_NOT_FOUND The node is a top level node
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_get_parent(khui_config_node vnode,
                    khui_config_node * result);

/*! \brief Get a handle to the first child node

    If the call is successful, \a result will receieve a handle to the
    first child node of the specified node.  The returned handle must
    be released with a call to khui_cfg_release()

    If \a parent does not have any child nodes, the function will
    return KHM_ERROR_NOT_FOUND and set \a result to NULL.

    \param[in] parent Parent node.  Set to NULL to specify root node.
    \param[out] result Receives a held handle to the first child node.

    \see khui_cfg_get_next()
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_get_first_child(khui_config_node parent,
                         khui_config_node * result);

/*! \brief Get a handle to the first subpanel

    If the call is successful, \a result will receieve a handle to the
    first subpanel node of the specified node.  The returned handle
    must be released with a call to khui_cfg_release()

    If \a parent does not have any subpanels, the function will return
    KHM_ERROR_NOT_FOUND and set \a result to NULL.

    A subpanel node is a node which has the ::KHUI_CNFLAG_SUBPANEL
    flag set.

    \param[in] parent Parent node.  Set to NULL to specify root node.
    \param[out] result Receives a held handle to the first subpanel node.

    \see khui_cfg_get_next()
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_get_first_subpanel(khui_config_node vparent,
                            khui_config_node * result);

/*! \brief Get a handle to the next sibling node

    If the call is successful, \a result will receive a held handle to
    the next sibling node.  The returned handle must be released with
    a call to khui_cfg_release().

    If there are no more sibling nodes, then the function return
    KHM_ERROR_NOT_FOUND and set \a result to NULL.

    This function can be used to traverse a list of child nodes as
    well as a list of subpanel nodes.

 */
KHMEXP khm_int32 KHMAPI
khui_cfg_get_next(khui_config_node node,
                  khui_config_node * result);

/*! \brief Get a handle to the next sibling node

    Similar to khui_cfg_get_next(), but implicitly releases the handle
    that was supplied.  Equivalent to doing :

    \code
    khui_cfg_get_next(node, &next);
    khui_cfg_release(node);
    node = next;
    \endcode

    \param[in,out] node On entry, specifies the node whose sibling
        needs to be fetched.  On exit, will have either NULL or a held
        handle to the sibling node.  The handle which was supplied to
        the function is released.

    \retval KHM_ERROR_SUCCESS The next node is now in \a node
    \retval KHM_ERROR_INVALID_PARAM \a node was not a valid handle
    \retval KHM_ERROR_NOT_FOUND There are no more siblings.  \a node
        is set to NULL.

    \note Even if there are no more siblings, the handle specified in
        \a node on entry is released.
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_get_next_release(khui_config_node * node);

/*! \brief Get the name of a configuration node 

    Gets the name (not the short description or the long description)
    of the given configuration node.
*/
KHMEXP khm_int32 KHMAPI
khui_cfg_get_name(khui_config_node node,
                  wchar_t * buf,
                  khm_size * cb_buf);

/*! \brief Get registration information for a node

    The registration information that is returned is a shallow copy of
    the data kept by NetIDMgr.  In particular, the strings that will
    be returned actually point to internal buffers and should not be
    modified.

    No further action is necessary to release the information.
    However, the returned data ceases to be valid when \a node is
    released with a call to khui_cfg_release().

    \param[in] node Node for which information is requested.  Can be NULL if requesting information about the root node.
    \param[out] reg Pointer to a ::khui_config_node_reg structure.
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_get_reg(khui_config_node node,
                 khui_config_node_reg * reg);

/*! \brief Associate private data with a configuration node

    The data that is associated with the configuration node is
    entirely up to the caller.  It is recommended that private data
    only be set by those who registered the configuration node.

    If the node already had private data associated with it, the new
    data pointer will overwrite the previous pointer.  The application
    will not perform any cleanup for private data if the configuration
    node is removed.  It is the responsibility of the caller to do so.
 */
KHMEXP void KHMAPI
khui_cfg_set_data(khui_config_node node, void * pdata);

/*! \brief Retreive private data associated with a configuration node

    The private data pointer that is retrieved is the same as the
    pointer that was specified in the last successful call to
    khui_cfg_set_data() for the same configuration node.  If there is
    an error or if private data has not been associated with the node,
    the return value would be NULL.
 */
KHMEXP void * KHMAPI
khui_cfg_get_data(khui_config_node node);

#ifdef NIMPRIVATE

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP HWND KHMAPI
khui_cfg_get_hwnd_inst(khui_config_node node,
                       khui_config_node ctx_node);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP LPARAM KHMAPI
khui_cfg_get_param_inst(khui_config_node node,
                        khui_config_node ctx_node);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP void KHMAPI
khui_cfg_set_hwnd_inst(khui_config_node node, 
                       khui_config_node ctx_node,
                       HWND hwnd);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP void KHMAPI
khui_cfg_set_param_inst(khui_config_node node, 
                        khui_config_node ctx_node,
                        LPARAM param);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP HWND KHMAPI
khui_cfg_get_hwnd(khui_config_node node);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP LPARAM KHMAPI
khui_cfg_get_param(khui_config_node node);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP void KHMAPI
khui_cfg_set_hwnd(khui_config_node node, HWND hwnd);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP void KHMAPI
khui_cfg_set_param(khui_config_node node, LPARAM param);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP void KHMAPI
khui_cfg_clear_params(void);

/*! \brief Internal use

    This function is used internally by NetIDMgr.  Do not use.
*/
KHMEXP void KHMAPI
khui_cfg_set_configui_handle(HWND hwnd);

#endif  /* NIMPRIVATE */

/*! \brief Update the state for the specified node

    \param[in] node ::khui_config_node handle for the configuration node.

    \param[in] flags New flags.  Combination of ::KHUI_CNFLAG_APPLIED and ::KHUI_CNFLAG_MODIFIED

    \param[in] mask Valid bits in \a flags

    \note Should only be called from within the dialog procedure for
        the configuration node.
 */
KHMEXP void KHMAPI
khui_cfg_set_flags(khui_config_node vnode, khm_int32 flags, khm_int32 mask);

/*! \brief Retrieve the state flags for the configuration node

    \see khui_cfg_set_flags()
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_get_flags(khui_config_node vnode);

/*! \brief Utility function: Initialize dialog box window data

    This function initializes the dialog box window data using the
    ::khui_config_init_data that was passed into the WM_INITDIALOG
    message.

    A new block of memory will be alocated to store the dialog data as
    well as any extra space specified.  A pointer to this memory block
    will be stored in the \a DWLP_USER slot in the dialog box.

    The allocated block of memory must be freed by a call to
    khui_cfg_free_dialog_data().  While handling other messages, the
    dialog data can be retrieved using khui_cfg_get_dialog_data().

    \param[in] hwnd_dlg Handle to the dialog box

    \param[in] data Pointer to the ::khui_config_init_data that was
        passed in to WM_INITDIALOG (this is the value of \a lParam)

    \param[in] cb_extra Number of extra bytes to allocate, along with
        the space required to store the contents of
        ::khui_config_init_data.  The extra space will be initialized
        to zero.

    \param[out] new_data Receives a pointer to the copy of the
        initialization data that was allocated.  Optional.  Pass in
        NULL if this value is not required.

    \param[out] extra Receives a pointer to the block of extra memory
        allocated as specified in \a cb_extra.  If \a cb_extra is 0,
        then this receives a NULL.

    \see khui_cfg_get_dialog_data(), khui_cfg_free_dialog_data()
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_init_dialog_data(HWND hwnd_dlg,
                          const khui_config_init_data * data,
                          khm_size cb_extra,
                          khui_config_init_data ** new_data,
                          void ** extra);

/*! \brief Utility function: Retrieves dialog data 

    Retrieves the dialog data previoulsy stored using
    khui_cfg_init_dialog_data().

    \param[in] hwnd_dlg Handle to the dialog box 

    \param[out] data Receives a pointer to the ::khui_config_init_data
        block.
    
    \param[out] extra Receives a pointer to the extra memory
        allocated. Optional (set to NULL if this value is not needed).
*/
KHMEXP khm_int32 KHMAPI
khui_cfg_get_dialog_data(HWND hwnd_dlg,
                         khui_config_init_data ** data,
                         void ** extra);

/*! \brief Utility function: Free dialog data

    Deallocates the memory allcated in a previous call to
    khui_cfg_init_dialog_data()
 */
KHMEXP khm_int32 KHMAPI
khui_cfg_free_dialog_data(HWND hwnd_dlg);

/*! \brief Sets the instance flags for a subpanel

    Since there can be more than one subpanel in a configuration
    panel, they shouldn't modify the flags of the configuration node
    directly.  Instead, they should call this function to set the
    instance flags.

    The instance flags will be merged with the flags for the
    configuration node automatically.
 */
KHMEXP void KHMAPI
khui_cfg_set_flags_inst(khui_config_init_data * d,
                        khm_int32 flags,
                        khm_int32 mask);

#ifdef __cplusplus
}
#endif

/*!@} */
/*!@} */
#endif
