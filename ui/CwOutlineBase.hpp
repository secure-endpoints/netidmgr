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
