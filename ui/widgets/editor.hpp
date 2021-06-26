/**
  *  \file ui/widgets/editor.hpp
  *  \brief Class ui::widgets::Editor
  */
#ifndef C2NG_UI_WIDGETS_EDITOR_HPP
#define C2NG_UI_WIDGETS_EDITOR_HPP

#include "afl/base/signalconnection.hpp"
#include "ui/root.hpp"
#include "ui/simplewidget.hpp"
#include "util/editor/editor.hpp"

namespace ui { namespace widgets {

    /** Editor widget.
        Allows control of a multi-line util::editor::Editor. */
    class Editor : public SimpleWidget {
     public:
        /** Constructor.
            \param ed   Editor. Must outlive this widget.
            \param root UI root (for color scheme, font) */
        Editor(util::editor::Editor& ed, Root& root);
        ~Editor();

        /** Set preferred size in pixels.
            \param size Size */
        void setPreferredSize(gfx::Point size);

        /** Set preferred size in font cells.
            \param columns Number of columns
            \param lines   Number of lines */
        void setPreferredSizeInCells(size_t columns, size_t lines);

        /** Set first column to show (scroll horizontally).
            \param fc First column (0-based) */
        void setFirstColumn(size_t fc);

        /** Set first line to show (scroll vertically).
            \param fl First line (0-based) */
        void setFirstLine(size_t fl);

        /** Toggle whether scrolling is allowed.
            Note that disabling this allows the user to move the cursor out of view.
            \param flag Flag */
        void setAllowScrolling(bool flag);

        /** Set editor flag.
            Use to toggle the Overwrite, WordWrap, AllowCursorAfterEnd flags.
            \param flag   Flag
            \param enable New value */
        void setFlag(util::editor::Flag flag, bool enable);

        /** Toggle editor flag.
            Use to toggle the Overwrite, WordWrap, AllowCursorAfterEnd flags.
            \param flag   Flag */
        void toggleFlag(util::editor::Flag flag);

        // Widget:
        virtual void draw(gfx::Canvas& can);
        virtual void handleStateChange(State st, bool enable);
        virtual void handlePositionChange(gfx::Rectangle& oldPosition);
        virtual ui::layout::Info getLayoutInfo() const;
        virtual bool handleKey(util::Key_t key, int prefix);
        virtual bool handleMouse(gfx::Point pt, MouseButtons_t pressedButtons);

     private:
        void onEditorChange(size_t firstLine, size_t lastLine);
        afl::base::Ref<gfx::Font> getFont();

        util::editor::Editor& m_editor;
        util::editor::Flags_t m_editorFlags;
        gfx::Point m_preferredSize;
        Root& m_root;
        size_t m_firstColumn;
        size_t m_firstLine;
        bool m_allowScrolling;

        afl::base::SignalConnection conn_editorChange;
    };

} }

#endif
