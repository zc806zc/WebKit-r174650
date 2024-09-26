/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SVGToOTFFontConversion.h"

#include "CSSStyleDeclaration.h"
#include "ElementChildIterator.h"
#include "SVGFontElement.h"
#include "SVGFontFaceElement.h"
#include "SVGGlyphElement.h"
#include "SVGHKernElement.h"
#include "SVGMissingGlyphElement.h"
#include "SVGPathBuilder.h"
#include "SVGPathParser.h"
#include "SVGPathStringSource.h"
#include "SVGVKernElement.h"

namespace WebCore {

static inline void append32(Vector<char>& result, uint32_t value)
{
    result.append(value >> 24);
    result.append(value >> 16);
    result.append(value >> 8);
    result.append(value);
}

class SVGToOTFFontConverter {
public:
    SVGToOTFFontConverter(const SVGFontElement&);
    void convertSVGToOTFFont();

    Vector<char> releaseResult()
    {
        return WTF::move(m_result);
    }

private:
    struct GlyphData {
        GlyphData(Vector<char>&& charString, const SVGGlyphElement* glyphElement, float horizontalAdvance, float verticalAdvance, FloatRect boundingBox, const String& codepoints)
            : boundingBox(boundingBox)
            , charString(charString)
            , codepoints(codepoints)
            , glyphElement(glyphElement)
            , horizontalAdvance(horizontalAdvance)
            , verticalAdvance(verticalAdvance)
        {
        }
        FloatRect boundingBox;
        Vector<char> charString;
        String codepoints;
        const SVGGlyphElement* glyphElement;
        float horizontalAdvance;
        float verticalAdvance;
    };

    struct KerningData {
        KerningData(uint16_t glyph1, uint16_t glyph2, int16_t adjustment)
            : glyph1(glyph1)
            , glyph2(glyph2)
            , adjustment(adjustment)
        {
        }
        uint16_t glyph1;
        uint16_t glyph2;
        int16_t adjustment;
    };

    void append32(uint32_t value)
    {
        WebCore::append32(m_result, value);
    }

    void append32BitCode(const char code[4])
    {
        m_result.append(code[0]);
        m_result.append(code[1]);
        m_result.append(code[2]);
        m_result.append(code[3]);
    }

    void append16(uint16_t value)
    {
        m_result.append(value >> 8);
        m_result.append(value);
    }

    void overwrite32(unsigned location, uint32_t value)
    {
        ASSERT(m_result.size() >= location + 4);
        m_result[location] = value >> 24;
        m_result[location + 1] = value >> 16;
        m_result[location + 2] = value >> 8;
        m_result[location + 3] = value;
    }

    void overwrite16(unsigned location, uint16_t value)
    {
        ASSERT(m_result.size() >= location + 2);
        m_result[location] = value >> 8;
        m_result[location + 1] = value;
    }

    static const size_t headerSize = 12;
    static const size_t directoryEntrySize = 16;

    uint32_t calculateChecksum(size_t startingOffset, size_t endingOffset) const;

    void processGlyphElement(const SVGElement& glyphOrMissingGlyphElement, const SVGGlyphElement*, float defaultHorizontalAdvance, float defaultVerticalAdvance, const String& codepoints, bool& initialGlyph);

    typedef void (SVGToOTFFontConverter::*FontAppendingFunction)();
    void appendTable(const char identifier[4], FontAppendingFunction);
    void appendCMAPTable();
    void appendGSUBTable();
    void appendHEADTable();
    void appendHHEATable();
    void appendHMTXTable();
    void appendVHEATable();
    void appendVMTXTable();
    void appendKERNTable();
    void appendMAXPTable();
    void appendNAMETable();
    void appendOS2Table();
    void appendPOSTTable();
    void appendCFFTable();
    void appendVORGTable();

    static bool compareCodepointsLexicographically(const GlyphData&, const GlyphData&);

    void appendValidCFFString(const String&);

    Vector<char> transcodeGlyphPaths(float width, const SVGElement& glyphOrMissingGlyphElement, FloatRect& boundingBox) const;

    void addCodepointRanges(const UnicodeRanges&, HashSet<Glyph>& glyphSet) const;
    void addCodepoints(const HashSet<String>& codepoints, HashSet<Glyph>& glyphSet) const;
    void addGlyphNames(const HashSet<String>& glyphNames, HashSet<Glyph>& glyphSet) const;
    void addKerningPair(Vector<KerningData>&, const SVGKerningPair&) const;
    template<typename T> size_t appendKERNSubtable(bool (T::*buildKerningPair)(SVGKerningPair&) const, uint16_t coverage);
    size_t finishAppendingKERNSubtable(Vector<KerningData>, uint16_t coverage);

    void appendArabicReplacementSubtable(size_t subtableRecordLocation, const char arabicForm[]);

