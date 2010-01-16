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

namespace nim {

    // Applies to DisplayContainer
    template<class T = DisplayContainer>
    class NOINITVTABLE WithNavigation : public T {
    protected:
        DisplayElement * el_focus;
        DisplayElement * el_anchor;

        DisplayElement * PrevElement(DisplayElement * c) {
            if (c == NULL) {
                if (TQLASTCHILD(this) == NULL)
                    return this;
                c = TQLASTCHILD(this);
                while (TQLASTCHILD(c))
                    c = TQLASTCHILD(c);
                return c;
            }
            if (TQPREVSIBLING(c)) {
                c = TQPREVSIBLING(c);
                while (TQLASTCHILD(c))
                    c = TQLASTCHILD(c);
                return c;
            } else
                return TQPARENT(c);
        }

        DisplayElement * PrevTabStop(DisplayElement * c) {
            do {
                c = PrevElement(c);
            } while (c != NULL && (!c->IsTabStop() || !c->IsVisible()));
            return c;
        }

        DisplayElement * NextElement(DisplayElement * c) {
            if (c == NULL)
                return this;
            if (TQFIRSTCHILD(c))
                return TQFIRSTCHILD(c);
            if (TQNEXTSIBLING(c))
                return TQNEXTSIBLING(c);
            if (TQPARENT(c)) {
                do {
                    c = TQPARENT(c);
                } while (c && TQNEXTSIBLING(c) == NULL);
                if (c)
                    return TQNEXTSIBLING(c);
            }
            return NULL;
        }

        DisplayElement * NextTabStop(DisplayElement * c) {
            do {
                c = NextElement(c);
            } while (c != NULL && (!c->IsTabStop() || !c->IsVisible()));
            return c;
        }

        static void SelectAllIn(DisplayElement * e, bool select) {
            e->Select(select);
            for (DisplayElement * c = TQFIRSTCHILD(e); c; c = TQNEXTSIBLING(c)) {
                SelectAllIn(c, select);
            }
        }

    public:
        WithNavigation() { el_focus = NULL; el_anchor = NULL; }
        ~WithNavigation() { }

        virtual void OnSetFocus(HWND hwnd_old) {
            if (el_focus)
                el_focus->Focus(true);
            __super::OnSetFocus(hwnd_old);
        }

        virtual void OnKillFocus(HWND hwnd_new) {
            if (el_focus)
                el_focus->Focus(false);
            __super::OnKillFocus(hwnd_new);
        }

	virtual void NotifyDeleteChild(DisplayElement * _parent, DisplayElement * e) {
            if (el_focus == e)
                el_focus = NULL;
            if (el_anchor == e)
                el_anchor = NULL;
            __super::NotifyDeleteChild(_parent, e);
        }

        virtual void NotifyDeleteAllChildren(DisplayElement * _parent) {
            el_focus = NULL;
            el_anchor = NULL;
            __super::NotifyDeleteAllChildren(_parent);
        }

        typedef enum SelectAction {
            SelectExclusive,
            SelectExtend,
            SelectAddToggle
        } SelectAction;

        void SetSelection(DisplayElement * e, SelectAction action) {
            DisplayElement * oldFocus = el_focus;

            if (e == NULL)
                return;

            switch (action) {
            case SelectAddToggle:
                e->Select(!e->selected);
                el_anchor = e;
                break;

            case SelectExclusive:
                SelectAllIn(this, false);
                e->Select(true);
                el_anchor = e;
                break;

            case SelectExtend:
                DisplayElement * c;
                for (c = e; c; c = NextTabStop(c)) {
                    if (c == el_anchor)
                        break;
                }
                if (c != NULL) {
                    SelectAllIn(this, false);
                    for (c = e; c; c = NextTabStop(c)) {
                        c->Select();
                        if (c == el_anchor)
                            break;
                    }
                    break;
                }
                for (c = e; c; c = PrevTabStop(c)) {
                    if (c == el_anchor)
                        break;
                }
                if (c != NULL) {
                    SelectAllIn(this, false);
                    for (c = e; c; c = PrevTabStop(c)) {
                        c->Select();
                        if (c == el_anchor)
                            break;
                    }
                    break;
                }
                e->Select();
                el_anchor = e;
                break;
            }

            el_focus = e;
            EnsureElementIsVisible(el_focus);

            if (oldFocus != el_focus) {
                if (oldFocus)
                    oldFocus->Focus(false);

                if (el_focus)
                    el_focus->Focus(true);
            }
        }

