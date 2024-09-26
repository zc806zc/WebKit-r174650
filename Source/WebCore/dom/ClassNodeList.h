/*
 * Copyright (C) 2007, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2007 David Smith (catfish.man@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ClassNodeList_h
#define ClassNodeList_h

#include "Element.h"
#include "LiveNodeList.h"
#include "Node.h"
#include "SpaceSplitString.h"

namespace WebCore {

class ClassNodeList final : public CachedLiveNodeList<ClassNodeList> {
public:
    static PassRef<ClassNodeList> create(ContainerNode&, const AtomicString& classNames);

    virtual ~ClassNodeList();

    virtual bool elementMatches(Element&) const override;
    virtual bool isRootedAtDocument() const override { return false; }

private:
    ClassNodeList(ContainerNode& rootNode, const AtomicString& classNames);

    SpaceSplitString m_classNames;
    AtomicString m_originalClassNames;
};

inline ClassNodeList::ClassNodeList(ContainerNode& rootNode, const AtomicString& classNames)
    : CachedLiveNodeList(rootNode, InvalidateOnClassAttrChange)
    , m_classNames(classNames, document().inQuirksMode())
    , m_originalClassNames(classNames)
{
}

inline bool ClassNodeList::elementMatches(Element& element) const
{
    if (!element.hasClass())
        return false;
    if (!m_classNames.size())
        return false;
    // FIXME: DOM4 allows getElementsByClassName to return non StyledElement.
    // https://bugs.webkit.org/show_bug.cgi?id=94718
    if (!element.isStyledElement())
        return false;
    return element.classNames().containsAll(m_classNames);
}

} // namespace WebCore

#endif // ClassNodeList_h
