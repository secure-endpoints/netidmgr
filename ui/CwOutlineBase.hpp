#pragma once

namespace nim
{
    class CwOutline;
    class CwIdentityOutline;
    class CwCredTypeOutline;
    class CwGenericOutline;
    class CwCredentialRow;
    class CwTable;

    CwOutline * CreateOutlineNode(Identity& identity, DisplayColumnList& columns, int column);
    CwOutline * CreateOutlineNode(Credential& cred, DisplayColumnList& columns, int column);

    template < class T >
    bool AllowRecursiveInsert(DisplayColumnList& columns, int column);

    class CwOutlineElement :
        virtual public DisplayElement {};



    /*! \brief Base for all outline objects in the credentials window
     */
    class CwOutlineBase :
        public WithOutline< CwOutlineElement >,
        public TimerQueueClient {

    public:
        CwOutlineBase *insertion_point;
        int indent;

    public:
        CwOutlineBase() {
            insertion_point = NULL;
            indent = 0;
        }

        void BeginUpdate();

        void EndUpdate();

        template <class T>
        CwOutlineBase * Insert(T& obj, DisplayColumnList& columns, unsigned int column);

        virtual bool Represents(const Credential& credential) { return false; }
        virtual bool Represents(const Identity& identity) { return false; }
        virtual void SetContext(SelectionContext& sctx) = 0;

        template <class U, class V>
        bool FindElements(const V& criterion, std::vector<U *>& results);
    };
}
