#define Uses_TApplication
#define Uses_TDialog
#define Uses_TDeskTop
#define Uses_TButton
#define Uses_TFrame
#define Uses_TScrollBar
#define Uses_TListViewer
#include <tvision/tv.h>
#include <turbo/turbo.h>
#include <tpath.h>

#include <memory>
#include <utility>
#include <forward_list>
#include <vector>

enum : ushort
{
    cmToggleLineNumbers = 1000,
    cmToggleLineWrapping,
    cmEditorSelected,
    cmNewFile,
    cmOpenFile,
    cmSaveFile,
};

struct DemoEditorListView;
struct DemoEditorState;
using EditorStateList = std::forward_list<DemoEditorState>;

struct DemoApplication : public TApplication
{

    DemoApplication();

};

struct DemoEditorWindow : public TDialog
{

    enum { listWidth = 20 };

    EditorStateList states;
    turbo::EditorView *edView;
    turbo::LeftMarginView *leftMargin;
    TScrollBar *hScrollBar, *vScrollBar;
    DemoEditorListView *listView;
    std::vector<char> title;

    DemoEditorWindow(const TRect &bounds);

    void shutDown() override;
    void handleEvent(TEvent &ev) override;
    void dragView(TEvent& event, uchar mode, TRect& limits, TPoint minSize, TPoint maxSize) override;
    const char *getTitle(short) override;

    void addEditor(turbo::Editor &, TStringView filePath);

};

struct DemoEditorState : public turbo::FileEditorState
{

    using super = turbo::FileEditorState;
    using super::FileEditorState;

    void drawViews() override;
    void afterSave() override;

};

struct DemoEditorListView : public TListViewer
{

    DemoEditorListView( const TRect& bounds, TScrollBar *aHScrollBar,
                     TScrollBar *aVScrollBar, EditorStateList &aList );

    EditorStateList &list;

    void getText(char *dest, short item, short maxLen) override;
    void focusItem(short item) override;
    void setRange(short aRange);

    int maxWidth();

};

DemoApplication::DemoApplication() :
    TProgInit(&initStatusLine,
              &initMenuBar,
              &initDeskTop)
{
    insertWindow(
        new DemoEditorWindow(
            deskTop->getExtent().grow(-2, -2)
        )
    );
}

DemoEditorWindow::DemoEditorWindow(const TRect &bounds) :
    TWindowInit(&initFrame),
    TDialog(bounds, nullptr)
{
    using namespace turbo;
    flags |= wfGrow;

    TRect viewBounds = getExtent().grow(-2, -4).move(0, -2);
    viewBounds.b.x -= listWidth + 1;

    edView = new EditorView(viewBounds);
    leftMargin = new LeftMarginView(1);
    hScrollBar = [&] {
        TRect r = viewBounds;
        r.a.y = r.b.y;
        r.b.y += 1;
        return new TScrollBar(r);
    }();
    vScrollBar = [&] {
        TRect r = viewBounds;
        r.a.x = r.b.x;
        r.b.x += 1;
        return new TScrollBar(r);
    }();
    TRect butBounds;
    {
        TStringView text = "Line Numbers";
        butBounds = TRect(0, 0, cstrlen(text) + 4, 2).move(viewBounds.a.x, viewBounds.b.y + 2);
        auto *but = new TButton(butBounds, text, cmToggleLineNumbers, bfNormal);
        but->growMode = gfGrowLoY | gfGrowHiY;
        insert(but);
    }
    {
        TStringView text = "Line Wrapping";
        butBounds = TRect(0, 0, cstrlen(text) + 4, 2).move(butBounds.b.x + 1, butBounds.a.y);
        auto *but = new TButton(butBounds, text, cmToggleLineWrapping, bfNormal);
        but->growMode = gfGrowLoY | gfGrowHiY;
        insert(but);
    }
    {
        TStringView text = "New File";
        butBounds = TRect(0, 0, cstrlen(text) + 4, 2).move(butBounds.b.x + 1, butBounds.a.y);
        auto *but = new TButton(butBounds, text, cmNewFile, bfNormal);
        but->growMode = gfGrowLoY | gfGrowHiY;
        insert(but);
    }
    {
        TStringView text = "Open File";
        butBounds = TRect(0, 0, cstrlen(text) + 4, 2).move(butBounds.b.x + 1, butBounds.a.y);
        auto *but = new TButton(butBounds, text, cmOpenFile, bfNormal);
        but->growMode = gfGrowLoY | gfGrowHiY;
        insert(but);
    }
    {
        TStringView text = "Save File";
        butBounds = TRect(0, 0, cstrlen(text) + 4, 2).move(butBounds.b.x + 1, butBounds.a.y);
        auto *but = new TButton(butBounds, text, cmSaveFile, bfNormal);
        but->growMode = gfGrowLoY | gfGrowHiY;
        insert(but);
    }
    listView = [&] {
        TRect r = viewBounds;
        r.a.x = r.b.x + 1;
        r.b.x += listWidth - 1;
        r.move(1, 0);
        auto *hScrollBar = [&] {
            TRect s = r;
            s.a.y = s.b.y;
            s.b.y += 1;
            return new TScrollBar(s);
        }();
        auto *vScrollBar = [&] {
            TRect s = r;
            s.a.x = s.b.x;
            s.b.x += 1;
            return new TScrollBar(s);
        }();
        hScrollBar->growMode |= gfGrowLoX;
        insert(hScrollBar);
        insert(vScrollBar);
        return new DemoEditorListView(r, hScrollBar, vScrollBar, states);
    }();
    listView->growMode = gfGrowLoX | gfGrowHiX | gfGrowHiY;
    // The editor view needs to go first so that, when resizing the window, the other
    // views get resized before it, which takes care of redrawing them all.
    insert(edView);
    insert(leftMargin);
    insert(hScrollBar);
    insert(vScrollBar);
    insert(listView);
}