        void EnsureElementIsVisible(DisplayElement * e) {
            Rect focusRect(MapFromDescendant(e, Point(0,0)), e->extents);
            if (!scroll.Contains(focusRect)) {
                Rect oldScroll = scroll;

                if (focusRect.Y < scroll.Y)
                    scroll.Y = focusRect.Y;
                else if (scroll.GetBottom() < focusRect.GetBottom())
                    scroll.Y = focusRect.GetBottom() - scroll.Height;

                ValidateScrollPos();
                if (!oldScroll.Equals(scroll)) {
                    UpdateScrollBars(true);
                    ScrollBy(Point(scroll.X - oldScroll.X, scroll.Y - oldScroll.Y));
                    NotifyLayoutInternal();
                }
            }
        }

        virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify) {
            switch (id) {
            case KHUI_PACTION_LEFT:
                if (el_focus && el_focus->expandable && el_focus->expanded) {
                    el_focus->Expand(false);
                    Invalidate();
                    break;
                }
                // fallthrough

            case KHUI_PACTION_UP:
                SetSelection(PrevTabStop(el_focus), SelectExclusive); break;

            case KHUI_PACTION_UP_EXTEND:
                SetSelection(PrevTabStop(el_focus), SelectExtend); break;

            case KHUI_PACTION_UP_TOGGLE:
                SetSelection(PrevTabStop(el_focus), SelectAddToggle); break;

            case KHUI_PACTION_PGUP_EXTEND:
            case KHUI_PACTION_PGUP:
                if (el_focus) {
                    Rect r;
                    Point p = MapFromDescendant(el_focus, Point(0,el_focus->extents.Height));
                    p.Y -= GetClientRect(&r).Height;
                    DisplayElement * e = DescendantFromPoint(p);
                    if (e) e = PrevTabStop(e);
                    if (e) e = NextTabStop(e);
                    SetSelection((e)? e : NextTabStop(NULL),
                        (id == KHUI_PACTION_PGUP)? SelectExclusive : SelectExtend);
                } else {
                    SetSelection(NextTabStop(NULL), 
                        (id == KHUI_PACTION_PGUP)? SelectExclusive : SelectExtend);
                }
                break;

            case KHUI_PACTION_RIGHT:
                if (el_focus && el_focus->expandable && !el_focus->expanded) {
                    el_focus->Expand(true);
                    Invalidate();
                    break;
                }
                // fallthrough

            case KHUI_PACTION_DOWN:
                SetSelection(NextTabStop(el_focus), SelectExclusive); break;

            case KHUI_PACTION_DOWN_EXTEND:
                SetSelection(NextTabStop(el_focus), SelectExtend); break;

            case KHUI_PACTION_DOWN_TOGGLE:
                SetSelection(NextTabStop(el_focus), SelectAddToggle); break;

            case KHUI_PACTION_PGDN_EXTEND:
            case KHUI_PACTION_PGDN:
                if (el_focus) {
                    Rect r;
                    Point p = MapFromDescendant(el_focus, Point(0,0));
                    p.Y += GetClientRect(&r).Height;
                    DisplayElement * e = DescendantFromPoint(p);
                    if (e) e = PrevTabStop(e);
                    if (e) e = NextTabStop(e);
                    SetSelection((e)? e : PrevTabStop(NULL),
                        (id == KHUI_PACTION_PGDN)? SelectExclusive : SelectExtend);
                } else {
                    SetSelection(PrevTabStop(NULL), 
                        (id == KHUI_PACTION_PGDN)? SelectExclusive : SelectExtend);
                }
                break;

            case KHUI_PACTION_SELALL:
                SelectAllIn(this, true);
                break;
            }

            T::OnCommand(id, hwndCtl, codeNotify);
        }

        virtual void OnChildClick(DisplayElement * e, const Point& p, UINT keyflags, bool doubleClick) {
            if (!e->IsTabStop()) return;

            if (doubleClick) {
                e->Activate();
                return;
            }

            if ((keyflags & MK_SHIFT) == MK_SHIFT) {
                SetSelection(e, SelectExtend);
            } else if ((keyflags & MK_CONTROL) == MK_CONTROL) {
                SetSelection(e, SelectAddToggle);
            } else {
                SetSelection(e, SelectExclusive);
            }
            T::OnChildClick(e, p, keyflags, doubleClick);
        }
    };
}
