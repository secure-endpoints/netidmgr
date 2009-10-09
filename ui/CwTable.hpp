/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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

#pragma once

namespace nim
{
#define DCL_KMSG(T,ST)                                                  \
    khm_int32 OnKmsg ## T ## _ ## ST (khm_ui_4 uparam, void * vparam)
#define DEFINE_KMSG(T,ST)                                               \
    khm_int32 CwTable::OnKmsg ## T ## _ ## ST (khm_ui_4 uparam, void * vparam)

#define HANDLE_KMSG(T,ST)                                               \
    if (msg_type == KMSG_ ## T && msg_subtype == KMSG_ ## T ## _ ## ST) \
        return OnKmsg ## T ## _ ## ST (uparam, vparam)


#pragma warning(push)
    // C4250 is 'class A' : inherits 'A::method()' via dominance

    // It gets triggered a lot because of virtual overrides in
    // DisplayContainer and CwOutlineBase.  This is expected.
#pragma warning(disable: 4250)




    /*! \brief Credential Window
     */
    class CwTable : public WithVerticalLayout< WithNavigation< WithTooltips< DisplayContainer > > >,
                    public CwOutlineBase,
                    public TimerQueueHost
    {
    public:
        // Credentials selection
        Identity   filter_by_identity; // Only if we are filtering by identity

        ConfigSpace cs_view;

        CredentialSet credset;

        // Flags
        bool   is_identity_view: 1; // Is this a compact identity view?
        bool   is_primary_view : 1; // Is this the primary credentials view?
        bool   is_custom_view  : 1; // Has this view been customized?
        bool   view_all_idents : 1; // View all identities? (only valid if is_identity_view)
        bool   columns_dirty   : 1; // Have the columns changed?
        bool   skipped_columns : 1; // Have any columns been skipped?

        DWORD  ticks_UpdateOutline; // GetTickCount() for the last UpdateOutline() call
        DWORD  ticks_UpdateCredentials; // GetTickCount() for the last UpdateCredentials() call

    public:
        CwTable() {
            is_identity_view = false;
            is_primary_view = false;
            is_custom_view = false;
            view_all_idents = false;
            columns_dirty = false;
            skipped_columns = false;
            ticks_UpdateOutline = 0;
            ticks_UpdateCredentials = 0;
        }

        ~CwTable() {}

        void SaveView();
        void LoadView(const wchar_t * view_name = NULL);
        void UpdateCredentials();
        void UpdateOutline();

        int  InsertDerivedIdentities(CwOutlineBase * o, Identity * id);
        void ToggleColumnByAttributeId(khm_int32 attr_id);

    public:
        typedef struct CwCreateParams {
            bool is_primary_view;
        };

        // createParams is a pointer to CwCreateParams
        virtual BOOL OnCreate(LPVOID createParams);

    public:                     // Overrides
        virtual void OnDestroy();
        virtual void OnColumnPosChanged(int from, int to);
        virtual void OnColumnSizeChanged(int idx);
        virtual void OnColumnSortChanged(int idx);
        virtual void OnColumnContextMenu(int idx, const Point& p);
        virtual khm_int32 OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype,
                                       khm_ui_4 uparam, void * vparam);
        virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
        virtual void OnContextMenu(const Point& p);

        virtual DWORD GetStyle();
        virtual DWORD GetStyleEx();

        virtual HFONT GetHFONT();

        virtual void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip);

        virtual void SetContext(SelectionContext& sctx);

    public:                     // Handlers for KMSG_*
        DCL_KMSG(CRED, ROOTDELTA);
        DCL_KMSG(CRED, PP_BEGIN);
        DCL_KMSG(CRED, PP_PRECREATE);
        DCL_KMSG(CRED, PP_END);
        DCL_KMSG(CRED, PP_DESTROY);
        DCL_KMSG(KCDB, IDENT);
        DCL_KMSG(KCDB, ATTRIB);
        DCL_KMSG(KMM, I_DONE);
        DCL_KMSG(ACT, ACTIVATE);
        DCL_KMSG(CREDP, BEGIN_NEWCRED);
        DCL_KMSG(CREDP, END_NEWCRED);
        DCL_KMSG(CREDP, PROG_NEWCRED);

    public:
        static khm_int32 attrib_to_action[KCDB_ATTR_MAX_ID + 1];
        static void RefreshGlobalColumnMenu(HWND hwnd);
    };

#pragma warning(pop)
}
