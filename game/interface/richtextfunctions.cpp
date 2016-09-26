/**
  *  \file game/interface/richtextfunctions.cpp
  */

#include <climits>
#include "game/interface/richtextfunctions.hpp"
#include "interpreter/values.hpp"
#include "util/rich/colorattribute.hpp"
#include "util/skincolor.hpp"
#include "util/rich/styleattribute.hpp"
#include "util/rich/linkattribute.hpp"
#include "afl/io/constmemorystream.hpp"
#include "afl/io/xml/reader.hpp"
#include "afl/io/xml/entityhandler.hpp"
#include "util/rich/parser.hpp"
#include "afl/string/parse.hpp"
#include "afl/io/xml/defaultentityhandler.hpp"
#include "afl/charset/utf8.hpp"

using interpreter::checkIntegerArg;
using interpreter::checkStringArg;
using interpreter::makeIntegerValue;
using interpreter::makeStringValue;
typedef game::interface::RichTextValue::Ptr_t Ptr_t;

namespace {
    enum StyleKind {
        kNone,
        kStyle,
        kColor
    };
    struct Style {
        const char* name;
        StyleKind   kind : 8;
        uint8_t     value;
    };
    const Style styles[] = {
        { "",           kNone,  0 },
        { "b",          kStyle, util::rich::StyleAttribute::Bold }, /* tag name */
        //{ "background", kColor, util::SkinColor::tc_Background },
        { "big",        kStyle, util::rich::StyleAttribute::Big },
        { "blue",       kColor, util::SkinColor::Blue },
        { "bold",       kStyle, util::rich::StyleAttribute::Bold }, /* real name */
        //{ "contrast",   kColor, util::SkinColor::tc_Contrast },
        { "dim",        kColor, util::SkinColor::Faded },
        { "em",         kStyle, util::rich::StyleAttribute::Bold }, /* tag name (should actually be italic) */
        { "fixed",      kStyle, util::rich::StyleAttribute::Fixed }, /* real name */
        { "green",      kColor, util::SkinColor::Green },
        //{ "heading",    kColor, util::SkinColor::tc_Heading },
        //{ "input",      kColor, util::SkinColor::tc_Input },
        //{ "invstatic",  kColor, util::SkinColor::tc_InvStatic },
        //{ "italic",     kStyle, util::rich::StyleAttribute::Italic }, /* not supported yet */
        { "kbd",        kStyle, util::rich::StyleAttribute::Key }, /* tag name */
        { "key",        kStyle, util::rich::StyleAttribute::Key }, /* real name */
        //{ "link",       kColor, util::SkinColor::tc_Link },
        //{ "linkfocus",  kColor, util::SkinColor::tc_LinkFocus },
        //{ "linkshade",  kColor, util::SkinColor::tc_LinkShade },
        { "none",       kNone,  0 },
        { "red",        kColor, util::SkinColor::Red },
        //{ "selection",  kColor, util::SkinColor::tc_Selection },
        { "small"    ,  kStyle, util::rich::StyleAttribute::Small },
        { "static",     kColor, util::SkinColor::Static },
        { "tt",         kStyle, util::rich::StyleAttribute::Fixed }, /* tag name */
        { "u",          kStyle, util::rich::StyleAttribute::Underline }, /* tag name */
        { "underline",  kStyle, util::rich::StyleAttribute::Underline }, /* real name */
        { "white",      kColor, util::SkinColor::White },
        { "yellow",     kColor, util::SkinColor::Yellow },
    };

    Ptr_t processStyle(String_t style, Ptr_t text)
    {
        style = afl::string::strTrim(afl::string::strLCase(style));

        const Style* p = 0;
        afl::base::Memory<const Style> pp(styles);
        while (const Style* q = pp.eat()) {
            if (q->name == style) {
                p = q;
                break;
            }
        }

        if (!p) {
            throw interpreter::Error("Invalid style");
        }

        switch (p->kind) {
         case kNone:
            return text;
         case kColor:
         {
             Ptr_t tmp = new util::rich::Text(*text);
             tmp->withNewAttribute(new util::rich::ColorAttribute(util::SkinColor::Color(p->value)));
             return tmp;
         }
         case kStyle:
         {
             Ptr_t tmp = new util::rich::Text(*text);
             tmp->withNewAttribute(new util::rich::StyleAttribute(util::rich::StyleAttribute::Style(p->value)));
             return tmp;
         }
        }

        // Fallback, does not happen
        return text;
    }
}

bool
game::interface::checkRichArg(RichTextValue::Ptr_t& out, afl::data::Value* value)
{
    // ex int/if/richif.h:checkRichArg
    if (value == 0) {
        return false;
    } else {
        if (RichTextValue* rv = dynamic_cast<RichTextValue*>(value)) {
            out = rv->get();
        } else {
            out = new util::rich::Text(interpreter::toString(value, false));
        }
        return true;
    }
}

