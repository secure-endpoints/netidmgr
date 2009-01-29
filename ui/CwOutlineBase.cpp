#include "khmapp.h"
#include<assert.h>

#include "credwnd_base.hpp"

namespace nim
{

    template <class T>
    bool AllowRecursiveInsert(DisplayColumnList& columns, int column)
    {
        return false;
    }

    template <>
    bool AllowRecursiveInsert<Credential>(DisplayColumnList& columns, int column)
    {
        return (column >= 0 && (unsigned) column < columns.size() && columns[column]->group);
    }

    template <class T>
    CwOutlineBase * CwOutlineBase::Insert(T& obj, DisplayColumnList& columns, unsigned int column) 
    {
        CwOutlineBase * target = NULL;
        bool insert_new = false;

        for_each_child(c) {
            if (c == insertion_point)
                insert_new = true;

            if (c->Represents(obj)) {
                if (insert_new) {
                    if (c == insertion_point) {
                        insertion_point = dynamic_cast<CwOutlineBase*>(TQNEXTSIBLING(insertion_point));
                    } else {
                        MoveChildBefore(c, insertion_point);
                    }
                }
                target = c;
                break;
            }
        }

        if (target == NULL) {
            target = CreateOutlineNode(obj, columns, column);
            InsertOutlineBefore(target, insertion_point);
        }

        if (AllowRecursiveInsert<T>(columns, column)) {
            target->Insert(obj, columns, ++column);
        }

        return target;
    }

    // Explicit instantiation
    template
    CwOutlineBase * CwOutlineBase::Insert<Identity>(Identity&, DisplayColumnList&, unsigned int);

    template
    CwOutlineBase * CwOutlineBase::Insert<Credential>(Credential&, DisplayColumnList&, unsigned int);

    void CwOutlineBase::BeginUpdate() 
    {
        insertion_point = NextOutline(TQFIRSTCHILD(this));
        CancelTimer();

        for_each_child(c)
            c->BeginUpdate();
    }

    void CwOutlineBase::EndUpdate()
    {
        CwOutlineBase * c;

        for (insertion_point = NextOutline(insertion_point);
             insertion_point;) {
            c = insertion_point;
            insertion_point = NextOutline(TQNEXTSIBLING(c));
            DeleteChild(c);
        }

        for_each_child(c)
            c->EndUpdate();
    }

    template <class elementT, class queryT>
    bool CwOutlineBase::FindElements(const queryT& criterion, std::vector<elementT *>& results) {
        elementT * addend;
        if (Represents(criterion) &&
            (addend = dynamic_cast<elementT *>(this)) != NULL) {
            results.push_back(addend);
        }

        for_each_child(c) {
            c->FindElements(criterion, results);
        }

        return results.size() > 0;
    }

    // Explicit instantiation
    template
    bool CwOutlineBase::FindElements<CwIdentityOutline, Identity>(const Identity&, std::vector<CwIdentityOutline *>&);
}