    Vector<GlyphData> m_glyphs;
    HashMap<String, Glyph> m_glyphNameToIndexMap; // SVG 1.1: "It is recommended that glyph names be unique within a font."
    HashMap<String, Vector<Glyph, 1>> m_codepointsToIndicesMap;
    Vector<char> m_result;
    FloatRect m_boundingBox;
    const SVGFontElement& m_fontElement;
    const SVGFontFaceElement* m_fontFaceElement;
    const SVGMissingGlyphElement* m_missingGlyphElement;
    String m_fontFamily;
    float m_advanceWidthMax;
    float m_advanceHeightMax;
    float m_minRightSideBearing;
    unsigned m_unitsPerEm;
    int m_tablesAppendedCount;
    char m_weight;
    bool m_italic;
};

static uint16_t roundDownToPowerOfTwo(uint16_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    return (x >> 1) + 1;
}

static uint16_t integralLog2(uint16_t x)
{
    uint16_t result = 0;
    while (x >>= 1)
        ++result;
    return result;
}

void SVGToOTFFontConverter::appendCMAPTable()
{
    auto startingOffset = m_result.size();
    append16(0);
    append16(1); // Number subtables

    append16(0); // Unicode
    append16(3); // Unicode version 2.2+
    append32(m_result.size() - startingOffset + sizeof(uint32_t)); // Byte offset of subtable

    // Braindead scheme: One segment for each character
    ASSERT(m_glyphs.size() < 0xFFFF);
    auto subtableLocation = m_result.size();
    append32(12 << 16); // Format 12
    append32(0); // Placeholder for byte length
    append32(0); // Language independent
    append32(0); // Placeholder for nGroups

    uint16_t nGroups = 0;
    UChar32 previousCodepoint = std::numeric_limits<UChar32>::max();
    for (size_t i = 0; i < m_glyphs.size(); ++i) {
        auto& glyph = m_glyphs[i];
        unsigned codepointSize = 0;
        UChar32 codepoint;
        if (glyph.codepoints.isNull())
            codepoint = 0;
        else if (glyph.codepoints.is8Bit())
            codepoint = glyph.codepoints[codepointSize++];
        else
            U16_NEXT(glyph.codepoints.characters16(), codepointSize, glyph.codepoints.length(), codepoint);

        // Don't map ligatures here.
        if (glyph.codepoints.length() > codepointSize || codepoint == previousCodepoint)
            continue;

        append32(codepoint); // startCharCode
        append32(codepoint); // endCharCode
        append32(i); // startGlyphCode
        ++nGroups;
        previousCodepoint = codepoint;
    }
    overwrite32(subtableLocation + 4, m_result.size() - subtableLocation);
    overwrite32(subtableLocation + 12, nGroups);
}

void SVGToOTFFontConverter::appendHEADTable()
{
    append32(0x00010000); // Version
    append32(0x00010000); // Revision
    append32(0); // Checksum placeholder; to be overwritten by the caller.
    append32(0x5F0F3CF5); // Magic number.
    append16((1 << 9) | 1);

    append16(m_unitsPerEm);
    append32(0); // First half of creation date
    append32(0); // Last half of creation date
    append32(0); // First half of modification date
    append32(0); // Last half of modification date
    append16(m_boundingBox.x()); // Minimum X
    append16(m_boundingBox.y()); // Minimum Y
    append16(m_boundingBox.maxX()); // Maximum X
    append16(m_boundingBox.maxY()); // Maximum Y
    append16((m_italic ? 1 << 1 : 0) | (m_weight >= 7 ? 1 : 0));
    append16(3); // Smallest readable size in pixels
    append16(0); // Might contain LTR or RTL glyphs
    append16(0); // Short offsets in the 'loca' table. However, OTF fonts don't have a 'loca' table so this is irrelevant
    append16(0); // Glyph data format
}

// Assumption: T2 can hold every value that a T1 can hold.
template<typename T1, typename T2> static inline T1 clampTo(T2 x)
{
    x = std::min(x, static_cast<T2>(std::numeric_limits<T1>::max()));
    x = std::max(x, static_cast<T2>(std::numeric_limits<T1>::min()));
    return static_cast<T1>(x);
}

void SVGToOTFFontConverter::appendHHEATable()
{
    int16_t ascent;
    int16_t descent;
    if (!m_fontFaceElement) {
        ascent = m_unitsPerEm;
        descent = 1;
    } else {
        ascent = m_fontFaceElement->ascent();
        descent = m_fontFaceElement->descent();

        // Some platforms, including OS X, use 0 ascent and descent to mean that the platform should synthesize
        // a value based on a heuristic. However, SVG fonts can legitimately have 0 for ascent or descent.
        // Specifing a single FUnit gets us as close to 0 as we can without triggering the synthesis.
        if (!ascent)
            ascent = 1;
        if (!descent)
            descent = 1;
    }

    append32(0x00010000); // Version
    append16(ascent);
    append16(descent);
    // WebKit SVG font rendering has hard coded the line gap to be 1/10th of the font size since 2008 (see r29719).
    append16(m_unitsPerEm / 10); // Line gap
    append16(clampTo<uint16_t>(m_advanceWidthMax));
    append16(clampTo<int16_t>(m_boundingBox.x())); // Minimum left side bearing
    append16(clampTo<int16_t>(m_minRightSideBearing)); // Minimum right side bearing
    append16(clampTo<int16_t>(m_boundingBox.maxX())); // X maximum extent
    // Since WebKit draws the caret and ignores the following values, it doesn't matter what we set them to.
    append16(1); // Vertical caret
    append16(0); // Vertical caret
    append16(0); // "Set value to 0 for non-slanted fonts"
    append32(0); // Reserved
    append32(0); // Reserved
    append16(0); // Current format
    append16(m_glyphs.size()); // Number of advance widths in HMTX table
}

void SVGToOTFFontConverter::appendHMTXTable()
{
    for (auto& glyph : m_glyphs) {
        append16(clampTo<uint16_t>(glyph.horizontalAdvance));
        append16(clampTo<int16_t>(glyph.boundingBox.x()));
    }
}

void SVGToOTFFontConverter::appendMAXPTable()
{
    append32(0x00010000); // Version
    append16(m_glyphs.size());
    append16(0xFFFF); // Maximum number of points in non-compound glyph
    append16(0xFFFF); // Maximum number of contours in non-compound glyph
    append16(0xFFFF); // Maximum number of points in compound glyph
    append16(0xFFFF); // Maximum number of contours in compound glyph
    append16(2); // Maximum number of zones
    append16(0); // Maximum number of points used in zone 0
    append16(0); // Maximum number of storage area locations
    append16(0); // Maximum number of function definitions
    append16(0); // Maximum number of instruction definitions
    append16(0); // Maximum stack depth
    append16(0); // Maximum size of instructions
    append16(m_glyphs.size()); // Maximum number of glyphs referenced at top level
    append16(0); // No compound glyphs
}

void SVGToOTFFontConverter::appendNAMETable()
{
    append16(0); // Format selector
    append16(1); // Number of name records in table
    append16(18); // Offset in bytes to the beginning of name character strings

    append16(0); // Unicode
    append16(3); // Unicode version 2.0 or later
    append16(0); // Language
    append16(1); // Name identifier. 1 = Font family
    append16(m_fontFamily.length());
    append16(0); // Offset into name data

    for (auto codeUnit : StringView(m_fontFamily).codeUnits())
        append16(codeUnit);
}

void SVGToOTFFontConverter::appendOS2Table()
{
    int16_t averageAdvance = m_unitsPerEm;
    bool ok;
    int value = m_fontElement.fastGetAttribute(SVGNames::horiz_adv_xAttr).toInt(&ok);
    if (!ok && m_missingGlyphElement)
        value = m_missingGlyphElement->fastGetAttribute(SVGNames::horiz_adv_xAttr).toInt(&ok);
    if (ok)
        averageAdvance = clampTo<int16_t>(value);

    append16(0); // Version
    append16(averageAdvance);
    append16(m_weight); // Weight class
    append16(5); // Width class
    append16(0); // Protected font
    // WebKit handles these superscripts and subscripts
    append16(0); // Subscript X Size
    append16(0); // Subscript Y Size
    append16(0); // Subscript X Offset
    append16(0); // Subscript Y Offset
    append16(0); // Superscript X Size
    append16(0); // Superscript Y Size
    append16(0); // Superscript X Offset
    append16(0); // Superscript Y Offset
    append16(0); // Strikeout width
    append16(0); // Strikeout Position
    append16(0); // No classification

    unsigned numPanoseBytes = 0;
    const unsigned panoseSize = 10;
    char panoseBytes[panoseSize];
    if (m_fontFaceElement) {
        Vector<String> segments;
        m_fontFaceElement->fastGetAttribute(SVGNames::panose_1Attr).string().split(' ', segments);
        if (segments.size() == panoseSize) {
            for (auto& segment : segments) {
                bool ok;
                int value = segment.toInt(&ok);
                if (ok && value >= 0 && value <= 0xFF)
                    panoseBytes[numPanoseBytes++] = value;
            }
        }
    }
    if (numPanoseBytes != panoseSize)
        memset(panoseBytes, 0, panoseSize);
    m_result.append(panoseBytes, panoseSize);

    for (int i = 0; i < 4; ++i)
        append32(0); // "Bit assignments are pending. Set to 0"
    append32(0x544B4257); // Font Vendor. "WBKT"
    append16((m_weight >= 7 ? 1 << 5 : 0) | (m_italic ? 1 : 0)); // Font Patterns.
    append16(0); // First unicode index
    append16(0xFFFF); // Last unicode index
}

void SVGToOTFFontConverter::appendPOSTTable()
{
    append32(0x00030000); // Format. Printing is undefined
    append32(0); // Italic angle in degrees
    append16(0); // Underline position
    append16(0); // Underline thickness
    append32(0); // Monospaced
    append32(0); // "Minimum memory usage when a TrueType font is downloaded as a Type 42 font"
    append32(0); // "Maximum memory usage when a TrueType font is downloaded as a Type 42 font"
    append32(0); // "Minimum memory usage when a TrueType font is downloaded as a Type 1 font"
    append32(0); // "Maximum memory usage when a TrueType font is downloaded as a Type 1 font"
}

static bool isValidStringForCFF(const String& string)
{
    for (auto c : StringView(string).codeUnits()) {
        if (c < 33 || c > 126)
            return false;
    }
    return true;
}

void SVGToOTFFontConverter::appendValidCFFString(const String& string)
{
    ASSERT(isValidStringForCFF(string));
    for (auto c : StringView(string).codeUnits())
        m_result.append(c);
}

void SVGToOTFFontConverter::appendCFFTable()
{
    auto startingOffset = m_result.size();

    // Header
    m_result.append(1); // Major version
    m_result.append(0); // Minor version
    m_result.append(4); // Header size
    m_result.append(4); // Offsets within CFF table are 4 bytes long

    // Name INDEX
    String fontName;
    if (m_fontFaceElement) {
        // FIXME: fontFamily() here might not be quite what we want.
        String potentialFontName = m_fontFamily;
        if (isValidStringForCFF(potentialFontName))
            fontName = potentialFontName;
    }
    append16(1); // INDEX contains 1 element
    m_result.append(4); // Offsets in this INDEX are 4 bytes long
    append32(1); // 1-index offset of name data
    append32(fontName.length() + 1); // 1-index offset just past end of name data
    appendValidCFFString(fontName);

    String weight;
    if (m_fontFaceElement) {
        auto& potentialWeight = m_fontFaceElement->fastGetAttribute(SVGNames::font_weightAttr);
        if (isValidStringForCFF(potentialWeight))
            weight = potentialWeight;
    }

    bool hasWeight = !weight.isNull();

    const char operand32Bit = 29;
    const char fullNameKey = 2;
    const char familyNameKey = 3;
    const char weightKey = 4;
    const char fontBBoxKey = 5;
    const char charsetIndexKey = 15;
    const char charstringsIndexKey = 17;
    const uint32_t userDefinedStringStartIndex = 391;
    const unsigned sizeOfTopIndex = 45 + (hasWeight ? 6 : 0);

    // Top DICT INDEX.
    append16(1); // INDEX contains 1 element
    m_result.append(4); // Offsets in this INDEX are 4 bytes long
    append32(1); // 1-index offset of DICT data
    append32(1 + sizeOfTopIndex); // 1-index offset just past end of DICT data

    // DICT information
#if !ASSERT_DISABLED
    unsigned topDictStart = m_result.size();
#endif
    m_result.append(operand32Bit);
    append32(userDefinedStringStartIndex);
    m_result.append(fullNameKey);
    m_result.append(operand32Bit);
    append32(userDefinedStringStartIndex);
    m_result.append(familyNameKey);
    if (hasWeight) {
        m_result.append(operand32Bit);
        append32(userDefinedStringStartIndex + 1);
        m_result.append(weightKey);
    }
    m_result.append(operand32Bit);
    append32(clampTo<int32_t>(m_boundingBox.x()));
    m_result.append(operand32Bit);
    append32(clampTo<int32_t>(m_boundingBox.maxX()));
    m_result.append(operand32Bit);
    append32(clampTo<int32_t>(m_boundingBox.y()));
    m_result.append(operand32Bit);
    append32(clampTo<int32_t>(m_boundingBox.maxY()));
    m_result.append(fontBBoxKey);
    m_result.append(operand32Bit);
    unsigned charsetOffsetLocation = m_result.size();
    append32(0); // Offset of Charset info. Will be overwritten later.
    m_result.append(charsetIndexKey);
    m_result.append(operand32Bit);
    unsigned charstringsOffsetLocation = m_result.size();
    append32(0); // Offset of CharStrings INDEX. Will be overwritten later.
    m_result.append(charstringsIndexKey);
    ASSERT(m_result.size() == topDictStart + sizeOfTopIndex);

    // String INDEX
    append16(1 + (hasWeight ? 1 : 0)); // Number of elements in INDEX
    m_result.append(4); // Offsets in this INDEX are 4 bytes long
    uint32_t offset = 1;
    append32(offset);
    offset += fontName.length();
    append32(offset);
    if (hasWeight) {
        offset += weight.length();
        append32(offset);
    }
    appendValidCFFString(fontName);
    appendValidCFFString(weight);

    append16(0); // Empty subroutine INDEX

    // Charset info
    overwrite32(charsetOffsetLocation, m_result.size() - startingOffset);
    m_result.append(0);
    for (Glyph i = 1; i < m_glyphs.size(); ++i)
        append16(i);

    // CharStrings INDEX
    overwrite32(charstringsOffsetLocation, m_result.size() - startingOffset);
    append16(m_glyphs.size());
    m_result.append(4); // Offsets in this INDEX are 4 bytes long
    offset = 1;
    append32(offset);
    for (auto& glyph : m_glyphs) {
        offset += glyph.charString.size();
        append32(offset);
    }
    for (auto& glyph : m_glyphs)
        m_result.appendVector(glyph.charString);
}

void SVGToOTFFontConverter::appendArabicReplacementSubtable(size_t subtableRecordLocation, const char arabicForm[])
{
    Vector<std::pair<Glyph, Glyph>> arabicFinalReplacements;
    for (auto& pair : m_codepointsToIndicesMap) {
        for (auto glyphIndex : pair.value) {
            auto& glyph = m_glyphs[glyphIndex];
            if (glyph.glyphElement && equalIgnoringCase(glyph.glyphElement->fastGetAttribute(SVGNames::arabic_formAttr), arabicForm))
                arabicFinalReplacements.append(std::make_pair(pair.value[0], glyphIndex));
        }
    }
    if (arabicFinalReplacements.size() > std::numeric_limits<uint16_t>::max())
        arabicFinalReplacements.clear();

    overwrite16(subtableRecordLocation + 6, m_result.size() - subtableRecordLocation);
    auto subtableLocation = m_result.size();
    append16(2); // Format 2
    append16(0); // Placeholder for offset to coverage table, relative to beginning of substitution table
    append16(arabicFinalReplacements.size()); // GlyphCount
    for (auto& pair : arabicFinalReplacements)
        append16(pair.second);

    overwrite16(subtableLocation + 2, m_result.size() - subtableLocation);
    append16(1); // CoverageFormat
    append16(arabicFinalReplacements.size()); // GlyphCount
    for (auto& pair : arabicFinalReplacements)
        append16(pair.first);
}

void SVGToOTFFontConverter::appendGSUBTable()
{
    auto tableLocation = m_result.size();
    auto headerSize = 10;

    append32(0x00010000); // Version
    append16(headerSize); // Offset to ScriptList
    auto featureListOffsetLocation = m_result.size();
    append16(0); // Placeholder for FeatureList offset
    auto lookupListOffsetLocation = m_result.size();
    append16(0); // Placeholder for LookupList offset
    ASSERT(tableLocation + headerSize == m_result.size());

    // ScriptList
    auto scriptListLocation = m_result.size();
    append16(1); // Number of ScriptRecords
    append32BitCode("arab");
    append16(m_result.size() + 2 - scriptListLocation); // Offset of Script table, relative to beginning of ScriptList

    auto scriptTableLocation = m_result.size();
    auto sizeOfEmptyScriptTable = 4;
    append16(sizeOfEmptyScriptTable); // Offset of default language system table, relative to beginning of Script table
    append16(0); // Number of following language system tables
    ASSERT_UNUSED(scriptTableLocation, scriptTableLocation + sizeOfEmptyScriptTable == m_result.size());

    append16(0); // LookupOrder "= NULL ... reserved"
    append16(0); // First feature is required
    uint16_t featureCount = 4;
    append16(4); // FeatureCount
    for (uint16_t i = 0; i < featureCount; ++i)
        append16(i); // Index of our feature into the FeatureList

    // FeatureList
    overwrite16(featureListOffsetLocation, m_result.size() - tableLocation);
    auto featureListLocation = m_result.size();
    size_t featureListSize = 2 + 6 * featureCount;
    size_t featureTableSize = 6;
    append16(featureCount); // FeatureCount
    append32BitCode("fina");
    append16(featureListSize + featureTableSize * 0); // Offset of feature table, relative to beginning of FeatureList table
    append32BitCode("medi");
    append16(featureListSize + featureTableSize * 1); // Offset of feature table, relative to beginning of FeatureList table
    append32BitCode("init");
    append16(featureListSize + featureTableSize * 2); // Offset of feature table, relative to beginning of FeatureList table
    append32BitCode("rlig");
    append16(featureListSize + featureTableSize * 3); // Offset of feature table, relative to beginning of FeatureList table
    ASSERT_UNUSED(featureListLocation, featureListLocation + featureListSize == m_result.size());

    for (unsigned i = 0; i < featureCount; ++i) {
        auto featureTableStart = m_result.size();
        append16(0); // FeatureParams "= NULL ... reserved"
        append16(1); // LookupCount
        append16(i); // LookupListIndex
        ASSERT_UNUSED(featureTableStart, featureTableStart + featureTableSize == m_result.size());
    }

    // LookupList
    overwrite16(lookupListOffsetLocation, m_result.size() - tableLocation);
    auto lookupListLocation = m_result.size();
    append16(featureCount); // LookupCount
    for (unsigned i = 0; i < featureCount; ++i)
        append16(0); // Placeholder for offset to feature table, relative to beginning of LookupList
    size_t subtableRecordLocations[featureCount];
    for (unsigned i = 0; i < featureCount; ++i) {
        subtableRecordLocations[i] = m_result.size();
        overwrite16(lookupListLocation + 2 + 2 * i, m_result.size() - lookupListLocation);
        append16(i == 3 ? 3 : 1); // Type 1: "Replace one glyph with one glyph" / Type 3: "Replace one glyph with one of many glyphs"
        append16(0); // LookupFlag
        append16(1); // SubTableCount
        append16(0); // Placeholder for offset to subtable, relative to beginning of Lookup table
    }

    appendArabicReplacementSubtable(subtableRecordLocations[0], "terminal");
    appendArabicReplacementSubtable(subtableRecordLocations[1], "medial");
    appendArabicReplacementSubtable(subtableRecordLocations[2], "initial");

    // Manually append empty "rlig" subtable
    overwrite16(subtableRecordLocations[3] + 6, m_result.size() - subtableRecordLocations[3]);
    append16(1); // Format 1
    append16(6); // offset to coverage table, relative to beginning of substitution table
    append16(0); // AlternateSetCount
    append16(1); // CoverageFormat
    append16(0); // GlyphCount
}

void SVGToOTFFontConverter::appendVORGTable()
{
    append16(1); // Major version
    append16(0); // Minor version

    bool ok;
    int16_t defaultVerticalOriginY = clampTo<int16_t>(m_fontElement.fastGetAttribute(SVGNames::vert_origin_yAttr).toInt(&ok));
    if (!ok && m_missingGlyphElement)
        defaultVerticalOriginY = clampTo<int16_t>(m_missingGlyphElement->fastGetAttribute(SVGNames::vert_origin_yAttr).toInt());
    append16(defaultVerticalOriginY);

    auto tableSizeOffset = m_result.size();
    append16(0); // Place to write table size.
    for (Glyph i = 0; i < m_glyphs.size(); ++i) {
        if (auto* glyph = m_glyphs[i].glyphElement) {
            if (int16_t verticalOriginY = clampTo<int16_t>(glyph->fastGetAttribute(SVGNames::vert_origin_yAttr).toInt())) {
                append16(i);
                append16(verticalOriginY);
            }
        }
    }
    ASSERT(!((m_result.size() - tableSizeOffset - 2) % 4));
    overwrite16(tableSizeOffset, (m_result.size() - tableSizeOffset - 2) / 4);
}

void SVGToOTFFontConverter::appendVHEATable()
{
    append32(0x00011000); // Version
    append16(m_unitsPerEm / 2); // Vertical typographic ascender (vertical baseline to the right)
    append16(clampTo<int16_t>(-static_cast<int>(m_unitsPerEm / 2))); // Vertical typographic descender
    append16(m_unitsPerEm / 10); // Vertical typographic line gap
    append16(clampTo<int16_t>(m_advanceHeightMax));
    append16(clampTo<int16_t>(m_unitsPerEm - m_boundingBox.maxY())); // Minimum top side bearing
    append16(clampTo<int16_t>(m_boundingBox.y())); // Minimum bottom side bearing
    append16(clampTo<int16_t>(m_unitsPerEm - m_boundingBox.y())); // Y maximum extent
    // Since WebKit draws the caret and ignores the following values, it doesn't matter what we set them to.
    append16(1); // Vertical caret
    append16(0); // Vertical caret
    append16(0); // "Set value to 0 for non-slanted fonts"
    append32(0); // Reserved
    append32(0); // Reserved
    append16(0); // "Set to 0"
    append16(m_glyphs.size()); // Number of advance heights in VMTX table
}

void SVGToOTFFontConverter::appendVMTXTable()
{
    for (auto& glyph : m_glyphs) {
        append16(clampTo<uint16_t>(glyph.verticalAdvance));
        append16(clampTo<int16_t>(m_unitsPerEm - glyph.boundingBox.maxY())); // top side bearing
    }
}

void SVGToOTFFontConverter::addCodepointRanges(const UnicodeRanges& unicodeRanges, HashSet<Glyph>& glyphSet) const
{
    for (auto& unicodeRange : unicodeRanges) {
        for (auto codepoint = unicodeRange.first; codepoint <= unicodeRange.second; ++codepoint) {
            UChar buffer[2];
            uint8_t length = 0;
            UBool error = false;
            U16_APPEND(buffer, length, 2, codepoint, error);
            if (!error) {
                for (auto index : m_codepointsToIndicesMap.get(String(buffer, length)))
                    glyphSet.add(index);
            }
        }
    }
}

void SVGToOTFFontConverter::addCodepoints(const HashSet<String>& codepoints, HashSet<Glyph>& glyphSet) const
{
    for (auto& codepointString : codepoints) {
        for (auto index : m_codepointsToIndicesMap.get(codepointString))
            glyphSet.add(index);
    }
}

void SVGToOTFFontConverter::addGlyphNames(const HashSet<String>& glyphNames, HashSet<Glyph>& glyphSet) const
{
    for (auto& glyphName : glyphNames) {
        if (Glyph glyph = m_glyphNameToIndexMap.get(glyphName))
            glyphSet.add(glyph);
    }
}

void SVGToOTFFontConverter::addKerningPair(Vector<KerningData>& data, const SVGKerningPair& kerningPair) const
{
    HashSet<Glyph> glyphSet1;
    HashSet<Glyph> glyphSet2;

    addCodepointRanges(kerningPair.unicodeRange1, glyphSet1);
    addCodepointRanges(kerningPair.unicodeRange2, glyphSet2);
    addGlyphNames(kerningPair.glyphName1, glyphSet1);
    addGlyphNames(kerningPair.glyphName2, glyphSet2);
    addCodepoints(kerningPair.unicodeName1, glyphSet1);
    addCodepoints(kerningPair.unicodeName2, glyphSet2);

    // FIXME: Use table format 2 so we don't have to append each of these one by one.
    for (auto& glyph1 : glyphSet1) {
        for (auto& glyph2 : glyphSet2)
            data.append(KerningData(glyph1, glyph2, clampTo<int16_t>(-kerningPair.kerning)));
    }
}

template<typename T> inline size_t SVGToOTFFontConverter::appendKERNSubtable(bool (T::*buildKerningPair)(SVGKerningPair&) const, uint16_t coverage)
{
    Vector<KerningData> kerningData;
    for (auto& element : childrenOfType<T>(m_fontElement)) {
        SVGKerningPair kerningPair;
        if ((element.*buildKerningPair)(kerningPair))
            addKerningPair(kerningData, kerningPair);
    }
    return finishAppendingKERNSubtable(WTF::move(kerningData), coverage);
}

size_t SVGToOTFFontConverter::finishAppendingKERNSubtable(Vector<KerningData> kerningData, uint16_t coverage)
{
    std::sort(kerningData.begin(), kerningData.end(), [](const KerningData& a, const KerningData& b) {
        return a.glyph1 < b.glyph1 || (a.glyph1 == b.glyph1 && a.glyph2 < b.glyph2);
    });

    size_t sizeOfKerningDataTable = 14 + 6 * kerningData.size();
    if (sizeOfKerningDataTable > std::numeric_limits<uint16_t>::max()) {
        kerningData.clear();
        sizeOfKerningDataTable = 14;
    }

    append16(0); // Version of subtable
    append16(sizeOfKerningDataTable); // Length of this subtable
    append16(coverage); // Table coverage bitfield

    uint16_t roundedNumKerningPairs = roundDownToPowerOfTwo(kerningData.size());

    append16(kerningData.size());
    append16(roundedNumKerningPairs * 6); // searchRange: "The largest power of two less than or equal to the value of nPairs, multiplied by the size in bytes of an entry in the table."
    append16(integralLog2(roundedNumKerningPairs)); // entrySelector: "log2 of the largest power of two less than or equal to the value of nPairs."
    append16((kerningData.size() - roundedNumKerningPairs) * 6); // rangeShift: "The value of nPairs minus the largest power of two less than or equal to nPairs,
                                                                        // and then multiplied by the size in bytes of an entry in the table."

    for (auto& kerningData : kerningData) {
        append16(kerningData.glyph1);
        append16(kerningData.glyph2);
        append16(kerningData.adjustment);
    }

    return sizeOfKerningDataTable;
}

void SVGToOTFFontConverter::appendKERNTable()
{
    append16(0); // Version
    append16(2); // Number of subtables

#if !ASSERT_DISABLED
    auto subtablesOffset = m_result.size();
#endif

    size_t sizeOfHorizontalSubtable = appendKERNSubtable<SVGHKernElement>(&SVGHKernElement::buildHorizontalKerningPair, 1);
    ASSERT_UNUSED(sizeOfHorizontalSubtable, subtablesOffset + sizeOfHorizontalSubtable == m_result.size());
    size_t sizeOfVerticalSubtable = appendKERNSubtable<SVGVKernElement>(&SVGVKernElement::buildVerticalKerningPair, 0);
    ASSERT_UNUSED(sizeOfVerticalSubtable, subtablesOffset + sizeOfHorizontalSubtable + sizeOfVerticalSubtable == m_result.size());

    // Work around a bug in Apple's font parser by adding some padding bytes. <rdar://problem/18401901>
    for (int i = 0; i < 6; ++i)
        m_result.append(0);
}

static void writeCFFEncodedNumber(Vector<char>& vector, float number)
{
    vector.append(0xFF);
    append32(vector, number * 0x10000);
}

static const char rLineTo = 0x05;
static const char rrCurveTo = 0x08;
static const char endChar = 0x0e;
static const char rMoveTo = 0x15;

class CFFBuilder : public SVGPathBuilder {
public:
    CFFBuilder(Vector<char>& cffData, float width, FloatPoint origin)
        : m_cffData(cffData)
        , m_hasBoundingBox(false)
    {
        writeCFFEncodedNumber(m_cffData, width);
        writeCFFEncodedNumber(m_cffData, origin.x());
        writeCFFEncodedNumber(m_cffData, origin.y());
        m_cffData.append(rMoveTo);
    }