// /* @q RAdd(args:RichText...):RichText (Function)
//    Concatenate all arguments, which can be strings or rich text, to a new rich text string,
//    and returns that.

//    If any argument is EMPTY, returns EMPTY.
//    If no arguments are given, returns an empty (=zero length) rich text string.

//    In text mode, this function produces plain strings instead,
//    as rich text attributes have no meaning to the text mode applications.

//    @since PCC2 1.99.21 */
afl::data::Value*
game::interface::IFRAdd(game::Session& /*session*/, interpreter::Arguments& args)
{
    // ex int/if/richif.h:IFRAdd
    if (args.getNumArgs() == 1) {
        // Special case: act as cast-to-rich-text, avoiding a copy
        Ptr_t result;
        if (checkRichArg(result, args.getNext())) {
            return new RichTextValue(result);
        } else {
            return 0;
        }
    } else {
        // General case
        Ptr_t result = new util::rich::Text();
        while (args.getNumArgs() > 0) {
            Ptr_t tmp;
            if (!checkRichArg(tmp, args.getNext())) {
                return 0;
            }
            *result += *tmp;
        }
        return new RichTextValue(result);
    }
}

// /* @q RMid(str:RichText, first:Int, Optional length:Int):RichText (Function)
//    Returns a substring of a rich text string.

//    %first specifies the first character position to extract, where 1 means the first.
//    %length specifies the number of characters to extract.
//    If %length is omitted or EMPTY, the remaining string is extracted.

//    If %str or %first are EMPTY, returns EMPTY.

//    In text mode, this function deals with plain strings instead,
//    as rich text attributes have no meaning to the text mode applications.

//    @since PCC2 1.99.21 */
afl::data::Value*
game::interface::IFRMid(game::Session& /*session*/, interpreter::Arguments& args)
{
    // ex int/if/richif.h:IFRMid
    args.checkArgumentCount(2, 3);

    // Parse args
    Ptr_t str;
    int32_t iStart, iLength;
    if (!checkRichArg(str, args.getNext()) || !checkIntegerArg(iStart, args.getNext(), 0, INT_MAX)) {
        return 0;
    }
    if (!checkIntegerArg(iLength, args.getNext(), 0, INT_MAX)) {
        iLength = INT_MAX;
    }

    // Convert BASIC indexes to C++ indexes
    // FIXME: this needs some optimisation
    const String_t& text = str->getText();
    afl::charset::Utf8 u8(0);
    size_t nStart  = u8.charToBytePos(text, iStart == 0 ? 0 : static_cast<size_t>(iStart) - 1);
    size_t nLength = u8.charToBytePos(text.substr(nStart), iLength);
    if (nStart > str->size()) {
        return new RichTextValue(new util::rich::Text());
    } else {
        return new RichTextValue(new util::rich::Text(str->substr(nStart, nLength)));
    }
}

// /* @q RString(str:RichText):Str (Function)
//    Returns the text content of a rich text string,
//    i.e. the string with all attributes removed.

//    If %str is EMPTY, returns EMPTY.

//    In text mode, this function deals with plain strings instead,
//    as rich text attributes have no meaning to the text mode applications.

//    @since PCC2 1.99.21 */
afl::data::Value*
game::interface::IFRString(game::Session& /*session*/, interpreter::Arguments& args)
{
    // ex int/if/richif.h:IFRString
    args.checkArgumentCount(1);
    Ptr_t str;
    if (checkRichArg(str, args.getNext())) {
        return makeStringValue(str->getText());
    } else {
        return 0;
    }
}

// /* @q RLen(str:RichText):Int (Function)
//    Returns the number of characters in a rich text string.

//    If %str is EMPTY, returns EMPTY.

//    In text mode, this function deals with plain strings instead,
//    as rich text attributes have no meaning to the text mode applications.

//    @since PCC2 1.99.21 */
afl::data::Value*
game::interface::IFRLen(game::Session& /*session*/, interpreter::Arguments& args)
{
    // ex int/if/richif.h:IFRLen
    args.checkArgumentCount(1);
    Ptr_t str;
    if (checkRichArg(str, args.getNext())) {
        // FIXME: use character numbers, not bytes!
        return makeIntegerValue(str->size());
    } else {
        return 0;
    }

}

// /* @q RStyle(style:Str, content:RichText...):RichText (Function)
//    Attaches a new style to a rich text string.
//    Concatenates all %content parameters, and returns a new rich text string with the specified attribute added.

