#pragma once

namespace nim {

    // Applies to DisplayElement
    template <class T = DisplayElement>
    class NOINITVTABLE WithHostedControl : public T {
    protected:
        HWND hwnd_hosted;

    public:
        WithHostedControl() : hwnd_hosted(NULL) { }

        ~WithHostedControl() {
            if (hwnd_hosted) {
                DestroyWindow(hwnd_hosted);
            }
        }

        void SetWindow(HWND _hwnd) {
            hwnd_hosted = _hwnd;
        }

        HWND GetWindow() {
            return hwnd_hosted;
        }

        virtual void NotifyLayout(void) {
            Point p_client;

            assert(owner);

            if (hwnd_hosted && owner) {
                p_client = owner->VirtualToClient(owner->MapFromDescendant(this, Point(0,0)));

                SetWindowPos(hwnd_hosted, NULL, p_client.X, p_client.Y,
                             0, 0, SWP_MOVEONLY);
            }
        }

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            if (hwnd_hosted) {
                RECT r;

                if (visible) {

                    ::GetClientRect(hwnd_hosted, &r);

                    extents.Width = r.right - r.left;
                    extents.Height = r.bottom - r.top;

                } else {

                    extents.Width = 0;
                    extents.Height = 0;

                    SetWindowPos(hwnd_hosted, NULL, 0, 0, 0, 0, SWP_HIDEONLY);
                }

            } else {

                extents.Width = 0;
                extents.Height = 0;

            }
        }
    };
}