    FloatRect boundingBox() const
    {
        return m_boundingBox;
    }

private:
    void updateBoundingBox(FloatPoint point)
    {
        if (!m_hasBoundingBox) {
            m_boundingBox = FloatRect(point, FloatSize());
            m_hasBoundingBox = true;
            return;
        }
        m_boundingBox.extend(point);
    }

    void writePoint(FloatPoint destination)
    {
        updateBoundingBox(destination);

        FloatSize delta = destination - m_current;
        writeCFFEncodedNumber(m_cffData, delta.width());
        writeCFFEncodedNumber(m_cffData, delta.height());

        m_current = destination;
    }

    virtual void moveTo(const FloatPoint& targetPoint, bool closed, PathCoordinateMode mode) override
    {
        if (closed && m_cffData.size())
            closePath();

        FloatPoint destination = mode == AbsoluteCoordinates ? targetPoint : m_current + targetPoint;

        writePoint(destination);
        m_cffData.append(rMoveTo);

        m_startingPoint = m_current;
    }

    virtual void lineTo(const FloatPoint& targetPoint, PathCoordinateMode mode) override
    {
        FloatPoint destination = mode == AbsoluteCoordinates ? targetPoint : m_current + targetPoint;

        writePoint(destination);
        m_cffData.append(rLineTo);
    }