//    <pre class="ccscript">
//      RStyle("red", "This is ", RStyle("bold", "great"))
//    </pre>
//    produces "<font color="red">This is <b>great</b></font>".

//    If any argument is EMPTY, returns EMPTY.

//    In text mode, this function just returns the concatenation of the %content,
//    as rich text attributes have no meaning to the text mode applications.

//    @todo document the styles
//    @since PCC2 1.99.21
//    @see RLink, RXml */
afl::data::Value*
game::interface::IFRStyle(game::Session& session, interpreter::Arguments& args)
{
    // ex int/if/richif.h:IFRStyle
    // Read style argument
    args.checkArgumentCountAtLeast(1);
    String_t style;
    if (!checkStringArg(style, args.getNext())) {
        return 0;
    }

    // Read remaining arguments, converting them to Rich Text. This is just what IFRAdd does.
    std::auto_ptr<afl::data::Value> tmp(IFRAdd(session, args));
    Ptr_t result;
    if (!checkRichArg(result, tmp.get())) {
        return 0;
    }

    // Process the style
    size_t pos = 0, i;
    while ((i = style.find(',', pos)) != String_t::npos) {
        result = processStyle(style.substr(pos, i-pos), result);
        pos = i+1;
    }
    return new RichTextValue(processStyle(style.substr(pos), result));
}

// /* @q RLink(target:Str, content:RichText...):RichText (Function)
//    Attaches a link to a rich text string.
//    Produces a rich text string that contains a link to the specified target,
//    and the concatenation of all %content parameters as text.

//    If any argument is EMPTY, returns EMPTY.

//    In text mode, this function just returns the concatenation of the %content,
//    as rich text attributes have no meaning to the text mode applications.

//    @since PCC2 1.99.21
//    @see RStyle, RXml */
afl::data::Value*
game::interface::IFRLink(game::Session& session, interpreter::Arguments& args)
{
    // Read link argument
    args.checkArgumentCountAtLeast(1);
    String_t link;
    if (!checkStringArg(link, args.getNext())) {
        return 0;
    }

    // Read remaining arguments, converting them to Rich Text. This is just what IFRAdd does.
    std::auto_ptr<afl::data::Value> tmp(IFRAdd(session, args));
    Ptr_t result;
    if (!checkRichArg(result, tmp.get())) {
        return 0;
    }

    // Build a link
    Ptr_t clone = new util::rich::Text(*result);
    clone->withNewAttribute(new util::rich::LinkAttribute(link));
    return new RichTextValue(clone);
}

// /* @q RXml(xml:Str, args:Str...):RichText (Function)
//    Create rich text string from XML.
//    Parses the %xml string.
//    Tags are converted into rich text attributes.
//    Entity references of the form &amp;&lt;digits&gt;; are replaced by the respective element from %args,
//    where the first element is &amp;0;.

//    For example,
//    <pre class="ccscript">
//      RXml("&lt;font color='&0;'>This is &lt;b>&1;&lt;/b>&lt;/font>", "red", "great")
//    </pre>
//    produces <font color="red">This is <b>great</b></font>.

//    In text mode, this function uses a simpler XML parser, and returns a plain string,
//    as rich text attributes have no meaning to the text mode applications.

//    @todo document the styles
//    @since PCC2 1.99.21
//    @see RStyle, RLink */
afl::data::Value*
game::interface::IFRXml(game::Session& /*session*/, interpreter::Arguments& args)
{
    // ex int/if/richif:IFRXml
    class MyEntityHandler : public afl::io::xml::EntityHandler {
     public:
        MyEntityHandler()
            : m_args()
            { }
        void addArgument(afl::data::Value* value)
            { m_args.push_back(value); }
        virtual String_t expandEntityReference(String_t name)
            {
                // ex IntXmlReader::expandEntity
                size_t n;
                if (afl::string::strToInteger(name, n)) {
                    if (n < m_args.size()) {
                        return interpreter::toString(m_args[n], false);
                    } else {
                        return String_t();
                    }
                } else {
                    return afl::io::xml::DefaultEntityHandler().expandEntityReference(name);
                }
            }
     private:
        std::vector<afl::data::Value*> m_args;
    };

    // Read XML text
    args.checkArgumentCountAtLeast(1);
    String_t xml;
    if (!checkStringArg(xml, args.getNext())) {
        return 0;
    }

    // Construct XML reader
    afl::io::ConstMemoryStream ms(afl::string::toBytes(xml));
    MyEntityHandler eh;
    while (args.getNumArgs() > 0) {
        eh.addArgument(args.getNext());
    }
    afl::io::xml::Reader rdr(ms, eh);
    util::rich::Parser p(rdr);
    p.readNext();

    // Read
    Ptr_t result = new util::rich::Text(p.parse());
    return new RichTextValue(result);
}