void DemoEditorWindow::shutDown()
{
    if (edView && edView->state)
        edView->state->disassociate();
    edView = nullptr;
    leftMargin = nullptr;
    hScrollBar = nullptr;
    vScrollBar = nullptr;
    listView = nullptr;
    TDialog::shutDown();
}

void DemoEditorWindow::handleEvent(TEvent &ev)
{
    using namespace turbo::constants;
    if (ev.what == evCommand && edView)
    {
        switch (ev.message.command)
        {
            case cmToggleLineNumbers:
                if (edView->state)
                {
                    edView->state->toggleLineNumbers();
                    edView->state->redraw();
                    clearEvent(ev);
                }
                break;
            case cmToggleLineWrapping:
                if (edView->state)
                {
                    edView->state->toggleLineWrapping(lwConfirm);
                    edView->state->redraw();
                    clearEvent(ev);
                }
                break;
            case cmEditorSelected:
            {
                auto &state = *(DemoEditorState *) ev.message.infoPtr;
                state.associate(edView, leftMargin, hScrollBar, vScrollBar);
                state.redraw();
                clearEvent(ev);
                break;
            }
            case cmNewFile:
                addEditor(turbo::createEditor(), "");
                break;
            case cmOpenFile:
            {
                auto &&r = turbo::openFileWithDialog();
                if (r.editor)
                    addEditor(*r.editor, r.filePath);
                break;
            }
            case cmSaveFile:
                if (edView->state)
                {
                    auto &state = *(DemoEditorState *) edView->state;
                    state.save();
                }
                break;
        }
    }
    TDialog::handleEvent(ev);
}

void DemoEditorWindow::dragView(TEvent& event, uchar mode, TRect& limits, TPoint minSize, TPoint maxSize)
{
    if (edView && edView->state)
    {
        auto lastResizeLock = edView->state->resizeLock;
        auto lastSize = edView->size;
        edView->state->resizeLock = true;
        TDialog::dragView(event, mode, limits, minSize, maxSize);
        edView->state->resizeLock = lastResizeLock;
        if (lastSize != edView->size)
            edView->state->redraw(); // Redraw without 'resizeLock = true'.
    }
    else
        TDialog::dragView(event, mode, limits, minSize, maxSize);
}

const char *DemoEditorWindow::getTitle(short)
{
    if (edView && edView->state)
    {
        auto &state = *(turbo::FileEditorState *) edView->state;
        auto name = TPath::basename(state.filePath);
        if (name.empty()) name = "Untitled";
        bool dirty = !state.inSavePoint();
        size_t length = name.size() + dirty;
        title.resize(0);
        title.resize(length + 1);
        memcpy(title.data(), name.data(), name.size());
        if (dirty) title[name.size()] = '*';
        title[length] = '\0';
        return title.data();
    }
    return nullptr;
}

void DemoEditorWindow::addEditor(turbo::Editor &editor, TStringView filePath)
{
    states.emplace_front(editor, filePath);
    listView->setRange(listView->range + 1);
    listView->focusItemNum(0); // Triggers EditorState::associate.
    listView->drawView();
}

void DemoEditorState::drawViews()
{
    if (view && view->owner)
    {
        auto &window = *(DemoEditorWindow *) view->owner;
        window.lock();
        super::drawViews();
        if (!resizeLock) // No need to draw the frame twice while resizing.
            window.frame->drawView();
        window.unlock();
    }
}

void DemoEditorState::afterSave()
{
    super::afterSave();
    if (view)
    {
        if (view->state)
            view->state->redraw();
        if (view->owner)
            ((DemoEditorWindow *) view->owner)->listView->drawView();
    }
}

DemoEditorListView::DemoEditorListView( const TRect& bounds, TScrollBar *aHScrollBar,
                                TScrollBar *aVScrollBar, EditorStateList &aList ) :
    TListViewer(bounds, 1, aHScrollBar, aVScrollBar),
    list(aList)
{
}

void DemoEditorListView::getText(char *dest, short item, short maxLen)
{
    short i = 0;
    for (auto &state : list)
        if (i++ == item)
        {
            if (!state.filePath.empty())
                strnzcpy(dest, state.filePath, maxLen);
            else
                strnzcpy(dest, "Untitled", maxLen);
            return;
        }
    snprintf(dest, maxLen, "<ERROR: out-of-bounds index %hd>", item);
}

void DemoEditorListView::focusItem(short item)
{
    TListViewer::focusItem(item);
    short i = 0;
    for (auto &state : list)
        if (i++ == item)
        {
            message(owner, evCommand, cmEditorSelected, &state);
            break;
        }
}

void DemoEditorListView::setRange(short aRange)
{
    TListViewer::setRange(aRange);
    if (hScrollBar != 0)
        hScrollBar->setRange(0, maxWidth() - size.x + 2);
}

int DemoEditorListView::maxWidth()
{
    char text[256];
    int width = 0;
    for (short i = 0; i < range; ++i)
    {
        getText(text, i, sizeof(text) - 1);
        width = max(strwidth(text), width);
    }
    return width;
}

int main()
{
    DemoApplication app;
    app.run();
    app.shutDown();
}