    virtual void curveToCubic(const FloatPoint& point1, const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode) override
    {
        FloatPoint destination1 = point1;
        FloatPoint destination2 = point2;
        FloatPoint destination3 = targetPoint;
        if (mode == RelativeCoordinates) {
            destination1 += m_current;
            destination2 += m_current;
            destination3 += m_current;
        }

        writePoint(destination1);
        writePoint(destination2);
        writePoint(destination3);
        m_cffData.append(rrCurveTo);
    }

    virtual void closePath() override
    {
        if (m_current != m_startingPoint)
            lineTo(m_startingPoint, AbsoluteCoordinates);
    }

    Vector<char>& m_cffData;
    FloatPoint m_startingPoint;
    FloatRect m_boundingBox;
    bool m_hasBoundingBox;
};

Vector<char> SVGToOTFFontConverter::transcodeGlyphPaths(float width, const SVGElement& glyphOrMissingGlyphElement, FloatRect& boundingBox) const
{
    Vector<char> result;

    auto& dAttribute = glyphOrMissingGlyphElement.fastGetAttribute(SVGNames::dAttr);
    if (dAttribute.isNull()) {
        writeCFFEncodedNumber(result, width);
        writeCFFEncodedNumber(result, 0);
        writeCFFEncodedNumber(result, 0);
        result.append(rMoveTo);
        result.append(endChar);
        return result;
    }

    // FIXME: If we are vertical, use vert_origin_x and vert_origin_y
    bool ok;
    float horizontalOriginX = glyphOrMissingGlyphElement.fastGetAttribute(SVGNames::horiz_origin_xAttr).toFloat(&ok);
    if (!ok)
        horizontalOriginX = m_fontFaceElement ? m_fontFaceElement->horizontalOriginX() : 0;
    float horizontalOriginY = glyphOrMissingGlyphElement.fastGetAttribute(SVGNames::horiz_origin_yAttr).toFloat(&ok);
    if (!ok && m_fontFaceElement)
        horizontalOriginY = m_fontFaceElement ? m_fontFaceElement->horizontalOriginY() : 0;

    CFFBuilder builder(result, width, FloatPoint(horizontalOriginX, horizontalOriginY));
    SVGPathStringSource source(dAttribute);

    SVGPathParser parser;
    parser.setCurrentSource(&source);
    parser.setCurrentConsumer(&builder);

    ok = parser.parsePathDataFromSource(NormalizedParsing);
    parser.cleanup();

    if (!ok)
        result.clear();

    boundingBox = builder.boundingBox();

    result.append(endChar);
    return result;
}

void SVGToOTFFontConverter::processGlyphElement(const SVGElement& glyphOrMissingGlyphElement, const SVGGlyphElement* glyphElement, float defaultHorizontalAdvance, float defaultVerticalAdvance, const String& codepoints, bool& initialGlyph)
{
    bool ok;
    float horizontalAdvance = glyphOrMissingGlyphElement.fastGetAttribute(SVGNames::horiz_adv_xAttr).toFloat(&ok);
    if (!ok)
        horizontalAdvance = defaultHorizontalAdvance;
    m_advanceWidthMax = std::max(m_advanceWidthMax, horizontalAdvance);
    float verticalAdvance = glyphOrMissingGlyphElement.fastGetAttribute(SVGNames::vert_adv_yAttr).toFloat(&ok);
    if (!ok)
        verticalAdvance = defaultVerticalAdvance;
    m_advanceHeightMax = std::max(m_advanceHeightMax, verticalAdvance);

    FloatRect glyphBoundingBox;
    auto path = transcodeGlyphPaths(horizontalAdvance, glyphOrMissingGlyphElement, glyphBoundingBox);
    if (initialGlyph)
        m_boundingBox = glyphBoundingBox;
    else
        m_boundingBox.unite(glyphBoundingBox);
    m_minRightSideBearing = std::min(m_minRightSideBearing, horizontalAdvance - glyphBoundingBox.maxX());
    initialGlyph = false;

    m_glyphs.append(GlyphData(WTF::move(path), glyphElement, horizontalAdvance, verticalAdvance, m_boundingBox, codepoints));
}

bool SVGToOTFFontConverter::compareCodepointsLexicographically(const GlyphData& data1, const GlyphData& data2)
{
    unsigned i1 = 0;
    unsigned i2 = 0;
    unsigned length1 = data1.codepoints.length();
    unsigned length2 = data2.codepoints.length();
    while (i1 < length1 && i2 < length2) {
        UChar32 codepoint1, codepoint2;
        if (data1.codepoints.is8Bit())
            codepoint1 = data1.codepoints[i1++];
        else
            U16_NEXT(data1.codepoints.characters16(), i1, length1, codepoint1);

        if (data2.codepoints.is8Bit())
            codepoint2 = data2.codepoints[i2++];
        else
            U16_NEXT(data2.codepoints.characters16(), i2, length2, codepoint2);

        if (codepoint1 < codepoint2)
            return true;
        if (codepoint1 > codepoint2)
            return false;
    }

    if (length1 == length2 && data1.glyphElement
        && equalIgnoringCase(data1.glyphElement->fastGetAttribute(SVGNames::arabic_formAttr), "isolated"))
        return true;
    return length1 < length2;
}

SVGToOTFFontConverter::SVGToOTFFontConverter(const SVGFontElement& fontElement)
    : m_fontElement(fontElement)
    , m_fontFaceElement(childrenOfType<SVGFontFaceElement>(m_fontElement).first())
    , m_missingGlyphElement(childrenOfType<SVGMissingGlyphElement>(m_fontElement).first())
    , m_advanceWidthMax(0)
    , m_advanceHeightMax(0)
    , m_minRightSideBearing(std::numeric_limits<float>::max())
    , m_unitsPerEm(0)
    , m_tablesAppendedCount(0)
    , m_weight(5)
    , m_italic(false)
{
    float defaultHorizontalAdvance = m_fontFaceElement ? m_fontFaceElement->horizontalAdvanceX() : 0;
    float defaultVerticalAdvance = m_fontFaceElement ? m_fontFaceElement->verticalAdvanceY() : 0;
    bool initialGlyph = true;

    if (m_fontFaceElement)
        m_unitsPerEm = m_fontFaceElement->unitsPerEm();

    if (m_missingGlyphElement)
        processGlyphElement(*m_missingGlyphElement, nullptr, defaultHorizontalAdvance, defaultVerticalAdvance, String(), initialGlyph);
    else {
        Vector<char> notdefCharString;
        writeCFFEncodedNumber(notdefCharString, m_unitsPerEm);
        writeCFFEncodedNumber(notdefCharString, 0);
        writeCFFEncodedNumber(notdefCharString, 0);
        notdefCharString.append(rMoveTo);
        notdefCharString.append(endChar);
        m_glyphs.append(GlyphData(WTF::move(notdefCharString), nullptr, m_unitsPerEm, m_unitsPerEm, FloatRect(), String()));
    }

    for (auto& glyphElement : childrenOfType<SVGGlyphElement>(m_fontElement)) {
        auto& unicodeAttribute = glyphElement.fastGetAttribute(SVGNames::unicodeAttr);
        if (!unicodeAttribute.isEmpty()) // If we can never actually trigger this glyph, ignore it completely
            processGlyphElement(glyphElement, &glyphElement, defaultHorizontalAdvance, defaultVerticalAdvance, unicodeAttribute, initialGlyph);
    }

    if (m_glyphs.size() > std::numeric_limits<Glyph>::max()) {
        m_glyphs.clear();
        return;
    }

    std::sort(m_glyphs.begin(), m_glyphs.end(), &compareCodepointsLexicographically);

    for (Glyph i = 0; i < m_glyphs.size(); ++i) {
        GlyphData& glyph = m_glyphs[i];
        if (glyph.glyphElement) {
            auto& glyphName = glyph.glyphElement->fastGetAttribute(SVGNames::glyph_nameAttr);
            if (!glyphName.isNull())
                m_glyphNameToIndexMap.add(glyphName, i);
        }
        if (m_codepointsToIndicesMap.isValidKey(glyph.codepoints)) {
            auto& glyphVector = m_codepointsToIndicesMap.add(glyph.codepoints, Vector<Glyph>()).iterator->value;
            // Prefer isolated arabic forms
            if (glyph.glyphElement && equalIgnoringCase(glyph.glyphElement->fastGetAttribute(SVGNames::arabic_formAttr), "isolated"))
                glyphVector.insert(0, i);
            else
                glyphVector.append(i);
        }
    }

    // FIXME: Handle commas.
    if (m_fontFaceElement) {
        Vector<String> segments;
        m_fontFaceElement->fastGetAttribute(SVGNames::font_weightAttr).string().split(' ', segments);
        for (auto& segment : segments) {
            if (equalIgnoringCase(segment, "bold")) {
                m_weight = 7;
                break;
            }
            bool ok;
            int value = segment.toInt(&ok);
            if (ok && value >= 0 && value < 1000) {
                m_weight = (value + 50) / 100;
                break;
            }
        }
        m_fontFaceElement->fastGetAttribute(SVGNames::font_styleAttr).string().split(' ', segments);
        for (auto& segment : segments) {
            if (equalIgnoringCase(segment, "italic") || equalIgnoringCase(segment, "oblique")) {
                m_italic = true;
                break;
            }
        }
    }

    if (m_fontFaceElement)
        m_fontFamily = m_fontFaceElement->fontFamily();
}

static inline bool isFourByteAligned(size_t x)
{
    return !(x & 3);
}

uint32_t SVGToOTFFontConverter::calculateChecksum(size_t startingOffset, size_t endingOffset) const
{
    ASSERT(isFourByteAligned(endingOffset - startingOffset));
    uint32_t sum = 0;
    for (size_t offset = startingOffset; offset < endingOffset; offset += 4) {
        sum += (static_cast<unsigned char>(m_result[offset + 3]) << 24)
            | (static_cast<unsigned char>(m_result[offset + 2]) << 16)
            | (static_cast<unsigned char>(m_result[offset + 1]) << 8)
            | static_cast<unsigned char>(m_result[offset]);
    }
    return sum;
}

void SVGToOTFFontConverter::appendTable(const char identifier[4], FontAppendingFunction appendingFunction)
{
    size_t offset = m_result.size();
    ASSERT(isFourByteAligned(offset));
    (this->*appendingFunction)();
    size_t unpaddedSize = m_result.size() - offset;
    while (!isFourByteAligned(m_result.size()))
        m_result.append(0);
    ASSERT(isFourByteAligned(m_result.size()));
    size_t directoryEntryOffset = headerSize + m_tablesAppendedCount * directoryEntrySize;
    m_result[directoryEntryOffset] = identifier[0];
    m_result[directoryEntryOffset + 1] = identifier[1];
    m_result[directoryEntryOffset + 2] = identifier[2];
    m_result[directoryEntryOffset + 3] = identifier[3];
    overwrite32(directoryEntryOffset + 4, calculateChecksum(offset, m_result.size()));
    overwrite32(directoryEntryOffset + 8, offset);
    overwrite32(directoryEntryOffset + 12, unpaddedSize);
    ++m_tablesAppendedCount;
}

void SVGToOTFFontConverter::convertSVGToOTFFont()
{
    if (m_glyphs.isEmpty())
        return;

    uint16_t numTables = 14;
    uint16_t roundedNumTables = roundDownToPowerOfTwo(numTables);
    uint16_t searchRange = roundedNumTables * 16; // searchRange: "(Maximum power of 2 <= numTables) x 16."

    m_result.append('O');
    m_result.append('T');
    m_result.append('T');
    m_result.append('O');
    append16(numTables);
    append16(searchRange);
    append16(integralLog2(roundedNumTables)); // entrySelector: "Log2(maximum power of 2 <= numTables)."
    append16(numTables * 16 - searchRange); // rangeShift: "NumTables x 16-searchRange."

    ASSERT(m_result.size() == headerSize);

    // Leave space for the directory entries.
    for (size_t i = 0; i < directoryEntrySize * numTables; ++i)
        m_result.append(0);

    appendTable("CFF ", &SVGToOTFFontConverter::appendCFFTable);
    appendTable("GSUB", &SVGToOTFFontConverter::appendGSUBTable);
    appendTable("OS/2", &SVGToOTFFontConverter::appendOS2Table);
    appendTable("VORG", &SVGToOTFFontConverter::appendVORGTable);
    appendTable("cmap", &SVGToOTFFontConverter::appendCMAPTable);
    auto headTableOffset = m_result.size();
    appendTable("head", &SVGToOTFFontConverter::appendHEADTable);
    appendTable("hhea", &SVGToOTFFontConverter::appendHHEATable);
    appendTable("hmtx", &SVGToOTFFontConverter::appendHMTXTable);
    appendTable("kern", &SVGToOTFFontConverter::appendKERNTable);
    appendTable("maxp", &SVGToOTFFontConverter::appendMAXPTable);
    appendTable("name", &SVGToOTFFontConverter::appendNAMETable);
    appendTable("post", &SVGToOTFFontConverter::appendPOSTTable);
    appendTable("vhea", &SVGToOTFFontConverter::appendVHEATable);
    appendTable("vmtx", &SVGToOTFFontConverter::appendVMTXTable);

    ASSERT(numTables == m_tablesAppendedCount);

    // checksumAdjustment: "To compute: set it to 0, calculate the checksum for the 'head' table and put it in the table directory,
    // sum the entire font as uint32, then store B1B0AFBA - sum. The checksum for the 'head' table will now be wrong. That is OK."
    overwrite32(headTableOffset + 8, 0xB1B0AFBAU - calculateChecksum(0, m_result.size()));
}

Vector<char> convertSVGToOTFFont(const SVGFontElement& element)
{
    SVGToOTFFontConverter converter(element);
    converter.convertSVGToOTFFont();
    return converter.releaseResult();
}

}
