/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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

#include "khmapp.h"
#include<assert.h>

#include "credwnd_base.hpp"

#include <sstream>

namespace nim
{
class CredentialListConstructor {
    Identity identity;
    CredentialSet credset;

    typedef std::map<khm_int32, int> StatMap;

    StatMap map;

public:
    CredentialListConstructor(Identity & _identity, CredentialSet & _credset) :
        identity(_identity), credset(_credset) {}

    static khm_int32 KHMCALLBACK CalculateStatCallback(Credential& c, void * param) {
        CredentialListConstructor * l = reinterpret_cast<CredentialListConstructor *>(param);

        if (c.GetIdentity() == l->identity) {
            khm_int32 t = (khm_int32) c.GetType();
            l->map[t]++;
        }

        return KHM_ERROR_SUCCESS;
    }

    std::wstring GetCaption() {
        std::wstringstream s;

        credset.Apply(CalculateStatCallback, this);

        for (StatMap::iterator i = map.begin(); i != map.end(); ++i) {
            CredentialType ct(i->first);

            if (i != map.begin())
                s << L"  ";

            s << ct.GetResourceString(KCDB_RES_INSTANCE, KCDB_RFS_SHORT) << L" (" << i->second << L")";
        }

        std::wstring ss = s.str();
        return ss;
    }
};

void CwIdentityCredentialTypesListElement::UpdateLayoutPost(Graphics& g, const Rect& layout) {
    CwTable * table = dynamic_cast<CwTable *>(owner);
    CredentialListConstructor lc(identity, table->credset);

    SetCaption(lc.GetCaption());

    __super::UpdateLayoutPost(g, layout);
}
}

